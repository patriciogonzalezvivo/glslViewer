#include "textureStreamOMX.h"

#ifdef PLATFORM_RPI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IL/OMX_Component.h"
#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_logging.h"

#include "interface/vmcs_host/vcilcs.h"
#include "interface/vmcs_host/vchost.h"
#include "interface/vmcs_host/vcilcs_common.h"

#include "../window.h"

#define IMAGE_SIZE_WIDTH 1920
#define IMAGE_SIZE_HEIGHT 1080

#define set_tunnel(t,a,b,c,d)  do {TUNNEL_T *_ilct = (t); \
  _ilct->source = (a); _ilct->source_port = (b); \
  _ilct->sink = (c); _ilct->sink_port = (d);} while(0)

#define ILC_GET_HANDLE(x) ilclient_get_handle(x)

#ifndef OMX_SKIP64BIT
#define ilclient_ticks_from_s64(s) (s)
#define ilclient_ticks_to_s64(t)   (t)
#else
static inline OMX_TICKS ilclient_ticks_from_s64(int64_t s) {
   OMX_TICKS ret;
   ret.nLowPart = s;
   ret.nHighPart = s>>32;
   return ret;
}
static inline int64_t ilclient_ticks_to_s64(OMX_TICKS t) {
   uint64_t u = t.nLowPart | ((uint64_t)t.nHighPart << 32);
   return u;
}
#endif

#define VCOS_LOG_CATEGORY (&ilclient_log_category)
#ifndef ILCLIENT_THREAD_DEFAULT_STACK_SIZE
#define ILCLIENT_THREAD_DEFAULT_STACK_SIZE   (6<<10)
#endif

typedef enum {
    ILCLIENT_EMPTY_BUFFER_DONE  = 0x1,
    ILCLIENT_FILL_BUFFER_DONE   = 0x2,
    ILCLIENT_PORT_DISABLED      = 0x4,
    ILCLIENT_PORT_ENABLED       = 0x8,
    ILCLIENT_STATE_CHANGED      = 0x10,
    ILCLIENT_BUFFER_FLAG_EOS    = 0x20,
    ILCLIENT_PARAMETER_CHANGED  = 0x40,
    ILCLIENT_EVENT_ERROR        = 0x80,
    ILCLIENT_PORT_FLUSH         = 0x100,
    ILCLIENT_MARKED_BUFFER      = 0x200,
    ILCLIENT_BUFFER_MARK        = 0x400,
    ILCLIENT_CONFIG_CHANGED     = 0x800
} ILEVENT_MASK_T;

typedef enum {
    ILCLIENT_FLAGS_NONE            = 0x0,
    ILCLIENT_ENABLE_INPUT_BUFFERS  = 0x1,
    ILCLIENT_ENABLE_OUTPUT_BUFFERS = 0x2,
    ILCLIENT_DISABLE_ALL_PORTS     = 0x4,
    ILCLIENT_HOST_COMPONENT        = 0x8,
    ILCLIENT_OUTPUT_ZERO_BUFFERS   = 0x10                                         
} ILCLIENT_CREATE_FLAGS_T;

static VCOS_LOG_CAT_T ilclient_log_category;

typedef struct _ILEVENT_T ILEVENT_T;
struct _ILEVENT_T {
    OMX_EVENTTYPE eEvent;
    OMX_U32 nData1;
    OMX_U32 nData2;
    OMX_PTR pEventData;
    struct _ILEVENT_T *next;
};

#define NUM_EVENTS 100
struct _ILCLIENT_T {
    ILEVENT_T *event_list;
    VCOS_SEMAPHORE_T event_sema;
    ILEVENT_T event_rep[NUM_EVENTS];

    ILCLIENT_CALLBACK_T port_settings_callback;
    void *port_settings_callback_data;
    ILCLIENT_CALLBACK_T eos_callback;
    void *eos_callback_data;
    ILCLIENT_CALLBACK_T error_callback;
    void *error_callback_data;
    ILCLIENT_BUFFER_CALLBACK_T fill_buffer_done_callback;
    void *fill_buffer_done_callback_data;
    ILCLIENT_BUFFER_CALLBACK_T empty_buffer_done_callback;
    void *empty_buffer_done_callback_data;
    ILCLIENT_CALLBACK_T configchanged_callback;
    void *configchanged_callback_data;
};

struct _COMPONENT_T {
    OMX_HANDLETYPE comp;
    ILCLIENT_CREATE_FLAGS_T flags;
    VCOS_SEMAPHORE_T sema;
    VCOS_EVENT_FLAGS_T event;
    struct _COMPONENT_T *related;
    OMX_BUFFERHEADERTYPE *out_list;
    OMX_BUFFERHEADERTYPE *in_list;
    char name[32];
    char bufname[32];
    unsigned int error_mask;
    unsigned int priv;
    ILEVENT_T *list;
    ILCLIENT_T *client;
};

#define random_wait()
static char *states[] = {"Invalid", "Loaded", "Idle", "Executing", "Pause", "WaitingForResources"};

typedef enum {
    ILCLIENT_ERROR_UNPOPULATED  = 0x1,
    ILCLIENT_ERROR_SAMESTATE    = 0x2,
    ILCLIENT_ERROR_BADPARAMETER = 0x4
} ILERROR_MASK_T;

// static OMX_ERRORTYPE ilclient_empty_buffer_done(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
// static OMX_ERRORTYPE ilclient_empty_buffer_done_error(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
// static OMX_ERRORTYPE ilclient_fill_buffer_done(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);
// static OMX_ERRORTYPE ilclient_fill_buffer_done_error(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);
// static OMX_ERRORTYPE ilclient_event_handler(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_PTR pAppData, OMX_IN OMX_EVENTTYPE eEvent, OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, OMX_IN OMX_PTR pEventData);
// static void ilclient_lock_events(ILCLIENT_T *st);
// static void ilclient_unlock_events(ILCLIENT_T *st);

void ilclient_debug_output(char *format, ...) {
    va_list args;

    va_start(args, format);
    vcos_vlog_info(format, args);
    va_end(args);
}

static OMX_ERRORTYPE ilclient_empty_buffer_done(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer) {
    COMPONENT_T *st = (COMPONENT_T *) pAppData;
    OMX_BUFFERHEADERTYPE *list;

    ilclient_debug_output("%s: empty buffer done %p", st->name, pBuffer);

    vcos_semaphore_wait(&st->sema);
    // insert at end of the list, so we process buffers in
    // the same order
    list = st->in_list;
    while(list && list->pAppPrivate)
        list = list->pAppPrivate;

    if (!list)
        st->in_list = pBuffer;
    else
        list->pAppPrivate = pBuffer;

    pBuffer->pAppPrivate = NULL;
    vcos_semaphore_post(&st->sema);

    vcos_event_flags_set(&st->event, ILCLIENT_EMPTY_BUFFER_DONE, VCOS_OR);

    if (st->client->empty_buffer_done_callback)
        st->client->empty_buffer_done_callback(st->client->empty_buffer_done_callback_data, st);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE ilclient_empty_buffer_done_error(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_PTR pAppData, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer) {
    vc_assert(0);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE ilclient_fill_buffer_done(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData, OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer) {
    COMPONENT_T *st = (COMPONENT_T *) pAppData;
    OMX_BUFFERHEADERTYPE *list;

    ilclient_debug_output("%s: fill buffer done %p", st->name, pBuffer);

    vcos_semaphore_wait(&st->sema);
    // insert at end of the list, so we process buffers in
    // the correct order
    list = st->out_list;
    while(list && list->pAppPrivate)
        list = list->pAppPrivate;

    if (!list)
        st->out_list = pBuffer;
    else
        list->pAppPrivate = pBuffer;
        
    pBuffer->pAppPrivate = NULL;
    vcos_semaphore_post(&st->sema);

    vcos_event_flags_set(&st->event, ILCLIENT_FILL_BUFFER_DONE, VCOS_OR);

    if (st->client->fill_buffer_done_callback)
        st->client->fill_buffer_done_callback(st->client->fill_buffer_done_callback_data, st);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE ilclient_fill_buffer_done_error(OMX_OUT OMX_HANDLETYPE hComponent,
      OMX_OUT OMX_PTR pAppData,
      OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer) {
    vc_assert(0);
    return OMX_ErrorNone;
}

static void ilclient_lock_events(ILCLIENT_T *st) {
    vcos_semaphore_wait(&st->event_sema);
}

static void ilclient_unlock_events(ILCLIENT_T *st) {
    vcos_semaphore_post(&st->event_sema);
}

static OMX_ERRORTYPE ilclient_event_handler(OMX_IN OMX_HANDLETYPE hComponent,
                                            OMX_IN OMX_PTR pAppData,
                                            OMX_IN OMX_EVENTTYPE eEvent,
                                            OMX_IN OMX_U32 nData1,
                                            OMX_IN OMX_U32 nData2,
                                            OMX_IN OMX_PTR pEventData) {
    COMPONENT_T *st = (COMPONENT_T *) pAppData;
    ILEVENT_T *event;
    OMX_ERRORTYPE error = OMX_ErrorNone;

    ilclient_lock_events(st->client);

    // go through the events on this component and remove any duplicates in the
    // existing list, since the client probably doesn't need them.  it's better
    // than asserting when we run out.
    event = st->list;
    while(event != NULL) {
        ILEVENT_T **list = &(event->next);
        while(*list != NULL) {
            if ((*list)->eEvent == event->eEvent &&
                (*list)->nData1 == event->nData1 &&
                (*list)->nData2 == event->nData2) {
                // remove this duplicate
                ILEVENT_T *rem = *list;
                ilclient_debug_output("%s: removing %d/%d/%d", st->name, event->eEvent, event->nData1, event->nData2);            
                *list = rem->next;
                rem->eEvent = -1;
                rem->next = st->client->event_list;
                st->client->event_list = rem;
            }
            else
                list = &((*list)->next);
        }

        event = event->next;
    }

    vc_assert(st->client->event_list);
    event = st->client->event_list;

    switch (eEvent) {
    case OMX_EventCmdComplete:
        switch (nData1) {
        case OMX_CommandStateSet:
            ilclient_debug_output("%s: callback state changed (%s)", st->name, states[nData2]);
            vcos_event_flags_set(&st->event, ILCLIENT_STATE_CHANGED, VCOS_OR);
            break;
        case OMX_CommandPortDisable:
            ilclient_debug_output("%s: callback port disable %d", st->name, nData2);
            vcos_event_flags_set(&st->event, ILCLIENT_PORT_DISABLED, VCOS_OR);
            break;
        case OMX_CommandPortEnable:
            ilclient_debug_output("%s: callback port enable %d", st->name, nData2);
            vcos_event_flags_set(&st->event, ILCLIENT_PORT_ENABLED, VCOS_OR);
            break;
        case OMX_CommandFlush:
            ilclient_debug_output("%s: callback port flush %d", st->name, nData2);
            vcos_event_flags_set(&st->event, ILCLIENT_PORT_FLUSH, VCOS_OR);
            break;
        case OMX_CommandMarkBuffer:
            ilclient_debug_output("%s: callback mark buffer %d", st->name, nData2);
            vcos_event_flags_set(&st->event, ILCLIENT_MARKED_BUFFER, VCOS_OR);
            break;
        default:
            vc_assert(0);
        }
        break;
    case OMX_EventError: {
            // check if this component failed a command, and we have to notify another command
            // of this failure
            if (nData2 == 1 && st->related != NULL)
                vcos_event_flags_set(&st->related->event, ILCLIENT_EVENT_ERROR, VCOS_OR);

            error = nData1;
            switch (error) {
            case OMX_ErrorPortUnpopulated:
                if (st->error_mask & ILCLIENT_ERROR_UNPOPULATED) {
                    ilclient_debug_output("%s: ignore error: port unpopulated (%d)", st->name, nData2);
                    event = NULL;
                    break;
                }
                ilclient_debug_output("%s: port unpopulated %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorSameState:
                if (st->error_mask & ILCLIENT_ERROR_SAMESTATE) {
                    ilclient_debug_output("%s: ignore error: same state (%d)", st->name, nData2);
                    event = NULL;
                    break;
                }
                ilclient_debug_output("%s: same state %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorBadParameter:
                if (st->error_mask & ILCLIENT_ERROR_BADPARAMETER) {
                    ilclient_debug_output("%s: ignore error: bad parameter (%d)", st->name, nData2);
                    event = NULL;
                    break;
                }
                ilclient_debug_output("%s: bad parameter %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorIncorrectStateTransition:
                ilclient_debug_output("%s: incorrect state transition %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorBadPortIndex:
                ilclient_debug_output("%s: bad port index %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorStreamCorrupt:
                ilclient_debug_output("%s: stream corrupt %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorInsufficientResources:
                ilclient_debug_output("%s: insufficient resources %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorUnsupportedSetting:
                ilclient_debug_output("%s: unsupported setting %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorOverflow:
                ilclient_debug_output("%s: overflow %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorDiskFull:
                ilclient_debug_output("%s: disk full %x (%d)", st->name, error, nData2);
                //we do not set the error
                break;
            case OMX_ErrorMaxFileSize:
                ilclient_debug_output("%s: max file size %x (%d)", st->name, error, nData2);
                //we do not set the error
                break;
            case OMX_ErrorDrmUnauthorised:
                ilclient_debug_output("%s: drm file is unauthorised %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorDrmExpired:
                ilclient_debug_output("%s: drm file has expired %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            case OMX_ErrorDrmGeneral:
                ilclient_debug_output("%s: drm library error %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            default:
                vc_assert(0);
                ilclient_debug_output("%s: unexpected error %x (%d)", st->name, error, nData2);
                vcos_event_flags_set(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                break;
            }
        }
        break;
    case OMX_EventBufferFlag:
        ilclient_debug_output("%s: buffer flag %d/%x", st->name, nData1, nData2);
        if (nData2 & OMX_BUFFERFLAG_EOS) {
            vcos_event_flags_set(&st->event, ILCLIENT_BUFFER_FLAG_EOS, VCOS_OR);
            nData2 = OMX_BUFFERFLAG_EOS;
        }
        else
            vc_assert(0);
        break;
    case OMX_EventPortSettingsChanged:
        ilclient_debug_output("%s: port settings changed %d", st->name, nData1);
        vcos_event_flags_set(&st->event, ILCLIENT_PARAMETER_CHANGED, VCOS_OR);
        break;
    case OMX_EventMark:
        ilclient_debug_output("%s: buffer mark %p", st->name, pEventData);
        vcos_event_flags_set(&st->event, ILCLIENT_BUFFER_MARK, VCOS_OR);
        break;
    case OMX_EventParamOrConfigChanged:
        ilclient_debug_output("%s: param/config 0x%X on port %d changed", st->name, nData2, nData1);
        vcos_event_flags_set(&st->event, ILCLIENT_CONFIG_CHANGED, VCOS_OR);
        break;
    default:
        vc_assert(0);
        break;
    }

    if (event) {
        // fill in details
        event->eEvent = eEvent;
        event->nData1 = nData1;
        event->nData2 = nData2;
        event->pEventData = pEventData;

        // remove from top of spare list
        st->client->event_list = st->client->event_list->next;

        // put at head of component event queue
        event->next = st->list;
        st->list = event;
    }
    ilclient_unlock_events(st->client);

    // now call any callbacks without the event lock so the client can 
    // remove the event in context
    switch(eEvent) {
    case OMX_EventError:
        if (event && st->client->error_callback)
            st->client->error_callback(st->client->error_callback_data, st, error);
        break;
    case OMX_EventBufferFlag:
        if ((nData2 & OMX_BUFFERFLAG_EOS) && st->client->eos_callback)
            st->client->eos_callback(st->client->eos_callback_data, st, nData1);
        break;
    case OMX_EventPortSettingsChanged:
        if (st->client->port_settings_callback)
            st->client->port_settings_callback(st->client->port_settings_callback_data, st, nData1);
        break;
    case OMX_EventParamOrConfigChanged:
        if (st->client->configchanged_callback)
            st->client->configchanged_callback(st->client->configchanged_callback_data, st, nData2);
        break;
    default:
        // ignore
        break;
    }

    return OMX_ErrorNone;
}

OMX_HANDLETYPE ilclient_get_handle(COMPONENT_T *comp) {
    vcos_assert(comp);
    return comp->comp;
}


ILCLIENT_T *ilclient_init() {
    ILCLIENT_T *st = vcos_malloc(sizeof(ILCLIENT_T), "ilclient");
    int i;
    
    if (!st)
        return NULL;
    
    vcos_log_set_level(VCOS_LOG_CATEGORY, VCOS_LOG_WARN);
    vcos_log_register("ilclient", VCOS_LOG_CATEGORY);

    memset(st, 0, sizeof(ILCLIENT_T));

    i = vcos_semaphore_create(&st->event_sema, "il:event", 1);
    vc_assert(i == VCOS_SUCCESS);

    ilclient_lock_events(st);
    st->event_list = NULL;
    for (i=0; i<NUM_EVENTS; i++) {
        st->event_rep[i].eEvent = -1; // mark as unused
        st->event_rep[i].next = st->event_list;
        st->event_list = st->event_rep+i;
    }
    ilclient_unlock_events(st);
    return st;
}

void ilclient_destroy(ILCLIENT_T *st) {
    vcos_semaphore_delete(&st->event_sema);
    vcos_free(st);
    vcos_log_unregister(VCOS_LOG_CATEGORY);
}

void ilclient_set_port_settings_callback(ILCLIENT_T *st, ILCLIENT_CALLBACK_T func, void *userdata) {
    st->port_settings_callback = func;
    st->port_settings_callback_data = userdata;
}

void ilclient_set_eos_callback(ILCLIENT_T *st, ILCLIENT_CALLBACK_T func, void *userdata) {
    st->eos_callback = func;
    st->eos_callback_data = userdata;
}

void ilclient_set_error_callback(ILCLIENT_T *st, ILCLIENT_CALLBACK_T func, void *userdata) {
    st->error_callback = func;
    st->error_callback_data = userdata;
}

void ilclient_set_fill_buffer_done_callback(ILCLIENT_T *st, ILCLIENT_BUFFER_CALLBACK_T func, void *userdata) {
    st->fill_buffer_done_callback = func;
    st->fill_buffer_done_callback_data = userdata;
}

void ilclient_set_empty_buffer_done_callback(ILCLIENT_T *st, ILCLIENT_BUFFER_CALLBACK_T func, void *userdata) {
    st->empty_buffer_done_callback = func;
    st->empty_buffer_done_callback_data = userdata;
}

void ilclient_set_configchanged_callback(ILCLIENT_T *st, ILCLIENT_CALLBACK_T func, void *userdata) {
    st->configchanged_callback = func;
    st->configchanged_callback_data = userdata;
}

int ilclient_wait_for_command_complete_dual(COMPONENT_T *comp, OMX_COMMANDTYPE command, OMX_U32 nData2, COMPONENT_T *other) {
    OMX_U32 mask = ILCLIENT_EVENT_ERROR;
    int ret = 0;

    switch(command) {
        case OMX_CommandStateSet:    mask |= ILCLIENT_STATE_CHANGED; break;
        case OMX_CommandPortDisable: mask |= ILCLIENT_PORT_DISABLED; break;
        case OMX_CommandPortEnable:  mask |= ILCLIENT_PORT_ENABLED;  break;
        default: return -1;
    }

    if (other)
        other->related = comp;

    while(1) {
        ILEVENT_T *cur, *prev = NULL;
        VCOS_UNSIGNED set;

        ilclient_lock_events(comp->client);

        cur = comp->list;
        while(cur &&
                !(cur->eEvent == OMX_EventCmdComplete && cur->nData1 == command && cur->nData2 == nData2) &&
                !(cur->eEvent == OMX_EventError && cur->nData2 == 1)) {
            prev = cur;
            cur = cur->next;
        }

        if (cur) {
            if (prev == NULL)
                comp->list = cur->next;
            else
                prev->next = cur->next;

            // work out whether this was a success or a fail event
            ret = cur->eEvent == OMX_EventCmdComplete || cur->nData1 == OMX_ErrorSameState ? 0 : -1;

            if (cur->eEvent == OMX_EventError)
                vcos_event_flags_get(&comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR_CONSUME, 0, &set);

            // add back into spare list
            cur->next = comp->client->event_list;
            comp->client->event_list = cur;
            cur->eEvent = -1; // mark as unused
            
            ilclient_unlock_events(comp->client);
            break;
        }
        else if (other != NULL) {
            // check the other component for an error event that terminates a command
            cur = other->list;
            while(cur && !(cur->eEvent == OMX_EventError && cur->nData2 == 1))
                cur = cur->next;

            if (cur) {
                // we don't remove the event in this case, since the user
                // can confirm that this event errored by calling wait_for_command on the
                // other component

                ret = -2;
                ilclient_unlock_events(comp->client);
                break;
            }
        }

        ilclient_unlock_events(comp->client);

        vcos_event_flags_get(&comp->event, mask, VCOS_OR_CONSUME, VCOS_SUSPEND, &set);
    }

    if (other)
        other->related = NULL;

    return ret;
}

int ilclient_wait_for_command_complete(COMPONENT_T *comp, OMX_COMMANDTYPE command, OMX_U32 nData2) {
    return ilclient_wait_for_command_complete_dual(comp, command, nData2, NULL);
}


void ilclient_disable_port(COMPONENT_T *comp, int portIndex) {
    OMX_ERRORTYPE error;
    error = OMX_SendCommand(comp->comp, OMX_CommandPortDisable, portIndex, NULL);
    vc_assert(error == OMX_ErrorNone);
    if (ilclient_wait_for_command_complete(comp, OMX_CommandPortDisable, portIndex) < 0)
        vc_assert(0);
}

void ilclient_enable_port(COMPONENT_T *comp, int portIndex) {
    OMX_ERRORTYPE error;
    error = OMX_SendCommand(comp->comp, OMX_CommandPortEnable, portIndex, NULL);
    vc_assert(error == OMX_ErrorNone);
    if (ilclient_wait_for_command_complete(comp, OMX_CommandPortEnable, portIndex) < 0)
        vc_assert(0);
}

int ilclient_create_component(ILCLIENT_T *client, COMPONENT_T **comp, char *name, ILCLIENT_CREATE_FLAGS_T flags) {
    OMX_CALLBACKTYPE callbacks;
    OMX_ERRORTYPE error;
    char component_name[128];
    int32_t status;

    *comp = vcos_malloc(sizeof(COMPONENT_T), "il:comp");
    if (!*comp)
        return -1;

    memset(*comp, 0, sizeof(COMPONENT_T));

#define COMP_PREFIX "OMX.broadcom."

    status = vcos_event_flags_create(&(*comp)->event,"il:comp");
    vc_assert(status == VCOS_SUCCESS);
    status = vcos_semaphore_create(&(*comp)->sema, "il:comp", 1);
    vc_assert(status == VCOS_SUCCESS);
    (*comp)->client = client;

    vcos_snprintf((*comp)->name, sizeof((*comp)->name), "cl:%s", name);
    vcos_snprintf((*comp)->bufname, sizeof((*comp)->bufname), "cl:%s buffer", name);
    vcos_snprintf(component_name, sizeof(component_name), "%s%s", COMP_PREFIX, name);

    (*comp)->flags = flags;

    callbacks.EventHandler = ilclient_event_handler;
    callbacks.EmptyBufferDone = flags & ILCLIENT_ENABLE_INPUT_BUFFERS ? ilclient_empty_buffer_done : ilclient_empty_buffer_done_error;
    callbacks.FillBufferDone = flags & ILCLIENT_ENABLE_OUTPUT_BUFFERS ? ilclient_fill_buffer_done : ilclient_fill_buffer_done_error;

    error = OMX_GetHandle(&(*comp)->comp, component_name, *comp, &callbacks);

    if (error == OMX_ErrorNone) {
        OMX_UUIDTYPE uid;
        char name[128];
        OMX_VERSIONTYPE compVersion, specVersion;

        if (OMX_GetComponentVersion((*comp)->comp, name, &compVersion, &specVersion, &uid) == OMX_ErrorNone) {
            char *p = (char *) uid + strlen(COMP_PREFIX);

            vcos_snprintf((*comp)->name, sizeof((*comp)->name), "cl:%s", p);
            (*comp)->name[sizeof((*comp)->name)-1] = 0;
            vcos_snprintf((*comp)->bufname, sizeof((*comp)->bufname), "cl:%s buffer", p);
            (*comp)->bufname[sizeof((*comp)->bufname)-1] = 0;
        }

        if (flags & (ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_OUTPUT_ZERO_BUFFERS)) {
            OMX_PORT_PARAM_TYPE ports;
            OMX_INDEXTYPE types[] = {OMX_IndexParamAudioInit, OMX_IndexParamVideoInit, OMX_IndexParamImageInit, OMX_IndexParamOtherInit};
            int i;

            ports.nSize = sizeof(OMX_PORT_PARAM_TYPE);
            ports.nVersion.nVersion = OMX_VERSION;

            for(i=0; i<4; i++) {
                OMX_ERRORTYPE error = OMX_GetParameter((*comp)->comp, types[i], &ports);
                if (error == OMX_ErrorNone) {
                    uint32_t j;
                    for(j=0; j<ports.nPorts; j++) {
                        if (flags & ILCLIENT_DISABLE_ALL_PORTS)
                            ilclient_disable_port(*comp, ports.nStartPortNumber+j);
                        
                        if (flags & ILCLIENT_OUTPUT_ZERO_BUFFERS) {
                            OMX_PARAM_PORTDEFINITIONTYPE portdef;
                            portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
                            portdef.nVersion.nVersion = OMX_VERSION;
                            portdef.nPortIndex = ports.nStartPortNumber+j;
                            if (OMX_GetParameter((*comp)->comp, OMX_IndexParamPortDefinition, &portdef) == OMX_ErrorNone) {
                                if (portdef.eDir == OMX_DirOutput && portdef.nBufferCountActual > 0) {
                                    portdef.nBufferCountActual = 0;
                                    OMX_SetParameter((*comp)->comp, OMX_IndexParamPortDefinition, &portdef);
                                }
                            }
                        }
                    }
                }
            }
        }
        return 0;
    }
    else {
        vcos_event_flags_delete(&(*comp)->event);
        vcos_semaphore_delete(&(*comp)->sema);
        vcos_free(*comp);
        *comp = NULL;
        return -1;
    }
}

int ilclient_remove_event(COMPONENT_T *st, OMX_EVENTTYPE eEvent, OMX_U32 nData1, int ignore1, OMX_IN OMX_U32 nData2, int ignore2) {
    ILEVENT_T *cur, *prev;
    uint32_t set;
    ilclient_lock_events(st->client);

    cur = st->list;
    prev = NULL;

    while (cur && !(cur->eEvent == eEvent && (ignore1 || cur->nData1 == nData1) && (ignore2 || cur->nData2 == nData2))) {
        prev = cur;
        cur = cur->next;
    }

    if (cur == NULL) {
        ilclient_unlock_events(st->client);
        return -1;
    }

    if (prev == NULL)
        st->list = cur->next;
    else
        prev->next = cur->next;

    // add back into spare list
    cur->next = st->client->event_list;
    st->client->event_list = cur;
    cur->eEvent = -1; // mark as unused

    // if we're removing an OMX_EventError or OMX_EventParamOrConfigChanged event, then clear the error bit from the eventgroup,
    // since the user might have been notified through the error callback, and then 
    // can't clear the event bit - this will then cause problems the next time they
    // wait for an error.
    if (eEvent == OMX_EventError)
        vcos_event_flags_get(&st->event, ILCLIENT_EVENT_ERROR, VCOS_OR_CONSUME, 0, &set);
    else if (eEvent == OMX_EventParamOrConfigChanged)
        vcos_event_flags_get(&st->event, ILCLIENT_CONFIG_CHANGED, VCOS_OR_CONSUME, 0, &set);

    ilclient_unlock_events(st->client);
    return 0;
}

/***********************************************************
 * Name: ilclient_state_transition
 *
 * Description: Transitions a null terminated list of IL components to
 * a given state.  All components are told to transition in a random
 * order before any are checked for transition completion.
 *
 * Returns: void
 ***********************************************************/
void ilclient_state_transition(COMPONENT_T *list[], OMX_STATETYPE state) {
    OMX_ERRORTYPE error;
    int i, num;
    uint32_t set;

    num=0;
    while (list[num])
        num++;

    // if we transition the supplier port first, it will call freebuffer on the non
    // supplier, which will correctly signal a port unpopulated error.  We want to
    // ignore these errors.
    if (state == OMX_StateLoaded)
        for (i=0; i<num; i++)
            list[i]->error_mask |= ILCLIENT_ERROR_UNPOPULATED;
    for (i=0; i<num; i++)
        list[i]->priv = ((rand() >> 13) & 0xff)+1;

    for (i=0; i<num; i++) {
        // transition the components in a random order
        int j, min = -1;
        for (j=0; j<num; j++)
            if (list[j]->priv && (min == -1 || list[min]->priv > list[j]->priv))
                min = j;

        list[min]->priv = 0;

        random_wait();
        //Clear error event for this component
        vcos_event_flags_get(&list[min]->event, ILCLIENT_EVENT_ERROR, VCOS_OR_CONSUME, 0, &set);

        error = OMX_SendCommand(list[min]->comp, OMX_CommandStateSet, state, NULL);
        vc_assert(error == OMX_ErrorNone);
    }

    random_wait();

    for (i=0; i<num; i++)
        if (ilclient_wait_for_command_complete(list[i], OMX_CommandStateSet, state) < 0)
            vc_assert(0);

    if (state == OMX_StateLoaded)
        for (i=0; i<num; i++)
            list[i]->error_mask &= ~ILCLIENT_ERROR_UNPOPULATED;
}

void ilclient_teardown_tunnels(TUNNEL_T *tunnel) {
    int i;
    OMX_ERRORTYPE error;

    i=0;;
    while (tunnel[i].source) {
        error = OMX_SetupTunnel(tunnel[i].source->comp, tunnel[i].source_port, NULL, 0);
        vc_assert(error == OMX_ErrorNone);

        error = OMX_SetupTunnel(tunnel[i].sink->comp, tunnel[i].sink_port, NULL, 0);
        vc_assert(error == OMX_ErrorNone);
        i++;
    }
}

void ilclient_disable_tunnel(TUNNEL_T *tunnel) {
    OMX_ERRORTYPE error;
    
    if (tunnel->source == 0 || tunnel->sink == 0)
        return;

    tunnel->source->error_mask |= ILCLIENT_ERROR_UNPOPULATED;
    tunnel->sink->error_mask |= ILCLIENT_ERROR_UNPOPULATED;

    error = OMX_SendCommand(tunnel->source->comp, OMX_CommandPortDisable, tunnel->source_port, NULL);
    vc_assert(error == OMX_ErrorNone);

    error = OMX_SendCommand(tunnel->sink->comp, OMX_CommandPortDisable, tunnel->sink_port, NULL);
    vc_assert(error == OMX_ErrorNone);

    if (ilclient_wait_for_command_complete(tunnel->source, OMX_CommandPortDisable, tunnel->source_port) < 0)
        vc_assert(0);

    if (ilclient_wait_for_command_complete(tunnel->sink, OMX_CommandPortDisable, tunnel->sink_port) < 0)
        vc_assert(0);

    tunnel->source->error_mask &= ~ILCLIENT_ERROR_UNPOPULATED;
    tunnel->sink->error_mask &= ~ILCLIENT_ERROR_UNPOPULATED;
}

int ilclient_wait_for_event(COMPONENT_T *comp, OMX_EVENTTYPE event,
                            OMX_U32 nData1, int ignore1, OMX_IN OMX_U32 nData2, int ignore2,
                            int event_flag, int suspend) {
    int32_t status;
    uint32_t set;

    while (ilclient_remove_event(comp, event, nData1, ignore1, nData2, ignore2) < 0) {
        // if we want to be notified of errors, check the list for an error now
        // before blocking, the event flag may have been cleared already.
        if (event_flag & ILCLIENT_EVENT_ERROR) {
            ILEVENT_T *cur;
            ilclient_lock_events(comp->client);
            cur = comp->list;
            while(cur && cur->eEvent != OMX_EventError)            
                cur = cur->next;
            
            if (cur) {
                // clear error flag
                vcos_event_flags_get(&comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR_CONSUME, 0, &set);
                ilclient_unlock_events(comp->client);
                return -2;
            }

            ilclient_unlock_events(comp->client);
        }

        // check for config change event if we are asked to be notified of that
        if (event_flag & ILCLIENT_CONFIG_CHANGED) {
            ILEVENT_T *cur;
            ilclient_lock_events(comp->client);
            cur = comp->list;
            while(cur && cur->eEvent != OMX_EventParamOrConfigChanged)
                cur = cur->next;
            
            ilclient_unlock_events(comp->client);

            if (cur)
                return ilclient_remove_event(comp, event, nData1, ignore1, nData2, ignore2) == 0 ? 0 : -3;
        }

        status = vcos_event_flags_get(&comp->event, event_flag, VCOS_OR_CONSUME, 
                                        suspend, &set);
        if (status != 0)
            return -1;
        if (set & ILCLIENT_EVENT_ERROR)
            return -2;
        if (set & ILCLIENT_CONFIG_CHANGED)
            return ilclient_remove_event(comp, event, nData1, ignore1, nData2, ignore2) == 0 ? 0 : -3;
    }

    return 0;
}

int ilclient_enable_tunnel(TUNNEL_T *tunnel) {
    OMX_STATETYPE state;
    OMX_ERRORTYPE error;

    ilclient_debug_output("ilclient: enable tunnel from %x/%d to %x/%d",
                            tunnel->source, tunnel->source_port,
                            tunnel->sink, tunnel->sink_port);

    error = OMX_SendCommand(tunnel->source->comp, OMX_CommandPortEnable, tunnel->source_port, NULL);
    vc_assert(error == OMX_ErrorNone);

    error = OMX_SendCommand(tunnel->sink->comp, OMX_CommandPortEnable, tunnel->sink_port, NULL);
    vc_assert(error == OMX_ErrorNone);

    // to complete, the sink component can't be in loaded state
    error = OMX_GetState(tunnel->sink->comp, &state);
    vc_assert(error == OMX_ErrorNone);
    if (state == OMX_StateLoaded) {
        int ret = 0;

        if (ilclient_wait_for_command_complete(tunnel->sink, OMX_CommandPortEnable, tunnel->sink_port) != 0 ||
            OMX_SendCommand(tunnel->sink->comp, OMX_CommandStateSet, OMX_StateIdle, NULL) != OMX_ErrorNone ||
            (ret = ilclient_wait_for_command_complete_dual(tunnel->sink, OMX_CommandStateSet, OMX_StateIdle, tunnel->source)) < 0) {
            if (ret == -2) {
                // the error was reported fom the source component: clear this error and disable the sink component
                ilclient_wait_for_command_complete(tunnel->source, OMX_CommandPortEnable, tunnel->source_port);
                ilclient_disable_port(tunnel->sink, tunnel->sink_port);
            }

            ilclient_debug_output("ilclient: could not change component state to IDLE");
            ilclient_disable_port(tunnel->source, tunnel->source_port);
            return -1;
        }
    }
    else {
        if (ilclient_wait_for_command_complete(tunnel->sink, OMX_CommandPortEnable, tunnel->sink_port) != 0) {
            ilclient_debug_output("ilclient: could not change sink port %d to enabled", tunnel->sink_port);

            //Oops failed to enable the sink port
            ilclient_disable_port(tunnel->source, tunnel->source_port);
            //Clean up the port enable event from the source port.
            ilclient_wait_for_event(tunnel->source, OMX_EventCmdComplete,
                                    OMX_CommandPortEnable, 0, tunnel->source_port, 0,
                                    ILCLIENT_PORT_ENABLED | ILCLIENT_EVENT_ERROR, VCOS_EVENT_FLAGS_SUSPEND);
            return -1;
        }
    }

    if (ilclient_wait_for_command_complete(tunnel->source, OMX_CommandPortEnable, tunnel->source_port) != 0) {
        ilclient_debug_output("ilclient: could not change source port %d to enabled", tunnel->source_port);

        //Failed to enable the source port
        ilclient_disable_port(tunnel->sink, tunnel->sink_port);
        return -1;
    }

    return 0;
}

void ilclient_flush_tunnels(TUNNEL_T *tunnel, int max) {
    OMX_ERRORTYPE error;
    int i;

    i=0;
    while (tunnel[i].source && (max == 0 || i < max)) {
        error = OMX_SendCommand(tunnel[i].source->comp, OMX_CommandFlush, tunnel[i].source_port, NULL);
        vc_assert(error == OMX_ErrorNone);

        error = OMX_SendCommand(tunnel[i].sink->comp, OMX_CommandFlush, tunnel[i].sink_port, NULL);
        vc_assert(error == OMX_ErrorNone);

        ilclient_wait_for_event(tunnel[i].source, OMX_EventCmdComplete,
                                OMX_CommandFlush, 0, tunnel[i].source_port, 0,
                                ILCLIENT_PORT_FLUSH, VCOS_EVENT_FLAGS_SUSPEND);
        ilclient_wait_for_event(tunnel[i].sink, OMX_EventCmdComplete,
                                OMX_CommandFlush, 0, tunnel[i].sink_port, 0,
                                ILCLIENT_PORT_FLUSH, VCOS_EVENT_FLAGS_SUSPEND);
        i++;
    }
}

void ilclient_return_events(COMPONENT_T *comp) {
    ilclient_lock_events(comp->client);
    while (comp->list) {
        ILEVENT_T *next = comp->list->next;
        comp->list->next = comp->client->event_list;
        comp->client->event_list = comp->list;
        comp->list = next;
    }
    ilclient_unlock_events(comp->client);
}

void ilclient_cleanup_components(COMPONENT_T *list[]) {
    int i;
    OMX_ERRORTYPE error;

    i=0;
    while (list[i]) {
        ilclient_return_events(list[i]);
        if (list[i]->comp) {
            error = OMX_FreeHandle(list[i]->comp);
            vc_assert(error == OMX_ErrorNone);
        }
        i++;
    }

    i=0;
    while (list[i]) {
        vcos_event_flags_delete(&list[i]->event);
        vcos_semaphore_delete(&list[i]->sema);
        vcos_free(list[i]);
        list[i] = NULL;
        i++;
    }
}

int ilclient_change_component_state(COMPONENT_T *comp, OMX_STATETYPE state) {
    OMX_ERRORTYPE error;
    error = OMX_SendCommand(comp->comp, OMX_CommandStateSet, state, NULL);
    vc_assert(error == OMX_ErrorNone);
    if (ilclient_wait_for_command_complete(comp, OMX_CommandStateSet, state) < 0) {
        ilclient_debug_output("ilclient: could not change component state to %d", state);
        ilclient_remove_event(comp, OMX_EventError, 0, 1, 0, 1);
        return -1;
    }
    return 0;
}

void ilclient_disable_port_buffers(COMPONENT_T *comp, int portIndex, OMX_BUFFERHEADERTYPE *bufferList, ILCLIENT_FREE_T ilclient_free, void *priv) {
    OMX_ERRORTYPE error;
    OMX_BUFFERHEADERTYPE *list = bufferList;
    OMX_BUFFERHEADERTYPE **head, *clist, *prev;
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    int num;

    // get the buffers off the relevant queue
    memset(&portdef, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = portIndex;
    
    // work out buffer requirements, check port is in the right state
    error = OMX_GetParameter(comp->comp, OMX_IndexParamPortDefinition, &portdef);
    if (error != OMX_ErrorNone || portdef.bEnabled != OMX_TRUE || portdef.nBufferCountActual == 0 || portdef.nBufferSize == 0)
        return;
    
    num = portdef.nBufferCountActual;
    
    error = OMX_SendCommand(comp->comp, OMX_CommandPortDisable, portIndex, NULL);
    vc_assert(error == OMX_ErrorNone);
        
    while(num > 0) {
        VCOS_UNSIGNED set;

        if (list == NULL) {
            vcos_semaphore_wait(&comp->sema);
            
            // take buffers for this port off the relevant queue
            head = portdef.eDir == OMX_DirInput ? &comp->in_list : &comp->out_list;
            clist = *head;
            prev = NULL;
            
            while(clist) {
                if ((portdef.eDir == OMX_DirInput ? clist->nInputPortIndex : clist->nOutputPortIndex) == portIndex) {
                    OMX_BUFFERHEADERTYPE *pBuffer = clist;
                    
                    if (!prev)
                        clist = *head = (OMX_BUFFERHEADERTYPE *) pBuffer->pAppPrivate;
                    else
                        clist = prev->pAppPrivate = (OMX_BUFFERHEADERTYPE *) pBuffer->pAppPrivate;
                    
                    pBuffer->pAppPrivate = list;
                    list = pBuffer;
                }
                else {
                    prev = clist;
                    clist = (OMX_BUFFERHEADERTYPE *) &(clist->pAppPrivate);
                }
            }
            
            vcos_semaphore_post(&comp->sema);
        }

        while(list) {
            void *buf = list->pBuffer;
            OMX_BUFFERHEADERTYPE *next = list->pAppPrivate;
            
            error = OMX_FreeBuffer(comp->comp, portIndex, list);
            vc_assert(error == OMX_ErrorNone);
            
            if (ilclient_free)
                ilclient_free(priv, buf);
            else
                vcos_free(buf);
            
            num--;
            list = next;
        }

        if (num) {
            OMX_U32 mask = ILCLIENT_PORT_DISABLED | ILCLIENT_EVENT_ERROR;
            mask |= (portdef.eDir == OMX_DirInput ? ILCLIENT_EMPTY_BUFFER_DONE : ILCLIENT_FILL_BUFFER_DONE);

            // also wait for command complete/error in case we didn't have all the buffers allocated
            vcos_event_flags_get(&comp->event, mask, VCOS_OR_CONSUME, -1, &set);

            if ((set & ILCLIENT_EVENT_ERROR) && ilclient_remove_event(comp, OMX_EventError, 0, 1, 1, 0) >= 0)
                return;

            if ((set & ILCLIENT_PORT_DISABLED) && ilclient_remove_event(comp, OMX_EventCmdComplete, OMX_CommandPortDisable, 0, portIndex, 0) >= 0)
                return;
        }            
    }
    
    if (ilclient_wait_for_command_complete(comp, OMX_CommandPortDisable, portIndex) < 0)
        vc_assert(0);
}


int ilclient_enable_port_buffers(COMPONENT_T *comp, int portIndex, ILCLIENT_MALLOC_T ilclient_malloc, ILCLIENT_FREE_T ilclient_free, void *priv) {
    OMX_ERRORTYPE error;
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    OMX_BUFFERHEADERTYPE *list = NULL, **end = &list;
    OMX_STATETYPE state;
    int i;

    memset(&portdef, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = portIndex;
    
    // work out buffer requirements, check port is in the right state
    error = OMX_GetParameter(comp->comp, OMX_IndexParamPortDefinition, &portdef);
    if (error != OMX_ErrorNone || portdef.bEnabled != OMX_FALSE || portdef.nBufferCountActual == 0 || portdef.nBufferSize == 0)
        return -1;

    // check component is in the right state to accept buffers
    error = OMX_GetState(comp->comp, &state);
    if (error != OMX_ErrorNone || !(state == OMX_StateIdle || state == OMX_StateExecuting || state == OMX_StatePause))
        return -1;

    // send the command
    error = OMX_SendCommand(comp->comp, OMX_CommandPortEnable, portIndex, NULL);
    vc_assert(error == OMX_ErrorNone);

    for (i=0; i != portdef.nBufferCountActual; i++) {
        unsigned char *buf;
        if (ilclient_malloc)
            buf = ilclient_malloc(priv, portdef.nBufferSize, portdef.nBufferAlignment, comp->bufname);
        else
            buf = vcos_malloc_aligned(portdef.nBufferSize, portdef.nBufferAlignment, comp->bufname);

        if (!buf)
            break;

        error = OMX_UseBuffer(comp->comp, end, portIndex, NULL, portdef.nBufferSize, buf);
        if (error != OMX_ErrorNone) {
            if (ilclient_free)
                ilclient_free(priv, buf);
            else
                vcos_free(buf);

            break;
        }
        end = (OMX_BUFFERHEADERTYPE **) &((*end)->pAppPrivate);
    }

    // queue these buffers
    vcos_semaphore_wait(&comp->sema);

    if (portdef.eDir == OMX_DirInput) {
        *end = comp->in_list;
        comp->in_list = list;
    }
    else {
        *end = comp->out_list;
        comp->out_list = list;
    }

    vcos_semaphore_post(&comp->sema);

    if (i != portdef.nBufferCountActual ||
        ilclient_wait_for_command_complete(comp, OMX_CommandPortEnable, portIndex) < 0) {
        ilclient_disable_port_buffers(comp, portIndex, NULL, ilclient_free, priv);

        // at this point the first command might have terminated with an error, which means that
        // the port is disabled before the disable_port_buffers function is called, so we're left
        // with the error bit set and an error event in the queue.  Clear these now if they exist.
        ilclient_remove_event(comp, OMX_EventError, 0, 1, 1, 0);

        return -1;
    }

    // success
    return 0;
}

int ilclient_setup_tunnel(TUNNEL_T *tunnel, unsigned int portStream, int timeout) {
    OMX_ERRORTYPE error;
    OMX_PARAM_U32TYPE param;
    OMX_STATETYPE state;
    int32_t status;
    int enable_error;

    // source component must at least be idle, not loaded
    error = OMX_GetState(tunnel->source->comp, &state);
    vc_assert(error == OMX_ErrorNone);
    if (state == OMX_StateLoaded && ilclient_change_component_state(tunnel->source, OMX_StateIdle) < 0)
        return -2;

    // wait for the port parameter changed from the source port
    if (timeout) {
        status = ilclient_wait_for_event(tunnel->source, OMX_EventPortSettingsChanged,
                                        tunnel->source_port, 0, -1, 1,
                                        ILCLIENT_PARAMETER_CHANGED | ILCLIENT_EVENT_ERROR, timeout);
        
        if (status < 0) {
            ilclient_debug_output(
                "ilclient: timed out waiting for port settings changed on port %d", tunnel->source_port);
            return status;
        }
    }

    // disable ports
    ilclient_disable_tunnel(tunnel);

    // if this source port uses port streams, we need to select one of them before proceeding
    // if getparameter causes an error that's fine, nothing needs selecting
    param.nSize = sizeof(OMX_PARAM_U32TYPE);
    param.nVersion.nVersion = OMX_VERSION;
    param.nPortIndex = tunnel->source_port;
    if (OMX_GetParameter(tunnel->source->comp, OMX_IndexParamNumAvailableStreams, &param) == OMX_ErrorNone) {
        if (param.nU32 == 0) {
            // no streams available
            // leave the source port disabled, and return a failure
            return -3;
        }
        if (param.nU32 <= portStream) {
            // requested stream not available
            // no streams available
            // leave the source port disabled, and return a failure
            return -4;
        }

        param.nU32 = portStream;
        error = OMX_SetParameter(tunnel->source->comp, OMX_IndexParamActiveStream, &param);
        vc_assert(error == OMX_ErrorNone);
    }

    // now create the tunnel
    error = OMX_SetupTunnel(tunnel->source->comp, tunnel->source_port, tunnel->sink->comp, tunnel->sink_port);

    enable_error = 0;

    if (error != OMX_ErrorNone || (enable_error=ilclient_enable_tunnel(tunnel)) < 0) {
        // probably format not compatible
        error = OMX_SetupTunnel(tunnel->source->comp, tunnel->source_port, NULL, 0);
        vc_assert(error == OMX_ErrorNone);
        error = OMX_SetupTunnel(tunnel->sink->comp, tunnel->sink_port, NULL, 0);
        vc_assert(error == OMX_ErrorNone);
        
        if (enable_error) {
            //Clean up the errors. This does risk removing an error that was nothing to do with this tunnel :-/
            ilclient_remove_event(tunnel->sink, OMX_EventError, 0, 1, 0, 1);
            ilclient_remove_event(tunnel->source, OMX_EventError, 0, 1, 0, 1);
        }

        ilclient_debug_output("ilclient: could not setup/enable tunnel (setup=0x%x,enable=%d)",
                                error, enable_error);
        return -5;
    }

    return 0;
}

OMX_BUFFERHEADERTYPE *ilclient_get_output_buffer(COMPONENT_T *comp, int portIndex, int block) {
    OMX_BUFFERHEADERTYPE *ret = NULL, *prev = NULL;
    VCOS_UNSIGNED set;

    do {
        vcos_semaphore_wait(&comp->sema);
        ret = comp->out_list;
        while(ret != NULL && ret->nOutputPortIndex != portIndex) {
            prev = ret;
            ret = ret->pAppPrivate;
        }
        
        if (ret) {
            if (prev == NULL)
                comp->out_list = ret->pAppPrivate;
            else
                prev->pAppPrivate = ret->pAppPrivate;
            
            ret->pAppPrivate = NULL;
        }
        vcos_semaphore_post(&comp->sema);

        if (block && !ret)
            vcos_event_flags_get(&comp->event, ILCLIENT_FILL_BUFFER_DONE, VCOS_OR_CONSUME, -1, &set);

    } while(block && !ret);

    return ret;
}

OMX_BUFFERHEADERTYPE *ilclient_get_input_buffer(COMPONENT_T *comp, int portIndex, int block) {
    OMX_BUFFERHEADERTYPE *ret = NULL, *prev = NULL;

    do {
        VCOS_UNSIGNED set;

        vcos_semaphore_wait(&comp->sema);
        ret = comp->in_list;
        while(ret != NULL && ret->nInputPortIndex != portIndex) {
            prev = ret;
            ret = ret->pAppPrivate;
        }
        
        if (ret) {
            if (prev == NULL)
                comp->in_list = ret->pAppPrivate;
            else
                prev->pAppPrivate = ret->pAppPrivate;
            
            ret->pAppPrivate = NULL;
        }
        vcos_semaphore_post(&comp->sema);

        if (block && !ret)
            vcos_event_flags_get(&comp->event, ILCLIENT_EMPTY_BUFFER_DONE, VCOS_OR_CONSUME, -1, &set);

    } while(block && !ret);

    return ret;
}

static struct {
    OMX_PORTDOMAINTYPE dom;
    int param;
} port_types[] = {
    { OMX_PortDomainVideo, OMX_IndexParamVideoInit },
    { OMX_PortDomainAudio, OMX_IndexParamAudioInit },
    { OMX_PortDomainImage, OMX_IndexParamImageInit },
    { OMX_PortDomainOther, OMX_IndexParamOtherInit },
};

int ilclient_get_port_index(COMPONENT_T *comp, OMX_DIRTYPE dir, OMX_PORTDOMAINTYPE type, int index) {
    uint32_t i;
    // for each possible port type...
    for (i=0; i<sizeof(port_types)/sizeof(port_types[0]); i++) {
        if ((port_types[i].dom == type) || (type == (OMX_PORTDOMAINTYPE) -1)) {
            OMX_PORT_PARAM_TYPE param;
            OMX_ERRORTYPE error;
            uint32_t j;

            param.nSize = sizeof(param);
            param.nVersion.nVersion = OMX_VERSION;
            error = OMX_GetParameter(ILC_GET_HANDLE(comp), port_types[i].param, &param);
            assert(error == OMX_ErrorNone);

            // for each port of this type...
            for (j=0; j<param.nPorts; j++) {
                int port = param.nStartPortNumber+j;

                OMX_PARAM_PORTDEFINITIONTYPE portdef;
                portdef.nSize = sizeof(portdef);
                portdef.nVersion.nVersion = OMX_VERSION;
                portdef.nPortIndex = port;

                error = OMX_GetParameter(ILC_GET_HANDLE(comp), OMX_IndexParamPortDefinition, &portdef);
                assert(error == OMX_ErrorNone);

                if (portdef.eDir == dir) {
                    if (index-- == 0)
                        return port;
                }
            }
        }
    }
    return -1;
}

int ilclient_suggest_bufsize(COMPONENT_T *comp, OMX_U32 nBufSizeHint) {
    OMX_PARAM_BRCMOUTPUTBUFFERSIZETYPE param;
    OMX_ERRORTYPE error;

    param.nSize = sizeof(param);
    param.nVersion.nVersion = OMX_VERSION;
    param.nBufferSize = nBufSizeHint;
    error = OMX_SetParameter(ILC_GET_HANDLE(comp), OMX_IndexParamBrcmOutputBufferSize,
                                &param);
    assert(error == OMX_ErrorNone);

    return (error == OMX_ErrorNone) ? 0 : -1;
}

unsigned int ilclient_stack_size(void) {
    return ILCLIENT_THREAD_DEFAULT_STACK_SIZE;
}

// --------------------------------- 

static int coreInit = 0;
static int nActiveHandles = 0;
static ILCS_SERVICE_T *ilcs_service = NULL;
static VCOS_MUTEX_T lock;
static VCOS_ONCE_T once = VCOS_ONCE_INIT;

/* Atomic creation of lock protecting shared state */
static void initOnce(void) {
    VCOS_STATUS_T status;
    status = vcos_mutex_create(&lock, VCOS_FUNCTION);
    vcos_demand(status == VCOS_SUCCESS);
}

/* OMX_Init */
OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void) {
    VCOS_STATUS_T status;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    status = vcos_once(&once, initOnce);
    vcos_demand(status == VCOS_SUCCESS);

    vcos_mutex_lock(&lock);
    
    if(coreInit == 0) {
        // we need to connect via an ILCS connection to VideoCore
        VCHI_INSTANCE_T initialise_instance;
        VCHI_CONNECTION_T *connection;
        ILCS_CONFIG_T config;

        vc_host_get_vchi_state(&initialise_instance, &connection);

        vcilcs_config(&config);

        ilcs_service = ilcs_init((VCHIQ_INSTANCE_T) initialise_instance, (void **) &connection, &config, 0);

        if(ilcs_service == NULL) {
            err = OMX_ErrorHardware;
            goto end;
        }

        coreInit = 1;
    }
    else
        coreInit++;

end:
    vcos_mutex_unlock(&lock);
    return err;
}

/* OMX_Deinit */
OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void) {
    if(coreInit == 0) // || (coreInit == 1 && nActiveHandles > 0))
        return OMX_ErrorNotReady;

    vcos_mutex_lock(&lock);

    coreInit--;

    if(coreInit == 0) {
        // we need to teardown the ILCS connection to VideoCore
        ilcs_deinit(ilcs_service);
        ilcs_service = NULL;
    }

    vcos_mutex_unlock(&lock);
    
    return OMX_ErrorNone;
}


/* OMX_ComponentNameEnum */
OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(OMX_OUT OMX_STRING cComponentName, OMX_IN  OMX_U32 nNameLength, OMX_IN  OMX_U32 nIndex) {
    if(ilcs_service == NULL)
        return OMX_ErrorBadParameter;

    return vcil_out_component_name_enum(ilcs_get_common(ilcs_service), cComponentName, nNameLength, nIndex);
}


/* OMX_GetHandle */
OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_STRING cComponentName, OMX_IN  OMX_PTR pAppData, OMX_IN  OMX_CALLBACKTYPE* pCallBacks) {
    OMX_ERRORTYPE eError;
    OMX_COMPONENTTYPE *pComp;
    OMX_HANDLETYPE hHandle = 0;

    if (pHandle == NULL || cComponentName == NULL || pCallBacks == NULL || ilcs_service == NULL) {
        if(pHandle)
            *pHandle = NULL;
        return OMX_ErrorBadParameter;
    }

    {
        pComp = (OMX_COMPONENTTYPE *)malloc(sizeof(OMX_COMPONENTTYPE));
        if (!pComp) {
            vcos_assert(0);
            return OMX_ErrorInsufficientResources;
        }
        memset(pComp, 0, sizeof(OMX_COMPONENTTYPE));
        hHandle = (OMX_HANDLETYPE)pComp;
        pComp->nSize = sizeof(OMX_COMPONENTTYPE);
        pComp->nVersion.nVersion = OMX_VERSION;
        eError = vcil_out_create_component(ilcs_get_common(ilcs_service), hHandle, cComponentName);

        if (eError == OMX_ErrorNone) {
            // Check that all function pointers have been filled in.
            // All fields should be non-zero.
            int i;
            uint32_t *p = (uint32_t *) pComp;
            for(i=0; i<sizeof(OMX_COMPONENTTYPE)>>2; i++)
                if(*p++ == 0)
                eError = OMX_ErrorInvalidComponent;

            if(eError != OMX_ErrorNone && pComp->ComponentDeInit)
                pComp->ComponentDeInit(hHandle);
        }      

        if (eError == OMX_ErrorNone) {
            eError = pComp->SetCallbacks(hHandle,pCallBacks,pAppData);
            if (eError != OMX_ErrorNone)
                pComp->ComponentDeInit(hHandle);
        }

        if (eError == OMX_ErrorNone) {
            *pHandle = hHandle;
        }
        else {
            *pHandle = NULL;
            free(pComp);
        }
    } 

    if (eError == OMX_ErrorNone) {
        vcos_mutex_lock(&lock);
        nActiveHandles++;
        vcos_mutex_unlock(&lock);
    }

    return eError;
}

/* OMX_FreeHandle */
OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(OMX_IN OMX_HANDLETYPE hComponent) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pComp;

    if (hComponent == NULL || ilcs_service == NULL)
        return OMX_ErrorBadParameter;

    pComp = (OMX_COMPONENTTYPE*)hComponent;

    if (ilcs_service == NULL)
        return OMX_ErrorBadParameter;

    eError = (pComp->ComponentDeInit)(hComponent);
    if (eError == OMX_ErrorNone) {
        vcos_mutex_lock(&lock);
        --nActiveHandles;
        vcos_mutex_unlock(&lock);
        free(pComp);
    }

    vcos_assert(nActiveHandles >= 0);

    return eError;
}

/* OMX_SetupTunnel */
OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(OMX_IN OMX_HANDLETYPE hOutput, OMX_IN OMX_U32 nPortOutput, OMX_IN OMX_HANDLETYPE hInput, OMX_IN OMX_U32 nPortInput) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pCompIn, *pCompOut;
    OMX_TUNNELSETUPTYPE oTunnelSetup;

    if ((hOutput == NULL && hInput == NULL) || ilcs_service == NULL)
        return OMX_ErrorBadParameter;

    oTunnelSetup.nTunnelFlags = 0;
    oTunnelSetup.eSupplier = OMX_BufferSupplyUnspecified;

    pCompOut = (OMX_COMPONENTTYPE*)hOutput;

    if (hOutput){
        eError = pCompOut->ComponentTunnelRequest(hOutput, nPortOutput, hInput, nPortInput, &oTunnelSetup);
    }

    if (eError == OMX_ErrorNone && hInput) {
        pCompIn = (OMX_COMPONENTTYPE*)hInput;
        eError = pCompIn->ComponentTunnelRequest(hInput, nPortInput, hOutput, nPortOutput, &oTunnelSetup);

        if (eError != OMX_ErrorNone && hOutput) {
            /* cancel tunnel request on output port since input port failed */
            pCompOut->ComponentTunnelRequest(hOutput, nPortOutput, NULL, 0, NULL);
        }
    }
    return eError;
}

/* OMX_GetComponentsOfRole */
OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_IN OMX_STRING role, OMX_INOUT OMX_U32 *pNumComps, OMX_INOUT OMX_U8  **compNames) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    *pNumComps = 0;
    return eError;
}

/* OMX_GetRolesOfComponent */
OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_IN OMX_STRING compName, OMX_INOUT OMX_U32 *pNumRoles, OMX_OUT OMX_U8 **roles) {
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    *pNumRoles = 0;
    return eError;
}

/* OMX_GetDebugInformation */
OMX_ERRORTYPE OMX_GetDebugInformation (OMX_OUT OMX_STRING debugInfo, OMX_INOUT OMX_S32 *pLen) {
    if (ilcs_service == NULL)
        return OMX_ErrorBadParameter;

    return vcil_out_get_debug_information(ilcs_get_common(ilcs_service), debugInfo, pLen);
}


// ---------------------------------

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

    if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_video_decode), m_buffer) != OMX_ErrorNone)
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
    if (omx_err != OMX_ErrorNone)
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