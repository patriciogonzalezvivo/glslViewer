/*
omx_event_and_buffer_handler.c - Helper routines for working with OMX
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include "omx_event_and_buffer_handler.h"

#define OERR(cmd) cmd;
#define dumpport(a, b)
#define debug_printf(fmt, ...) {};

// So why does this even exist, why not use the ilclient library?
// Well I'm sure the ilclient library is just fine, but when you're dealing with OMX
// you very quickly get frustrated with the lack of transparency from the system,
// and start losing trust. And the worst doubt that creeps in when you're working on apps
// is the thought of "maybe it's the library's fault" "maybe it's the platforms fault"
// because it clouds ones debugging effectiveness.

// At the relatively small cost of a couple of hundred lines of code you can avoid some
// of that doubt.

// There's also very few decent examples of OMX usage on the Internet, even fewer
// of OMX image handling, and most of the ones that work don't use the ilclient library.
// When I see that I just go "ehhh, if they didn't gamble on ilclient, I'm not going to either"
// and instead learn a bit more about how the system actually works without the abstraction.

#define omx_event_group_count 4
#define omx_event_max_per_group 16
#define omx_buffers_max 16

// Why 4? Because that's all I needed. Hell I needed 3, but why be stingy.
// Basically these groups are the way that I deal with the little and big arcs
// of the OMX decode lifecycle. You start some processes going high up in the chain
// that will get back to you at some point, and you need to be able to catch them when
// they do, but in the meantime you need to get on with small OMX tasks that are spitting
// out their own events. Hence, groups.

void chompers_flush(){
  printf("Chompers_flush");
}

typedef struct omx_event_wait_t {
  int comp_id;                  // Renderer or Decoder
  int class_id;                 // General Event, Empty or Full Buffer
  int slot1;                    // event "type" for regular events, or buffer pointer for
                                // empty/full events
  int slot2;
  int slot3;
  int fired;
} omx_event_wait_t;

typedef struct omx_event_list_t {
  int count[omx_event_group_count];
  omx_event_wait_t events[omx_event_group_count][omx_event_max_per_group];
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} omx_event_list_t;

typedef struct chompers_queue_t {
  void *param1;
  void *param2;
  void *param3;
} chompers_queue_t;

OMX_BUFFERHEADERTYPE *decoder_bufferHeader[omx_buffers_max];
int decoder_bufferHeader_active[omx_buffers_max];
int decoder_numBuffers;
void *decoder_bufferPtr[16];

chompers_queue_t chompers_list[10];

int num_chompers;
pthread_mutex_t chompers_mutex;
pthread_cond_t chompers_cond;
pthread_t chompers_thread;

omx_event_list_t omx_event_list;


// When we want to start afresh and listen for some new events, we first clear out any
// previously waited for events.

void omx_event_reset_group (int group_id) {
  debug_printf ("OMX RESET EVENT GROUP %i ----------\n",group_id);
  pthread_mutex_lock (&omx_event_list.mutex);
  omx_event_list.count[group_id] = 0;
  pthread_mutex_unlock (&omx_event_list.mutex);
}

void omx_event_reset () {
  omx_event_reset_group (0);
}

// Add this event to the list of required events

void omx_event_require (int comp_id, int class_id, int slot1, int slot2, int slot3, int group) {
  int oc;
  pthread_mutex_lock (&omx_event_list.mutex);
  debug_printf ("<< REQ  OMX EVENT: COMP: %i CLASS: %i S1: %i S2: %i S3: %i\n", comp_id, class_id, slot1, slot2, slot3);
  oc = omx_event_list.count[group];

  omx_event_list.events[group][oc].comp_id = comp_id;
  omx_event_list.events[group][oc].class_id = class_id;
  omx_event_list.events[group][oc].slot1 = slot1;
  omx_event_list.events[group][oc].slot2 = slot2;
  omx_event_list.events[group][oc].slot3 = slot3;
  omx_event_list.events[group][oc].fired = 0;
  omx_event_list.count[group]++;
  pthread_mutex_unlock (&omx_event_list.mutex);
}

// A convenience function for the frequent action of requiring just one event

void omx_event_reset_and_require (int comp_id, int class_id, int slot1, int slot2, int slot3, int group) {
  omx_event_reset ();
  omx_event_require (comp_id, class_id, slot1, slot2, slot3, group);
}

// I added these to track out-of-band errors from the components
// (put it in a struct sometime please, got a nice event struct for this)

int error_comp_id;
int error_class_id;
int error_slot1;
int error_slot2;
int error_slot3;


// Once we've added a few events we're waiting for to a group we call this function
// to pause execution until all the events in the group are triggered.
// In some parts of the code I know errors or timeouts may signal problems and so I want to
// catch them there. In other parts, I'm not interested in timeouts because some mega decoding
// actions can exceed even a 2s timeout.

void omx_event_wait_group (int groupid, int abort_on_error, int timeout_s) {
  struct timeval stoptv;
  struct timespec ts;
  int i = 0;
  int q;
  int oc;
  int groups[omx_event_group_count];
  int res = 0;

  pthread_mutex_lock (&omx_event_list.mutex);
  error_timeout = 0;
  gettimeofday (&stoptv, NULL);
  ts.tv_sec = stoptv.tv_sec;
  ts.tv_nsec = stoptv.tv_usec * 1000;
  ts.tv_sec += timeout_s;

  while (1) {
    // Run through all of our queued events and check if they're all marked as fired
    // But why are you checking all groups instead of just the one requested?
    // Because it started with a slightly different idea of how events would come through
    // from OMX and whether I would need to have OR events instead of just AND ones.
    // So far I can get away with (AND events)* OR error OR timeout.
    // But OMX is fickle so I'm ready to switch back if I learn otherwise.
    
    for (q = 0; q < omx_event_group_count; q++) {
      oc = omx_event_list.count[q];
      groups[q] = oc;
      for (i = 0; i < oc; i++) {
        if (omx_event_list.events[q][i].fired == 1)
          groups[q]--;
      }
    }

    if (groups[groupid] == 0)
      break;
    if ((error_state) && (abort_on_error))
      break;
      
    // If not all the required events have fired, wait till we get a signal,
    // or if specified, a timeout.  

    if (timeout_s > 0) {
      res = pthread_cond_timedwait (&omx_event_list.cond, &omx_event_list.mutex, &ts);
      if (res == ETIMEDOUT) {
        error_timeout = 1;
        break;
      }
    } else {
      debug_printf("OMX EVENT WAITING FOR GROUP: %i\n", groupid);
      pthread_cond_wait (&omx_event_list.cond, &omx_event_list.mutex);
      }
  }
  pthread_mutex_unlock (&omx_event_list.mutex);
  debug_printf("OMX EVENT WAIT CLEARED: %i\n", groupid);
}

void omx_event_wait () {
  omx_event_wait_group (0, 0, 0);
}

// We've got an event, if we required it record it as being fired and send out a signal
// to a listening omx_event_wait_group.

void omx_event_handler_generic (int comp_id, int class_id, int slot1, int slot2, int slot3) {
  int flag = 0;
  int i;
  int q;

  pthread_mutex_lock (&omx_event_list.mutex);
  debug_printf (">> RECV OMX EVENT: COMP: %i CLASS: %i S1: %i (0x%08x) S2: %i (0x%08x) S3: %i (0x%08x)\n", comp_id, class_id, slot1, slot1, slot2, slot2, slot3, slot3);

  if ((class_id == OMX_EVENT_TYPE_CUSTOM) && ((OMX_EVENTTYPE) slot1 == OMX_EventError)) {
    flag = 1;
    error_slot1 = slot1;
    error_slot2 = slot2;
    error_slot3 = slot3;
    error_class_id = class_id;
    error_comp_id = comp_id;
    error_state = 1;
  }
  for (q = 0; q < omx_event_group_count; q++) {
    int oc = omx_event_list.count[q];

    for (i = 0; i < oc; i++) {
      if ((omx_event_list.events[q][i].comp_id == comp_id) &&
          (omx_event_list.events[q][i].class_id == class_id) &&
          (omx_event_list.events[q][i].slot1 == slot1) && (omx_event_list.events[q][i].slot2 == slot2) && (omx_event_list.events[q][i].slot3 == slot3)) {
        omx_event_list.events[q][i].fired = 1;
        flag = 1;
      }
    }
  }
  if (flag == 1)
    pthread_cond_signal (&omx_event_list.cond);
  pthread_mutex_unlock (&omx_event_list.mutex);
}

// Map the standard OMX callbacks through to our handler

OMX_ERRORTYPE omx_event_handler_custom (OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData) {
  omx_event_handler_generic ((int) pAppData, OMX_EVENT_TYPE_CUSTOM, eEvent, nData1, nData2);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_event_handler_empty_buffer (OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBufferHeader) {
  omx_event_handler_generic ((int) pAppData, OMX_EVENT_TYPE_EMPTY_BUFFER, (int) pBufferHeader, 0, 0);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_event_handler_filled_buffer (OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBufferHeader) {
  omx_event_handler_generic ((int) pAppData, OMX_EVENT_TYPE_FULL_BUFFER, (int) pBufferHeader, 0, 0);
  return OMX_ErrorNone;
}



void chompers_flush(doctor_chompers_t * dc){
  pthread_mutex_lock (&chompers_mutex);
  num_chompers = 0;
  pthread_mutex_unlock (&chompers_mutex);
}

void chompers_filler (OMX_HANDLETYPE hComponent, doctor_chompers_t * dc, OMX_BUFFERHEADERTYPE * header) {
  unsigned int to_transfer;
  int can_be_finished = 1;
  
  pthread_mutex_lock (&chompers_mutex);
  // If we have the finished flag it means one of our comrades has already cleared this point and
  // sent the EOS signal to the component. We're done, lets not send it another 
  // empty buffer with an EOS.
  if (!dc->finished) {
    // How much can we send? Smaller of the buffer size vs the amount of data we have left
    to_transfer = dc->data_size - dc->position;  
  
    // If we still have stuff left in our source buffer we know this can't be our last round
    // of buffer filling
    if (to_transfer > header->nAllocLen) {
      to_transfer = header->nAllocLen;
      can_be_finished = 0;
    };
    debug_printf ("CHOMPERS FILLING: POS: %i DC: %p HDR: %p To Transfer: %i\n", dc->position, dc, header, to_transfer);
    // Fill up the buffer and set the relevant flags and parameters in the header
    memcpy (header->pBuffer, dc->data + dc->position, to_transfer);

    header->nFilledLen = to_transfer;
    header->nOffset = 0;
    if (can_be_finished == 1) {
      // I added OMX_BUFFERFLAG_ENDOFFRAME because when OMX gives you shit you just
      // throw everything at it and hope something helps. And someone recommends it on
      // some random forum post. It's probably only applicable for video though.
      header->nFlags = OMX_BUFFERFLAG_EOS | OMX_BUFFERFLAG_ENDOFFRAME;
      debug_printf ("EOS set\n");
      dc->finished = 1;
    } else
      header->nFlags = 0;

    dc->position += to_transfer;
    // CAN'T CALL THIS FROM THE EVENT HANDLER! CAN'T CALL IT IF WE'RE IN THE EVENT HANDLER!
    // CAN'T RISK HOLDING A MUTEX THE EVENT HANDLER MAY BE HOLDING!
    // That means I learnt the hard way not to call OMX_EmptyBuffer from a buffer event handler
    // itself. Yes it's mentioned in the API documentation.
    // Hence chompers_filler is no longer directly called from the event handler, and is instead on a
    // separate thread gated by a signal.
    {
      int i;

      for (i = 0; i < decoder_numBuffers; i++) {
        if (decoder_bufferHeader[i] == header)
          decoder_bufferHeader_active[i] = 1;
      }
    }
    pthread_mutex_unlock (&chompers_mutex);
    if (OMX_EmptyThisBuffer (hComponent, header) != OMX_ErrorNone)
      debug_printf ("BUFFER FILL ERROR\n");
    debug_printf (" BUFFER EMPTY CALLED\n");
  } else
    pthread_mutex_unlock (&chompers_mutex);

}

// This is how you tell the buffer filling thread that we have a buffer free and
// it can throw the buffer to the OMX component when it gets a chance.

void add_to_chompers_queue (void *param1, void *param2, void *param3) {
  int i;

  pthread_mutex_lock (&chompers_mutex);
  chompers_list[num_chompers].param1 = param1;
  chompers_list[num_chompers].param2 = param2;
  chompers_list[num_chompers].param3 = param3;
  num_chompers++;
  for (i = 0; i < decoder_numBuffers; i++) {
    if (decoder_bufferHeader[i] == param3)
      decoder_bufferHeader_active[i] = 0;
  }
  pthread_cond_signal (&chompers_cond);
  pthread_mutex_unlock (&chompers_mutex);
}

// We have a slightly different callback for components whose buffer refilling is our
// responsibility (i.e. it actually means we have to deal with it). I don't want any
// chance that another component sends out an "empty buffer" message and then have the
// chompers mangle a buffer they can't handle.

OMX_ERRORTYPE omx_event_handler_empty_buffer_refiller (OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBufferHeader) {
  doctor_chompers_t *this_drt = (doctor_chompers_t *) pBufferHeader->pAppPrivate;
  debug_printf ("Valid pointer? %p\n", pBufferHeader);

  add_to_chompers_queue (hComponent, this_drt, pBufferHeader);
  omx_event_handler_generic ((int) pAppData, OMX_EVENT_TYPE_EMPTY_BUFFER, (int) pBufferHeader, 0, 0);
  return OMX_ErrorNone;
}


// The main method for the thread that waits for the chance to throw a buffer at a
// deserving component.

void *do_chompers (void *threadid) {
  pthread_mutex_lock (&chompers_mutex);
  while (1) {
    if (num_chompers > 0) {
      void *param1, *param2, *param3;
      num_chompers--;
      param1 = chompers_list[num_chompers].param1;
      param2 = chompers_list[num_chompers].param2;
      param3 = chompers_list[num_chompers].param3;

      pthread_mutex_unlock (&chompers_mutex);
      chompers_filler (param1, param2, param3);
      pthread_mutex_lock (&chompers_mutex);
    } else {
      pthread_cond_wait (&chompers_cond, &chompers_mutex);
    }
  }
  pthread_mutex_unlock (&chompers_mutex);
  pthread_exit(NULL);
}

// Clear everything and get the mutex setup

void omx_event_setup () {
  int i = 0;
  pthread_mutex_init (&chompers_mutex, NULL);
  pthread_cond_init (&chompers_cond, NULL);
  num_chompers = 0;

  pthread_create (&chompers_thread, NULL, do_chompers, NULL);

  for (i = 0; i < omx_event_group_count; i++)
    omx_event_list.count[i] = 0;
  pthread_mutex_init (&omx_event_list.mutex, NULL);
  pthread_cond_init (&omx_event_list.cond, NULL);
}

void omx_buffers_remove (struct OMX_COMPONENTTYPE *handle, int port) {
  int i;
  for (i = 0; i < decoder_numBuffers; i++) {
    OMX_FreeBuffer (handle, port, decoder_bufferHeader[i]);
    free (decoder_bufferPtr[i]);
  }
  decoder_numBuffers = 0;
}

void omx_buffers_setup (struct OMX_COMPONENTTYPE *handle, int port, int alignment, int size, int count, void* ptr) {
  int i;
  decoder_numBuffers = count;
  for (i = 0; i < decoder_numBuffers; i++) {
    posix_memalign (&decoder_bufferPtr[i], alignment, size);
    OMX_UseBuffer (handle, &decoder_bufferHeader[i], port, ptr, size, decoder_bufferPtr[i]);
    decoder_bufferHeader_active[i] = 0;
  }
}

void omx_buffers_activate(struct OMX_COMPONENTTYPE *handle, void* ptr){
  int i;

  for (i = 0; i < decoder_numBuffers; i++) {
    add_to_chompers_queue (handle, ptr, decoder_bufferHeader[i]);
  }
}