/*
omx_image.c - image decoding and resizing via OpenMAX to OpenGL
Copyright 2013 Bryn Thomas <brynthomas@gmail.com>

This program is free software; you can redistribute it and/or modify it 
under the terms of the GNU General Public License, version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <IL/OMX_Broadcom.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "omx_image.h"
#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define OERR(cmd) cmd;
#define dumpport(a, b)
#define debug_printf(fmt, ...) {};

typedef enum { OMX_NOT_USED, OMX_COMP_ID_RENDERER, OMX_COMP_ID_DECODER, OMX_COMP_ID_RESIZER } omx_component_t;

int jpeg_run_once = 0;
OMX_HANDLETYPE decoder_handle = 0;
OMX_HANDLETYPE renderer_handle = 0;
OMX_HANDLETYPE resizer_handle = 0;
int decoder_inPort, decoder_outPort;
int renderer_inPort, renderer_outPort;
int resizer_inPort, resizer_outPort;

EGLDisplay *eglDisplay;
EGLContext *eglContext;

OMX_BUFFERHEADERTYPE *texture_bufferHeader;
EGLImageKHR texture_mem_handle;

doctor_chompers_t *jpeg_buffer;

// A lot of the time the process is going to be:
// "send command" -> "wait for acknowledgement of same command"
// This makes it easier.

// But why does it use the component to work out the handle in this shoddy way?
// While I was writing the code I was shifting code and copying and pasting loads,
// and there's nothing more irritating that a bug where a component is not responding
// because you've told it to enable another components port because you forgot to change
// it as part of C&P.

void omx_event_queue_and_send_cmd (int comp_id, int slot2, int slot3) {
  OMX_HANDLETYPE this_handle = NULL;

  if (comp_id == OMX_COMP_ID_RESIZER)
    this_handle = resizer_handle;
  if (comp_id == OMX_COMP_ID_RENDERER)
    this_handle = renderer_handle;
  if (comp_id == OMX_COMP_ID_DECODER)
    this_handle = decoder_handle;
    
  omx_event_require (comp_id, OMX_EVENT_TYPE_CUSTOM, OMX_EventCmdComplete, slot2, slot3, 0);
  OMX_SendCommand (this_handle, slot2, slot3, NULL);
}

void omx_image_fill_buffer_from_disk (char *filename, doctor_chompers_t * buffer) {
  int32_t pos;
  FILE *fp;
  
  buffer->data_size = 0;
  buffer->data = NULL;
  buffer->position = 0;
  fp = fopen (filename, "rb");

  if (!fp)
    return;
  if (fseek (fp, 0L, SEEK_END) < 0) {
    fclose (fp);
    return;
  };
  pos = ftell (fp);
  debug_printf ("%i\n", pos);
  if (pos == LONG_MAX) {
    fclose (fp);
    return;
  };
  buffer->data_size = pos;
  fseek (fp, 0L, SEEK_SET);
  buffer->data = malloc (buffer->data_size);
  fread (buffer->data, 1, buffer->data_size, fp);
  buffer->position = 0;
  fclose (fp);

}

void omx_rollback (int stage) ;

void omx_power_on () {
  debug_printf ("Initing\n");
  // First step always, fire up OpenMAX
  OMX_Init ();
  
  // We use three components in the process of getting our image from a JPG/PNG into our OpenGL texture
  // We need to ask OMX to give us handles to each of them, and give the components functions
  // that they can call as event callbacks.
  {
    OMX_CALLBACKTYPE m_callbacks;
    OMX_CALLBACKTYPE m_callbacks_gen;

    m_callbacks.EventHandler = omx_event_handler_custom;
    m_callbacks.EmptyBufferDone = omx_event_handler_empty_buffer_refiller;
    m_callbacks.FillBufferDone = omx_event_handler_filled_buffer;
    // The decoder gets a different "EmptyBufferDone" to the other two, because the decoder
    // is where we're going to shovel data into from our application buffers. The other two
    // will be shoveling data between other OpenMAX components and an OpenGL buffer.
    // So we don't care about fulfilling their buffer needs (we still want the event though).
    
    // Our first component is the decoder. You stick JPG/PNG data into the buffer in the front and you
    // get YUV or RGB data out the other end.
    OMX_GetHandle (&decoder_handle, (char *)"OMX.broadcom.image_decode", (void *) OMX_COMP_ID_DECODER, &m_callbacks);

    m_callbacks_gen.EventHandler = omx_event_handler_custom;
    m_callbacks_gen.EmptyBufferDone = omx_event_handler_empty_buffer;
    m_callbacks_gen.FillBufferDone = omx_event_handler_filled_buffer;
    // Our second component is the resizer. You pass it RGB/YUV data in the front and it can resize
    // it, and spit out RGB/YUV data on the other end. I think it can crop too if you want.
    OMX_GetHandle (&resizer_handle, (char *)"OMX.broadcom.resize", (void *) OMX_COMP_ID_RESIZER, &m_callbacks_gen);
    // The final component egl_render takes in RGB/YUV data and dumps it into an OpenGL texture on
    // the other end.
    OMX_GetHandle (&renderer_handle, (char *)"OMX.broadcom.egl_render", (void *) OMX_COMP_ID_RENDERER, &m_callbacks_gen);
  }
    
  // Above we got our handles, and that lets us talk to the component as a whole. But each of these
  // components has an input and an output port (at least) to which we also will need to talk
  // in order to control specific settings and to bind buffers and tunnels.
  
  // So lets get the port numbers and give them "human" names so it's easier to work with.
  {
    OMX_PORT_PARAM_TYPE port;

    port.nSize = sizeof (OMX_PORT_PARAM_TYPE);
    port.nVersion.nVersion = OMX_VERSION;
    OMX_GetParameter (decoder_handle, OMX_IndexParamImageInit, &port);
    if (port.nPorts != 2)
      debug_printf ("Whoops, decoder ports!\n");
    decoder_inPort = port.nStartPortNumber;
    decoder_outPort = port.nStartPortNumber + 1;

    OMX_GetParameter (renderer_handle, OMX_IndexParamVideoInit, &port);
    if (port.nPorts != 2)
      debug_printf ("Whoops, renderer ports!\n");
    renderer_inPort = port.nStartPortNumber;
    renderer_outPort = port.nStartPortNumber + 1;

    OMX_GetParameter (resizer_handle, OMX_IndexParamImageInit, &port);
    if (port.nPorts != 2)
      debug_printf ("Whoops, resizer ports!\n");
    resizer_inPort = port.nStartPortNumber;
    resizer_outPort = port.nStartPortNumber + 1;
  }

  // First step, disable all the ports. Why? What state are they in initially? Enabled maybe?
  // Just do it. The components are probably in StateLoaded, instead of StateIdle just do it.
  // Whenever using OMX, not doing this as your first step = tears
  {
    omx_event_reset ();
    omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandPortDisable, decoder_inPort);
    omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandPortDisable, decoder_outPort);

    omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandPortDisable, renderer_inPort);
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandPortDisable, renderer_outPort);

    omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandPortDisable, resizer_inPort);
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandPortDisable, resizer_outPort);
    
    // Ports all off? Maybe it's finished by now, either way we can stick all the components into 
    // Idle state.

    omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandStateSet, OMX_StateIdle);
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandStateSet, OMX_StateIdle);
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandStateSet, OMX_StateIdle);
    omx_event_wait ();
    
    // And we wait for confirmation that everything is clear.
  }
}



int omx_image_loader (int texture_id, EGLDisplay * i_eglDisplay, EGLContext * i_eglContext, int force_width, int force_height, int *output_width, int *output_height,
                      decoder_image_format_t image_format, doctor_chompers_t * input_buffer) {
  // We're going to need access to the display/context later and in rollback so lets hold onto these
  eglDisplay = i_eglDisplay;
  eglContext = i_eglContext;
  error_state = 0;
  
  // Don't waste my time with an empty buffer.
  if (input_buffer->data_size == 0)
    return -1;
    
  // I have one internal structure that I use for all processing, so when
  // I get a different input_buffer I extract the relevant parameters into that
  // Mostly because the "doctor chompers" input buffer is not a final structure for how I'd like
  // to pass data, would rather have my own "fill" buffer callback to make for better
  // streaming performance. Anything called doctor chompers clearly is not final.
  
  // At the end of it this is a buffer which holds the size, position and data of our JPG/PNG.
  if (jpeg_run_once == 0) {
    jpeg_buffer = malloc (sizeof (doctor_chompers_t));
  }
  jpeg_buffer->finished = 0;
  jpeg_buffer->data = input_buffer->data;
  jpeg_buffer->position = input_buffer->position;
  jpeg_buffer->data_size = input_buffer->data_size;

  // Call the setup stuff that only needs to happen once.
  // Over revisions of this I've varied the amount that I "rollback" the amount of setup that I do
  // such as leaving tunnels in place between image runs, or leaving ports in the enabled state.
  // Ultimately it's come down to just leaving OMX running as well as keeping the handles active.
  // Why? It gives more flexibility in the changes you can make to the kinds and sizes of images read
  // as I'll explain in the comments later.
  
  // One thing that's important to understand with OMX is that everything happens in a
  // very specific order, and if you need to make a change to a setting that happens early on.
  // Well, you'd better get ready to unravel each step you did afterwards to get it into the same
  // state, then make the change to the setting, then replay forward again.
  // (such as switching from JPG to PNG and visa versa)
  
  // I don't feel like tracking that so I just unroll everything. If I hit a performance wall
  // I'll probably come back and review this and track it more carefully and be smarter about it.
  
  if (jpeg_run_once == 0) {
    omx_event_setup ();
    omx_power_on ();
    jpeg_run_once = 1;
  }

  // Call up the decoder input port and tell it what kind of image to expect.
  {
    OMX_IMAGE_PARAM_PORTFORMATTYPE imagePortFormat;

    memset (&imagePortFormat, 0, sizeof (imagePortFormat));
    imagePortFormat.nSize = sizeof (imagePortFormat);
    imagePortFormat.nVersion.nVersion = OMX_VERSION;
    imagePortFormat.nPortIndex = decoder_inPort;
    if (image_format == JPEG)
      imagePortFormat.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    else if (image_format == PNG)
      imagePortFormat.eCompressionFormat = OMX_IMAGE_CodingPNG;
    debug_printf ("Image Format %i\n", imagePortFormat.eCompressionFormat);
    OERR (OMX_SetParameter (decoder_handle, OMX_IndexParamImagePortFormat, &imagePortFormat));
  }
  dumpport (decoder_handle, decoder_inPort);

  // This is not generally needed but it's one of those hacky things done to make the
  // code a bit more consistent later. I wanted to receive a specific event when it
  // had decoded enough to know the size of the image, but if I decoded multiple images
  // in a row which had the same size I didn't get my nice event. So I just mess around
  // with what the size is on the output port first, so that when it reads the JPG and
  // changes it, it generates a new event.
  
  // But what if your JPG is actually 160x64? Whoops.
  {
    OMX_PARAM_PORTDEFINITIONTYPE portdef;

    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = decoder_outPort;
    OMX_GetParameter (decoder_handle, OMX_IndexParamPortDefinition, &portdef);
    portdef.format.image.nFrameWidth = 160;
    portdef.format.image.nFrameHeight = 64;
    portdef.format.image.nStride = 160;
    portdef.format.image.nSliceHeight = 64;
    OERR (OMX_SetParameter (decoder_handle, OMX_IndexParamPortDefinition, &portdef));
  }

  // I call up the decoder input port again to find out the size and number of input buffers
  // that it needs or allows for me to load the JPG/PNG data into.
  {
    OMX_PARAM_PORTDEFINITIONTYPE portdef;

    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = decoder_inPort;
    OMX_GetParameter (decoder_handle, OMX_IndexParamPortDefinition, &portdef);

    // You really should not need to force the buffersize here on the input decoder port.
    // This library works just fine streaming in data by filling and rotating multiple small
    // buffers
    // Except.....
    // I have a progressive JPEG (id: 1332877510.2) which, yes, should fail
    // However it doesn't fail like other progressive JPEGs where OpenMAX returns the expected
    // 0x8000100b (OMX_ErrorStreamCorrupt) message but you can normally then unroll OpenMax and
    // continue.
    // With this file it doesn't return confirmation of emptying at least one of the buffers
    // which it should do even when it has an error condition.
    // It then goes into an unusable state where any command you send (or even
    // OMX_SetParameter) will result in OMX freezing.
    // You can't reset it by OMX_FreeHandle, or OMX_Deinit, it's dead.

    // So to work around this one JPG, and potentially any other similar JPG out there, I
    // force the buffer to be the size of the image I'm reading. Then it doesn't crash on this file.
    // But then I think if I'm doing this, why am I not just passing it my original JPEG
    // buffer that I read in and avoiding this double JPG buffer.
    // Except I'd have to posix_memalign that buffer when creating it, and set the
    // portAligment
    // But it's a bigass buffer so I could just use a chunky alignment like 64 and it wouldn't
    // matter the few bytes wasted, and it would be pretty safe. But I don't like making that
    // an acceptable assumption by the calling code, I want small buffers, and hope I can find
    // a better workaround or bugfix.

    // Really do dislike not rotating small buffers, and it's also kind of against the style
    // of OpenMAX programming.
    // But big buffers are still better than the entire program being unrecoverable
    // on random JPEGs.
    if ((image_format == JPEG) && (jpeg_buffer->data_size > portdef.nBufferSize)) {
      portdef.nBufferSize = jpeg_buffer->data_size;
      // So if I'm forced to go and give it a giant buffer, I don't want to have to allocate it
      // too many of them (heck, it all fits in one).
      // Can't go less than 2, still less than the default of 3 though.
      portdef.nBufferCountActual = 2;
      OERR (OMX_SetParameter (decoder_handle, OMX_IndexParamPortDefinition, &portdef));
      OMX_GetParameter (decoder_handle, OMX_IndexParamPortDefinition, &portdef);
    }

    // Now we switch on the decoder input port and allocate and give it its buffers at the same time.
    omx_event_reset ();
    omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandPortEnable, decoder_inPort);
    
    omx_buffers_setup(decoder_handle, decoder_inPort, portdef.nBufferAlignment, portdef.nBufferSize, portdef.nBufferCountActual, jpeg_buffer);
    
    omx_event_wait ();
  }

  debug_printf ("Checkpoint F\n");
  
  // Decoder input is on buffers are in place, lets fire up the decoder.
  // There isn't anything yet for it to decode, but it'll be ready to go.

  omx_event_reset ();
  omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandStateSet, OMX_StateExecuting);
  omx_event_wait ();

  // These are two events which I monitor later on in the program and are generally the source of 
  // timeouts and errors for dud JPGs (and tell me when important stages are finished).
  omx_event_reset_group (1);
  omx_event_require (OMX_COMP_ID_RESIZER, OMX_EVENT_TYPE_CUSTOM, OMX_EventBufferFlag, resizer_outPort, 1, 1);
  omx_event_reset_group (2);
  omx_event_require (OMX_COMP_ID_DECODER, OMX_EVENT_TYPE_CUSTOM, OMX_EventPortSettingsChanged, decoder_outPort, 0, 2);

  // We now begin throwing our real data into buffers and from there into the decoder component
  omx_buffers_activate(decoder_handle, jpeg_buffer);
 
  // We need to wait for enough of the data to go through that it has some port information
  // such as resolution etc, hence waiting for the "EventPortSettingsChanged"
 
  // While throwing arbitrary data at this, I see that if you send a small enough piece of
  // data, then even though it's an invalid JPEG and you send EOS, you don't get a useful
  // error out (as in any error). We use the timeout to catch those which mucks with performance
  // on bad JPGs.
  omx_event_wait_group (2, 1, 1);
  if (error_state == 1) {
    int error_code = -1;

    debug_printf ("OpenMAX! Rollback.\n");
    
    // Unwind the stuff we've done so far so that when we get called again we're in a known,
    // consistent state. 

    omx_rollback (1);
    
    // If we get here there's still a decent chance that the JPG can be read. Maybe it's just
    // a progressive JPG that OMX can't handle natively and we can fix it up and send it again.
    
    // The error_codes returned don't mean anything except for the point at where it occurred,
    // which hints as to if it's a progressive JPG vs a more permanent error.

    return error_code;
  };

  if (error_timeout == 1) {
    int error_code = -2;

    debug_printf ("OpenMAX! Rollback due to timeout.\n");

    omx_rollback (1);

    return error_code;
  };

  debug_printf ("SUPER IMPORTANT PORT VALUES\n");

  dumpport (decoder_handle, decoder_outPort);
  dumpport (resizer_handle, resizer_inPort);

  {
    OMX_PARAM_PORTDEFINITIONTYPE portdefa;

    portdefa.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdefa.nVersion.nVersion = OMX_VERSION;
    portdefa.nPortIndex = resizer_outPort;
    OERR (OMX_GetParameter (resizer_handle, OMX_IndexParamPortDefinition, &portdefa));
    // If you don't set the F***ING stride to 0 here, and you did a round of decoding before,
    // well then it will use the values from there and dick up when it comes time to hooking
    // up to the renderer causing the tunnel to fail.
    // It autodetects wonderfully for the first image you do, but then it mucks up
    // when it reuses its previously autodetected settings.
    // Or you'll have a stride that's too big and garbage will appear all over the screen.
    // Either way you lose.
    // Why not set it to the "correct" value? Because it won't accept the real stride value
    // here. SetParameter will fail on the resizer if you try.
    portdefa.format.image.nStride = 0;  // multi of 32
    portdefa.format.image.nSliceHeight = 0;     // multi of 16
    OERR (OMX_SetParameter (resizer_handle, OMX_IndexParamPortDefinition, &portdefa));
  };

  // We check the size of the image that the decoder has, compare it to the requested
  // width and height, and set appropriately. If 0 on both requested sizes we just 
  // use the decoder values. If 0 on one of them, we use the other value and maintain
  // aspect ratio.
  
  // We never let the texture get bigger than 1920x1080. OpenGL ES limits you to 2048x2048
  // but I don't need more than will fit on the screen for my own uses.
  
  {
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    OMX_PARAM_RESIZETYPE portdefalt;
    int res_height;
    int res_width;
    
    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = decoder_outPort;
    OMX_GetParameter (decoder_handle, OMX_IndexParamPortDefinition, &portdef);
    portdefalt.nSize = sizeof (OMX_PARAM_RESIZETYPE);
    portdefalt.nVersion.nVersion = OMX_VERSION;
    portdefalt.nPortIndex = resizer_outPort;
    res_height = portdef.format.image.nFrameHeight;
    res_width = portdef.format.image.nFrameWidth;

    if ((force_height != 0) || (force_width != 0)) {
      if ((force_height != 0) && (force_width != 0)) {
        res_width = force_width;
        res_height = force_height;
      } else if ((force_width != 0)) {
        res_height = (unsigned int) (((double) force_width / (double) res_width) * (double) res_height);
        res_width = force_width;
      } else {
        res_width = (unsigned int) (((double) force_height / (double) res_height) * (double) res_width);
        res_height = force_height;
      }
    }
    if (res_width > 1920) {
      res_height = (unsigned int) (((double) 1920 / (double) res_width) * (double) res_height);
      res_width = 1920;

    }
    if (res_height > 1080) {
      res_width = (unsigned int) (((double) 1080 / (double) res_height) * (double) res_width);
      res_height = 1080;

    }
    portdefalt.nMaxWidth = res_width;
    portdefalt.nMaxHeight = res_height;
    portdefalt.eMode = OMX_RESIZE_BOX;
    portdefalt.bPreserveAspectRatio = OMX_FALSE;
    portdefalt.bAllowUpscaling = OMX_FALSE;
    OMX_SetParameter (resizer_handle, OMX_IndexParamResize, &portdefalt);
  }

  // Now we can link the decoder output to the resizer input and enable their ports
  // We should get an EventPortSettings change pop out from the other end of the resizer.
  omx_event_reset ();
  omx_event_require (OMX_COMP_ID_RESIZER, OMX_EVENT_TYPE_CUSTOM, OMX_EventPortSettingsChanged, resizer_outPort, 0, 0);

  debug_printf ("Checkpoint G\n");

  OERR (OMX_SetupTunnel (decoder_handle, decoder_outPort, resizer_handle, resizer_inPort));

  omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandPortEnable, decoder_outPort);
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandPortEnable, resizer_inPort);

  omx_event_wait ();

  debug_printf ("Checkpoint H\n");
  
  // Power on the resizer

  omx_event_reset ();
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandStateSet, OMX_StateExecuting);
  omx_event_wait ();
  
  dumpport (resizer_handle, resizer_outPort);
  dumpport (renderer_handle, renderer_inPort);
  debug_printf ("NOW AFTER\n");
  
  // And now we do the same linking process between the resizer and renderer.
  // Setup the tunnel between an outport and an inport and switch on the component.

  omx_event_reset ();
  OERR (OMX_SetupTunnel (resizer_handle, resizer_outPort, renderer_handle, renderer_inPort));

  omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandPortEnable, resizer_outPort);
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandPortEnable, renderer_inPort);
  omx_event_wait ();

  dumpport (resizer_handle, resizer_outPort);
  dumpport (renderer_handle, renderer_inPort);
  
  // Now I can switch on the renderer?
  // But wait... where will the output of the renderer go if it's running?
  // Shhh we have to do that later.

  omx_event_reset ();
  omx_event_require (OMX_COMP_ID_RENDERER, OMX_EVENT_TYPE_CUSTOM, OMX_EventPortSettingsChanged, renderer_outPort, 0, 0);
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandStateSet, OMX_StateExecuting);
  omx_event_wait ();


  debug_printf ("Checkpoint I\n");

  // Call up the resizer outport and find out what size image it's actually going to/has generated.
  // We'll use that to set up our OpenGL ES texture and get a "GPU pointer" to it.
  {
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    int real_output_width, real_output_height;

    portdef.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = resizer_outPort;
    OMX_GetParameter (resizer_handle, OMX_IndexParamPortDefinition, &portdef);

    real_output_width = (int) portdef.format.image.nFrameWidth;
    real_output_height = (int) portdef.format.image.nFrameHeight;
    debug_printf ("W: %i H: %i\n", real_output_width, real_output_height);
    if (output_width)
      *output_width = real_output_width;
    if (output_height)
      *output_height = real_output_height;
    glBindTexture (GL_TEXTURE_2D, texture_id);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // You better set the GL_TEXTURE_MIN_FILTER here to GL_LINEAR here, otherwise it default to
    // creating its mipmaps and pisses away gobs of video RAM. A 1920x1080 image goes from using roughly
    // 8MB to 14MB. And you have to do it BEFORE the glTexImage2D command.
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, real_output_width, real_output_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    debug_printf ("GL ERROR: %i . texid: %i\n", glGetError (), texture_id);
    // This gives us a handle into GPU memory that we can use when defining our output buffer.
    texture_mem_handle = (void *) eglCreateImageKHR (*eglDisplay, *eglContext, EGL_GL_TEXTURE_2D_KHR, (EGLClientBuffer) texture_id, 0);

    debug_printf ("EGL Pointer: %p %i, %i\n", texture_mem_handle, real_output_width, real_output_height);
  }
  debug_printf ("Handle obtained\n");

  // Switch on the renderer output port and tell that port that it can use the texture as the buffer.
  omx_event_reset ();
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandPortEnable, renderer_outPort);
  OMX_UseEGLImage (renderer_handle, &texture_bufferHeader, renderer_outPort, NULL, texture_mem_handle);
  omx_event_wait ();
  // JPEG test id: 1328550540.1 shows that if you have a later 0x8000100b stream error you
  // need to catch it here. This one is based on an incomplete transfer of a JPEG file.
  omx_event_wait_group (1, 1, 0);
  if (error_state == 1) {
    int error_code = -3;

    debug_printf ("OpenMAX Pt 2! Rollback.\n");

    omx_rollback (3);

    return error_code;
  };
  

  debug_printf ("WG1 passed\n");

  // Yay! If you made it to this point without errors you probably have a successfully decoded file.
  // Just wait for it to finish dumping the image into the texture.
  omx_event_reset ();
  omx_event_require (OMX_COMP_ID_RENDERER, OMX_EVENT_TYPE_FULL_BUFFER, (int) texture_bufferHeader, 0, 0, 0);
  OMX_FillThisBuffer (renderer_handle, texture_bufferHeader);
  omx_event_wait ();
  
  debug_printf ("Now I want everything to SHUT DOWN\n");
  
  // And now we roll back everything to an idle state (disconnect tunnels, disable ports etc)

  omx_rollback (3);
  return 0;
}

void omx_rollback(int stage) {
  // No matter the reason, the first step is to get it to stop making the problem worse by
  // feeding further data into the decoder more info.
  jpeg_buffer->finished = 1;
  chompers_flush();
  
  // Depending from where we stopped decoding we have a horrible stage flag system to
  // change what gets rolled back.
  
  // Ultimately it's just:
  // 1. switch everything into idle
  // 2. switch off ports without buffers
  // 3. disable tunnels
  // 4. for ports with buffers, switch them off and remove the buffers at the same time

  omx_event_reset ();
  omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandStateSet, OMX_StateIdle);
  omx_event_wait ();
  if (stage >= 3) {
    omx_event_reset ();
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandStateSet, OMX_StateIdle);
    omx_event_wait ();
    omx_event_reset ();
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandStateSet, OMX_StateIdle);
    omx_event_wait ();

    omx_event_reset ();
    omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandPortDisable, decoder_outPort);
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandPortDisable, resizer_inPort);
    omx_event_wait ();

    omx_event_reset ();
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandPortDisable, resizer_outPort);
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandPortDisable, renderer_inPort);
    omx_event_wait ();

    OMX_SetupTunnel (decoder_handle, decoder_outPort, NULL, 0);
    OMX_SetupTunnel (resizer_handle, resizer_outPort, NULL, 0);

    omx_event_reset ();

    omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandPortDisable, renderer_outPort);
    OMX_FreeBuffer (renderer_handle, renderer_outPort, texture_bufferHeader);
    omx_event_wait ();

    eglDestroyImageKHR (*eglDisplay, texture_mem_handle);
    
    // Why am I toggling between loaded and idle... I don't know
    omx_event_reset ();
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandStateSet, OMX_StateLoaded);
    omx_event_wait ();
    omx_event_reset ();
    omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandStateSet, OMX_StateIdle);
    omx_event_wait ();
  }
  omx_event_reset ();
  omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandPortDisable, decoder_inPort);

  // I flush buffers and shut down the decoder so that I'm allowed to change the image format
  // from PNG to JPEG etc.

  omx_buffers_remove(decoder_handle, decoder_inPort);
  omx_event_wait ();

  omx_event_reset ();
  omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandFlush, decoder_inPort);
  omx_event_queue_and_send_cmd (OMX_COMP_ID_DECODER, OMX_CommandFlush, decoder_outPort);
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandFlush, resizer_inPort);
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RESIZER, OMX_CommandFlush, resizer_outPort);
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandFlush, renderer_inPort);
  omx_event_queue_and_send_cmd (OMX_COMP_ID_RENDERER, OMX_CommandFlush, renderer_outPort);
  omx_event_wait ();
}

// I wrote this while trying to get around OMX freezing with that one progressive JPG, hoping
// that a full Deinit would cure it. It didn't (would freeze even during the Deinit), but
// leaving the code in for now till I finish the cleanup.
void omx_power_off () {
  debug_printf ("Power off starting\n");
  // OMX_FreeHandle(decoder_handle);
  debug_printf ("Decoder freed\n");
  // OMX_FreeHandle(renderer_handle);
  debug_printf ("Renderer freed\n");
  // OMX_FreeHandle(resizer_handle);
  debug_printf ("Trying to deinit\n");
  OMX_Deinit ();
  debug_printf ("Deinited\n");
}