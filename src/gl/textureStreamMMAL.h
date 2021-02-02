
#pragma once

#include "textureStream.h"

// #define PLATFORM_RPI
#if defined(PLATFORM_RPI) || defined(PLATFORM_RPI4)

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
    MMAL_PORT_T         *preview_port   = NULL;
    MMAL_PORT_T         *video_port     = NULL;
    MMAL_PORT_T         *still_port     = NULL;
    MMAL_QUEUE_T        *video_queue    = NULL;
    MMAL_POOL_T         *video_pool     = NULL;

    //RASPICAM_CAMERA_PARAMETERS cameraParameters;

    // GLuint cam_ytex, cam_utex, cam_vtex;
    EGLImageKHR yimg = EGL_NO_IMAGE_KHR;
    EGLImageKHR uimg = EGL_NO_IMAGE_KHR;
    EGLImageKHR vimg = EGL_NO_IMAGE_KHR;

    int m_fps;
};

#endif
