#include "gl.h"

#ifdef PLATFORM_RPI 

#ifndef DRIVER_LEGACY
static PFNEGLCREATEIMAGEKHRPROC createImageProc = NULL;
static PFNEGLDESTROYIMAGEKHRPROC destroyImageProc = NULL;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTargetTexture2DProc = NULL;
#endif

EGLImageKHR createImage(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) {

    #ifdef DRIVER_LEGACY
    return eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);

    #else
    if (!createImageProc)
        createImageProc = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");

    return createImageProc(dpy, ctx, target, buffer, attrib_list);
    #endif
}

EGLBoolean destroyImage(EGLDisplay dpy, EGLImageKHR image) {

    #ifdef DRIVER_LEGACY
    return eglDestroyImageKHR(dpy, image);

    #else
    if (!destroyImageProc)
        destroyImageProc = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
    
    return destroyImageProc(dpy, image);
    #endif
}

void imageTargetTexture2D(EGLenum target, EGLImageKHR image) {

    #ifdef DRIVER_LEGACY
    return glEGLImageTargetTexture2DOES(target, image);

    #else
    if (!imageTargetTexture2DProc)
        imageTargetTexture2DProc = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
    
    imageTargetTexture2DProc(target, image);
    #endif
}

#endif