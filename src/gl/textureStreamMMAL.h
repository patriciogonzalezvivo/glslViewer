
#pragma once

#include "textureStream.h"

#ifdef PLATFORM_RPI
#include "gl.h"
#include "shader.h"
#include "vbo.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

class TextureStreamMMAL : public TextureStream {
public:
    TextureStreamMMAL();
    virtual ~TextureStreamMMAL();

    virtual bool    load(const std::string& _filepath, bool _vFlip);
    virtual bool    update();
    virtual void    clear();


protected:

    static void camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);
    static void video_output_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer);

    MMAL_COMPONENT_T    *camera_component = NULL;
    //RASPICAM_CAMERA_PARAMETERS cameraParameters;

    GLuint          m_fbo_id;
    GLuint          m_old_fbo_id;
    GLuint          m_brcm_id;
    //EGLImageKHR     m_egl_img;
    Shader          m_shader;
    Vbo*            m_vbo;
};

#endif
