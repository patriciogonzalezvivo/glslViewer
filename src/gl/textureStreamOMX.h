#pragma once

#include "textureStream.h"

#ifdef PLATFORM_RPI

#include <stdio.h>

#include "bcm_host.h"

#include "IL/OMX_Broadcom.h"
#include "interface/vcos/vcos.h"

typedef struct _ILCLIENT_T ILCLIENT_T;
struct _COMPONENT_T;
typedef struct _COMPONENT_T COMPONENT_T;

typedef void (*ILCLIENT_CALLBACK_T)(void *userdata, COMPONENT_T *comp, OMX_U32 data);
typedef void (*ILCLIENT_BUFFER_CALLBACK_T)(void *data, COMPONENT_T *comp);
typedef void *(*ILCLIENT_MALLOC_T)(void *userdata, VCOS_UNSIGNED size, VCOS_UNSIGNED align, const char *description);
typedef void (*ILCLIENT_FREE_T)(void *userdata, void *pointer);

  
typedef struct {
    COMPONENT_T *source;  /**< The source component */
    int source_port;      /**< The output port index on the source component */
    COMPONENT_T *sink;    /**< The sink component */
    int sink_port;        /**< The input port index on the sink component */
} TUNNEL_T;


class TextureStreamOMX : public TextureStream {
public:
    TextureStreamOMX();
    virtual ~TextureStreamOMX();

    virtual bool    load(const std::string& _filepath, bool _vFlip);
    virtual bool    update();
    virtual void    clear();

protected:

    FILE*       m_file;
    void*       m_eglImage;

    OMX_BUFFERHEADERTYPE*   m_buffer;
    ILCLIENT_T*             m_client;
    TUNNEL_T                m_tunnel[4];
    COMPONENT_T*            m_list[5];
    COMPONENT_T*            m_video_decode = NULL;
    COMPONENT_T*            m_video_scheduler = NULL;

    int             m_port_settings_changed = 0;
    int             m_first_packet = 1;
    int             m_status = 0;

};
#endif