#pragma once

#include "textureStream.h"

#ifdef PLATFORM_RPI

#include "bcm_host.h"
#include "ilclient.h"

#include <stdio.h>

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