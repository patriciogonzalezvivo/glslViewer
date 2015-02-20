#pragma once

#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>

#include "gl.h"
#include "utils.h"

typedef struct Mouse{
    Mouse():x(0),y(0),button(0){};
    
    float   x,y;
    float   velX,velY;
    int     button;
};
static Mouse mouse;
static unsigned char keypress;

typedef struct {
    uint32_t x, y, width, height;
} Viewport;

static Viewport viewport;

void resizeGL(int _newWidth, int _newHeight){
    viewport.width = _newWidth;
    viewport.height = _newHeight;
    glViewport(0, 0, viewport.width, viewport.height);
}

#ifdef PLATFORM_OSX
static GLFWwindow* window;

void handleError(const std::string& _message, int _exitStatus) {
    std::cerr << "ABORT: "<< _message << std::endl;
    exit(_exitStatus);
    // exit_func();
}

void handleKeypress(GLFWwindow* _window, int _key, int _scancode, int _action, int _mods) {
    keypress = _key;

    switch (_key) {
        case 256: // ESC
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;
    }
}

void handleResize(GLFWwindow* _window, int _w, int _h) {
    resizeGL(_w, _h);
}

void handleCursor(GLFWwindow* _window, double x, double y) {

#ifdef RETINA_DISPLAY
    x *= 2.0;
    y *= 2.0; 
#endif 

    mouse.velX = x - mouse.x;
    mouse.velY = (viewport.height - y) - mouse.y;
    mouse.x = x;
    mouse.y = viewport.height - y;

    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    if (action == GLFW_PRESS) {
        mouse.button = 1;
    } else {
        mouse.button = 0;
    }

    if (mouse.x < 0) mouse.x=0;
    if (mouse.y < 0) mouse.y=0;
    if (mouse.x > viewport.width) mouse.x = viewport.width;
    if (mouse.y > viewport.height) mouse.y = viewport.height;
}

void initGL(int argc, char **argv){

    const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int screen_width = 500;
    int screen_height = 500;

    if( mode != NULL){
        screen_width = mode->width/2.;
        screen_height = mode->height/2.;
    }
    
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = screen_width;
    viewport.height = screen_height;

    for (int i = 1; i < argc ; i++){
        if ( std::string(argv[i]) == "-x" ) {
            i++;
            viewport.x = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-y" ) {
            i++;
            viewport.y = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-w" || 
                    std::string(argv[i]) == "--width" ) {
            i++;
            viewport.width = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-h" || 
                    std::string(argv[i]) == "--height") {
            i++;
            viewport.height = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "--square") {
            if (screen_width > screen_height) {
                viewport.x = screen_width/2-screen_height/2;
            } else {
                viewport.y = screen_height/2-screen_width/2;
            }
            viewport.width = viewport.height = MIN(screen_width,screen_height);
        } else if ( std::string(argv[i]) == "-l" || 
                    std::string(argv[i]) == "--life-coding" ){
            viewport.x = viewport.width-500;
            viewport.width = viewport.height = 500;
        }
    }

    if(!glfwInit()) {
        handleError("GLFW init failed", -1);
    }

#ifdef RETINA_DISPLAY
    window = glfwCreateWindow(viewport.width/2, viewport.height/2, "piFrag", NULL, NULL);
#else 
    window = glfwCreateWindow(viewport.width, viewport.height, "piFrag", NULL, NULL);
#endif

    if(!window) {
        glfwTerminate();
        handleError("GLFW create window failed", -1);
    }

    glfwMakeContextCurrent(window);

    // glewExperimental = GL_TRUE;
    GLenum err = glewInit();

    if(err != GLEW_OK) {
        std::cerr << glewGetErrorString(err) << std::endl;
        handleError("GlEW init failed", -1);
    }

    glfwSetWindowSizeCallback(window, handleResize);
    glfwSetKeyCallback(window, handleKeypress);
    glfwSetCursorPosCallback(window, handleCursor);
}

bool isGL(){
    return !glfwWindowShouldClose(window);
}

void updateGL(){
    keypress = 0;
    glfwPollEvents();
}

void renderGL(){
    glfwSwapBuffers(window);
}

void closeGL(){
    glfwTerminate();
}
#endif

#ifdef PLATFORM_RPI
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

    static EGL_DISPMANX_WINDOW_T nativeviewport;

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

    // create an EGL viewport surface
    success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
    assert( success >= 0 );

    viewport.x = 0;
    viewport.y = 0;
    viewport.width = state->screen_width;
    viewport.height = state->screen_height;

    //Setup
    for (int i = 1; i < argc ; i++){
        if ( std::string(argv[i]) == "-x" ) {
            i++;
            viewport.x = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-y" ) {
            i++;
            viewport.y = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-w" || 
                    std::string(argv[i]) == "--width" ) {
            i++;
            viewport.width = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-h" || 
                    std::string(argv[i]) == "--height") {
            i++;
            viewport.height = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "--square") {
            if (state->screen_width > state->screen_height) {
                viewport.x = state->screen_width/2-state->screen_height/2;
            } else {
                viewport.y = state->screen_height/2-state->screen_width/2;
            }
            viewport.width = viewport.height = MIN(state->screen_width,state->screen_height);
        } else if ( std::string(argv[i]) == "-l" || 
                    std::string(argv[i]) == "--life-coding" ){
            viewport.x = viewport.width-500;
            viewport.width = viewport.height = 500;
        }
    }

    dst_rect.x = viewport.x;
    dst_rect.y = viewport.y;
    dst_rect.width = viewport.width;
    dst_rect.height = viewport.height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = viewport.width << 16;
    src_rect.height = viewport.height << 16;

    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );

    dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display,
                                       0/*layer*/, &dst_rect, 0/*src*/,
                                       &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);

    nativeviewport.element = dispman_element;
    nativeviewport.width = viewport.width;
    nativeviewport.height = viewport.height;
    vc_dispmanx_update_submit_sync( dispman_update );

    state->surface = eglCreateWindowSurface( state->display, config, &nativeviewport, NULL );
    assert(state->surface != EGL_NO_SURFACE);

    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);

    printf("OpenGL Initialize at %i,%i,%i,%i\n",viewport.x,viewport.y,viewport.width,viewport.height);
}

bool isGL(){
    return true;
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
        if (mouse.x > viewport.width) mouse.x = viewport.width;
        if (mouse.y > viewport.height) mouse.y = viewport.height;
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
#endif