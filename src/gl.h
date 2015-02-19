#pragma once

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>

#include "utils.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "bcm_host.h"

typedef struct Mouse{
    Mouse():x(0),y(0),button(0){};
    
    float   x,y;
    float   velX,velY;
    int     button;
};
static Mouse mouse;
static int keypress;

typedef struct {
    uint32_t x, y, width, height;
} Viewport;

static Viewport window;

// OPENGL on RASPBERRYPI
//----------------------------------------------------
typedef struct {
    uint32_t screen_width;
    uint32_t screen_height;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

} CUBE_STATE_T;
static CUBE_STATE_T _state, *state=&_state;

void initGL(int argc, char **argv){

    // Start OpenGL ES
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

    window.x = 0;
    window.y = 0;
    window.width = state->screen_width;
    window.height = state->screen_height;

    //Setup
    for (int i = 1; i < argc ; i++){
        if ( std::string(argv[i]) == "-x" ) {
            i++;
            window.x = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-y" ) {
            i++;
            window.y = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-w" || 
                    std::string(argv[i]) == "--width" ) {
            i++;
            window.width = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-h" || 
                    std::string(argv[i]) == "--height") {
            i++;
            window.height = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "--square") {
            if (state->screen_width > state->screen_height) {
                window.x = state->screen_width/2-state->screen_height/2;
            } else {
                window.y = state->screen_height/2-state->screen_width/2;
            }
            window.width = window.height = MIN(state->screen_width,state->screen_height);
        } else if ( std::string(argv[i]) == "-l" || 
                    std::string(argv[i]) == "--life-coding" ){
            window.x = window.width-500;
            window.width = window.height = 500;
        }
    }

    dst_rect.x = window.x;
    dst_rect.y = window.y;
    dst_rect.width = window.width;
    dst_rect.height = window.height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = window.width << 16;
    src_rect.height = window.height << 16;

    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );

    dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display,
                                       0/*layer*/, &dst_rect, 0/*src*/,
                                       &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);

    nativewindow.element = dispman_element;
    nativewindow.width = window.width;
    nativewindow.height = window.height;
    vc_dispmanx_update_submit_sync( dispman_update );

    state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
    assert(state->surface != EGL_NO_SURFACE);

    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);

    printf("OpenGL Initialize at %i,%i,%i,%i\n",window.x,window.y,window.width,window.height);
}

bool updateMouse(){
    static int fd = -1;

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
        if (mouse.x < 0) mouse.x=0;
        if (mouse.y < 0) mouse.y=0;
        if (mouse.x > window.width) mouse.x = window.width;
        if (mouse.y > window.height) mouse.y = window.height;
        return true;
    }
    return false;
}

int getkey() {
    int character;
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);
    
    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    character = fgetc(stdin);

    /* restore the original terminal attributes */
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
    
    return character;
}

void updateGL(){
    updateMouse();
    keypress = getkey();
}

void renderGL(){
    eglSwapBuffers(state->display, state->surface);
}

void closeGL(){
    eglSwapBuffers(state->display, state->surface);

    // Release OpenGL resources
    eglMakeCurrent( state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    eglDestroySurface( state->display, state->surface );
    eglDestroyContext( state->display, state->context );
    eglTerminate( state->display );

    printf("\nOpenGL Closed\n");
}