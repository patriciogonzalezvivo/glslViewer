// #pragma once

#ifdef PLATFORM_OSX
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

#ifdef PLATFORM_LINUX
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#endif

#ifdef PLATFORM_RPI
#include "bcm_host.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif
