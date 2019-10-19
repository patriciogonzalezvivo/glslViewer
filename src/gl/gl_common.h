
// Default for RASPBERRYPI < 4
#if defined(DRIVER_VC)
#include "bcm_host.h"
#undef countof

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// Default for RASPBERRY PI4
#elif defined(DRIVER_GBM)
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// MACOS
#elif defined(PLATFORM_OSX)
#define GL_PROGRAM_BINARY_LENGTH 0x8741
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#elif defined(_WIN32)
#include <GL/glew.h>
#else
// ANY LINUX using GLFW 
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#endif
