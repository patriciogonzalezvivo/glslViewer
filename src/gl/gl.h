#if defined(PLATFORM_OSX)
#define GL_PROGRAM_BINARY_LENGTH 0x8741
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#elif defined(PLATFORM_LINUX)
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#elif defined(PLATFORM_RPI)
#include "bcm_host.h"
#undef countof
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#elif defined(PLATFORM_RPI4)
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
