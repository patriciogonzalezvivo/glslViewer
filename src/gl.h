#pragma once

#include <assert.h>
#include <fcntl.h>

#include <iostream> 

#include "bcm_host.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

typedef struct {
    uint32_t screen_width;
    uint32_t screen_height;

    uint32_t mouse_x;
    uint32_t mouse_y;
    uint32_t mouse_b;

    // OpenGL|ES objects
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

} CUBE_STATE_T;

static CUBE_STATE_T _state, *state=&_state;

typedef struct Mouse{
    Mouse():x(0),y(0),button(0){};
    
    float   x,y;
    float   velX,velY;
    int     button;
};

static Mouse mouse;

static void initOpenGL(){
    bcm_host_init();

    // Clear application state
    memset( state, 0, sizeof( *state ) );

	int32_t success = 0;
    EGLBoolean result;
    EGLint num_config;
    
    static EGL_DISPMANX_WINDOW_T nativewindow;
    
    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    
    static const EGLint attribute_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
    
    static const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLConfig config;
    
    // get an EGL display connection
    state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(state->display!=EGL_NO_DISPLAY);
    
    // initialize the EGL display connection
    result = eglInitialize(state->display, NULL, NULL);
    assert(EGL_FALSE != result);
    
    // get an appropriate EGL frame buffer configuration
    result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
    assert(EGL_FALSE != result);
    
    // get an appropriate EGL frame buffer configuration
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);
    
    // create an EGL rendering context
    state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
    assert(state->context!=EGL_NO_CONTEXT);
    
    // create an EGL window surface
    success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
    assert( success >= 0 );

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = state->screen_width;
    dst_rect.height = state->screen_height;
    
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = state->screen_width << 16;
    src_rect.height = state->screen_height << 16;
    
    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );

    dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display,
                                               0/*layer*/, &dst_rect, 0/*src*/,
                                               &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);
    
    nativewindow.element = dispman_element;
    nativewindow.width = state->screen_width;
    nativewindow.height = state->screen_height;
    vc_dispmanx_update_submit_sync( dispman_update );
    
    state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
    assert(state->surface != EGL_NO_SURFACE);
     
    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);

}

static bool updateMouse(){
    static int fd = -1;
    const int width=state->screen_width, height=state->screen_height;
    //static int x=width, y=height;
    const int XSIGN = 1<<4, YSIGN = 1<<5;
    if (fd<0) {
        fd = open("/dev/input/mouse0",O_RDONLY|O_NONBLOCK);
    }
    if (fd>=0) {
        
        // Set values to 0
        mouse.velX=0;
        mouse.velY=0;
        
        // Extract values from driver
        struct {char buttons, dx, dy; } m;
        while (1) {
            int bytes = read(fd, &m, sizeof m);
            
            if (bytes < (int)sizeof m) {
                return false;
            } else if (m.buttons&8) {
                break; // This bit should always be set
            }
            
            read(fd, &m, 1); // Try to sync up again
        }
        
        // Set button value
        if (m.buttons&3)
            mouse.button = m.buttons&3;
        else
            mouse.button = 0;
        
        // Set deltas
        mouse.velX=m.dx;
        mouse.velY=m.dy;
        if (m.buttons&XSIGN) mouse.velX-=256;
        if (m.buttons&YSIGN) mouse.velY-=256;
        
        // Add movement
        mouse.x+=mouse.velX;
        mouse.y+=mouse.velY;
        
        // Clamp values
        if (mouse.x<0) mouse.x=0;
        if (mouse.y<0) mouse.y=0;
        if (mouse.x>width) mouse.x=width;
        if (mouse.y>height) mouse.y=height;
        return true;
    }
    return false;
}
