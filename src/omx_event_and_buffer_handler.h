#pragma once

#include "gl.h"
#include <IL/OMX_Broadcom.h>
#include <pthread.h>

// Why OMX_Broadcom? Because we want access to on of the param definitions on the resizer
// component that isn't available with the "normal" include.

typedef enum { OMX_EVENT_TYPE_CUSTOM, OMX_EVENT_TYPE_FULL_BUFFER, OMX_EVENT_TYPE_EMPTY_BUFFER } omx_event_t;

typedef struct doctor_chompers_t {
//  pthread_mutex_t mutex;
  char* data;
  unsigned int position;
  unsigned int data_size;
  int finished;
} doctor_chompers_t;

void omx_event_setup ();
void omx_event_reset ();
void omx_event_reset_group (int group_id);
void omx_event_require (int comp_id, int class_id, int slot1, int slot2, int slot3, int group) ;
void omx_event_reset_and_require (int comp_id, int class_id, int slot1, int slot2, int slot3, int group);
void omx_event_wait () ;
void omx_event_wait_group (int groupid, int abort_on_error, int timeout_s);

void add_to_chompers_queue (void *param1, void *param2, void *param3);
void chompers_flush();

void omx_buffers_remove (struct OMX_COMPONENTTYPE *handle, int port);
void omx_buffers_setup (struct OMX_COMPONENTTYPE *handle, int port, int alignment, int size, int count, void* ptr);
void omx_buffers_activate(struct OMX_COMPONENTTYPE *handle, void* ptr);

OMX_ERRORTYPE omx_event_handler_custom (OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
OMX_ERRORTYPE omx_event_handler_empty_buffer (OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBufferHeader);
OMX_ERRORTYPE omx_event_handler_filled_buffer (OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBufferHeader);
OMX_ERRORTYPE omx_event_handler_empty_buffer_refiller (OMX_HANDLETYPE hComponent, OMX_PTR pAppData, OMX_BUFFERHEADERTYPE * pBufferHeader);
