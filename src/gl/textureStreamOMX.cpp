#include "textureStreamOMX.h"

#ifdef PLATFORM_RPI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../window.h"

#define IMAGE_SIZE_WIDTH 1920
#define IMAGE_SIZE_HEIGHT 1080

static OMX_BUFFERHEADERTYPE* eglBuffer = NULL;
static COMPONENT_T* egl_render = NULL;

static void* eglImage = 0;

int thread_run = 0;

void my_fill_buffer_done(void* data, COMPONENT_T* comp) {
   OMX_STATETYPE state;

   if (OMX_GetState(ILC_GET_HANDLE(egl_render), &state) != OMX_ErrorNone) {
      printf("OMX_FillThisBuffer failed while getting egl_render component state\n");
      return;
   }

   if (state != OMX_StateExecuting)
      return;

   if (OMX_FillThisBuffer(ilclient_get_handle(egl_render), eglBuffer) != OMX_ErrorNone)
      printf("OMX_FillThisBuffer failed in callback\n");
}

TextureStreamOMX::TextureStreamOMX() : 
    m_file(NULL),
    m_eglImage(0),
    m_video_decode(NULL),
    m_video_scheduler(NULL),
    m_port_settings_changed(0),
    m_first_packet(1),
    m_status(0)
    {

}

TextureStreamOMX::~TextureStreamOMX() {
    clear();
}

bool TextureStreamOMX::load(const std::string& _filepath, bool _vFlip) {

    // TODOs:
    //  - get video width and height

    glEnable(GL_TEXTURE_2D);

    // load three texture buffers but use them on six OGL|ES texture surfaces
    if (m_id == 0)
        glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexImage2D(   GL_TEXTURE_2D, 0, GL_RGBA, IMAGE_SIZE_WIDTH, IMAGE_SIZE_HEIGHT, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /* Create EGL Image */
    m_eglImage = eglCreateImageKHR(
                    getEGLDisplay(),
                    getEGLContext(),
                    EGL_GL_TEXTURE_2D_KHR,
                    (EGLClientBuffer)m_id,
                    0);
        
    if (m_eglImage == EGL_NO_IMAGE_KHR) {
        printf("eglCreateImageKHR failed.\n");
        exit(1);
    }

    memset(m_list, 0, sizeof(m_list));
    memset(m_tunnel, 0, sizeof(m_tunnel));

    if ((m_file = fopen(_filepath.c_str(), "rb")) == NULL)
        return (void *)-2;

    if ((m_client = ilclient_init()) == NULL) {
        fclose(m_file);
        return (void *)-3;
    }

    if (OMX_Init() != OMX_ErrorNone) {
        ilclient_destroy(m_client);
        fclose(m_file);
        return (void *)-4;
    }

    // callback
    ilclient_set_fill_buffer_done_callback(m_client, my_fill_buffer_done, 0);

    // create video_decode
    if (ilclient_create_component(m_client, &m_video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
        m_status = -14;
    m_list[0] = m_video_decode;

    // create egl_render
    if (m_status == 0 && ilclient_create_component(m_client, &egl_render, "egl_render", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_OUTPUT_BUFFERS) != 0)
        m_status = -14;
    m_list[1] = egl_render;

    // create clock
    COMPONENT_T* clock = NULL;
    if (m_status == 0 && ilclient_create_component(m_client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
        m_status = -14;
    m_list[2] = clock;

    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;

    memset(&cstate, 0, sizeof(cstate));
    cstate.nSize = sizeof(cstate);
    cstate.nVersion.nVersion = OMX_VERSION;
    cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
    cstate.nWaitMask = 1;
    if (clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
        m_status = -13;

    // create video_scheduler
    if (m_status == 0 && ilclient_create_component(m_client, &m_video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
        m_status = -14;
    m_list[3] = m_video_scheduler;

    set_tunnel(m_tunnel, m_video_decode, 131, m_video_scheduler, 10);
    set_tunnel(m_tunnel+1, m_video_scheduler, 11, egl_render, 220);
    set_tunnel(m_tunnel+2, clock, 80, m_video_scheduler, 12);

    // setup clock tunnel first
    if (m_status == 0 && ilclient_setup_tunnel(m_tunnel+2, 0, 0) != 0)
        m_status = -15;
    else
        ilclient_change_component_state(clock, OMX_StateExecuting);

    if (m_status == 0)
        ilclient_change_component_state(m_video_decode, OMX_StateIdle);

    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion = OMX_VERSION;
    format.nPortIndex = 130;
    format.eCompressionFormat = OMX_VIDEO_CodingAVC;

    if (m_status == 0 &&
        OMX_SetParameter(ILC_GET_HANDLE(m_video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
        ilclient_enable_port_buffers(m_video_decode, 130, NULL, NULL, NULL) == 0) {
            
        m_port_settings_changed = 0;
        m_first_packet = 1;

        ilclient_change_component_state(m_video_decode, OMX_StateExecuting);

        return true;
   }

    return false;
}

bool TextureStreamOMX::update() {
    unsigned int data_len = 0;

    if ( (m_buffer = ilclient_get_input_buffer(m_video_decode, 130, 1)) != NULL ) {
        // feed data and wait until we get port settings changed
        unsigned char *dest = m_buffer->pBuffer;

        // loop if at end
        if (feof(m_file))
            rewind(m_file);

        data_len += fread(dest, 1, m_buffer->nAllocLen - data_len, m_file);

        if  (m_port_settings_changed == 0 &&
            ((data_len > 0 && ilclient_remove_event(m_video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
             (data_len == 0 && ilclient_wait_for_event(m_video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                       ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0))) {
            m_port_settings_changed = 1;

            if (ilclient_setup_tunnel(m_tunnel, 0, 0) != 0) {
                m_status = -7;
                return false;
            }

            ilclient_change_component_state(m_video_scheduler, OMX_StateExecuting);

            // now setup tunnel to egl_render
            if (ilclient_setup_tunnel(m_tunnel+1, 0, 1000) != 0) {
                m_status = -12;
                return false;
            }

            // Set egl_render to idle
            ilclient_change_component_state(egl_render, OMX_StateIdle);

            // Enable the output port and tell egl_render to use the texture as a buffer
            //ilclient_enable_port(egl_render, 221); THIS BLOCKS SO CAN'T BE USED
            if (OMX_SendCommand(ILC_GET_HANDLE(egl_render), OMX_CommandPortEnable, 221, NULL) != OMX_ErrorNone) {
                printf("OMX_CommandPortEnable failed.\n");
                exit(1);
            }

            if (OMX_UseEGLImage(ILC_GET_HANDLE(egl_render), &eglBuffer, 221, NULL, eglImage) != OMX_ErrorNone) {
                printf("OMX_UseEGLImage failed.\n");
                exit(1);
            }

            // Set egl_render to executing
            ilclient_change_component_state(egl_render, OMX_StateExecuting);

            // Request egl_render to write data to the texture buffer
            if (OMX_FillThisBuffer(ILC_GET_HANDLE(egl_render), eglBuffer) != OMX_ErrorNone) {
                printf("OMX_FillThisBuffer failed.\n");
                exit(1);
            }
        }

        if (!data_len)
            return false;

        m_buffer->nFilledLen = data_len;
        data_len = 0;

        m_buffer->nOffset = 0;
        if (m_first_packet) {
            m_buffer->nFlags = OMX_BUFFERFLAG_STARTTIME;
            m_first_packet = 0;
        }
        else
            m_buffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

        if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_video_decode), m_buffer) != OMX_ErrorNone) {
            m_status = -6;
            return false;
        }
    }

}

void TextureStreamOMX::clear() {
    m_buffer->nFilledLen = 0;
    m_buffer->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

    if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_video_decode), m_buffer) != OMX_ErrorNone)
        m_status = -20;

    // need to flush the renderer to allow video_decode to disable its input port
    ilclient_flush_tunnels(m_tunnel, 0);

    ilclient_disable_port_buffers(m_video_decode, 130, NULL, NULL, NULL);

    fclose(m_file);

    ilclient_disable_tunnel(m_tunnel);
    ilclient_disable_tunnel(m_tunnel+1);
    ilclient_disable_tunnel(m_tunnel+2);
    ilclient_teardown_tunnels(m_tunnel);

    ilclient_state_transition(m_list, OMX_StateIdle);
    
    /*
        * To cleanup egl_render, we need to first disable its output port, then
        * free the output eglBuffer, and finally request the state transition
        * from to Loaded.
        */
    OMX_ERRORTYPE omx_err = OMX_SendCommand(ILC_GET_HANDLE(egl_render), OMX_CommandPortDisable, 221, NULL);
    if (omx_err != OMX_ErrorNone)
        printf("Failed OMX_SendCommand\n");
    
    omx_err = OMX_FreeBuffer(ILC_GET_HANDLE(egl_render), 221, eglBuffer);
    if(omx_err != OMX_ErrorNone)
        printf("OMX_FreeBuffer failed\n");

    ilclient_state_transition(m_list, OMX_StateLoaded);
    
    ilclient_cleanup_components(m_list);

    OMX_Deinit();

    ilclient_destroy(m_client);

    if (m_id != 0)
        glDeleteTextures(1, &m_id);
    m_id = 0;
}

#endif