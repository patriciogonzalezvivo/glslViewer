#include "app.h"

#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>

#include "glm/gtc/matrix_transform.hpp"

#include "utils.h"

#ifdef PLATFORM_RPI
typedef struct {
    uint32_t screen_width;
    uint32_t screen_height;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
} CUBE_STATE_T;
static CUBE_STATE_T _state, *state=&_state;
#else
static GLFWwindow* window;
static float dpiScale = 1.0;
#endif

static glm::ivec4 viewport;
static glm::mat4 orthoMatrix;

typedef struct {
    float   x,y;
    float   velX,velY;
    int     button;
} Mouse;
static Mouse mouse;
static unsigned char keyPressed;

// TIME
//----------------------------------------------------
struct timeval tv;
unsigned long long timeStart;
float timeSec = 0.0f;

void initTime() {
    gettimeofday(&tv, NULL);
    timeStart = (unsigned long long)(tv.tv_sec) * 1000 +
                (unsigned long long)(tv.tv_usec) / 1000; 
}

void updateTime() {
    gettimeofday(&tv, NULL);
    unsigned long long timeNow =    (unsigned long long)(tv.tv_sec) * 1000 +
                                    (unsigned long long)(tv.tv_usec) / 1000;

    timeSec = (timeNow - timeStart)*0.001;
}

float getTime() {
    return timeSec;
}

// OPENGL through BREADCOM GPU on RASPBERRYPI
//----------------------------------------------------
#ifdef PLATFORM_RPI
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
        EGL_DEPTH_SIZE, 16,
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
    viewport.z = state->screen_width;
    viewport.w = state->screen_height;

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
            viewport.z = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-h" || 
                    std::string(argv[i]) == "--height") {
            i++;
            viewport.w = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "--square") {
            if (state->screen_width > state->screen_height) {
                viewport.x = state->screen_width/2-state->screen_height/2;
            } else {
                viewport.y = state->screen_height/2-state->screen_width/2;
            }
            viewport.z = viewport.w = MIN(state->screen_width,state->screen_height);
        } else if ( std::string(argv[i]) == "-l" || 
                    std::string(argv[i]) == "--life-coding" ){
            viewport.x = viewport.z-500;
            viewport.z = viewport.w = 500;
        }
    }

    dst_rect.x = viewport.x;
    dst_rect.y = viewport.y;
    dst_rect.width = viewport.z;
    dst_rect.height = viewport.w;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = viewport.z << 16;
    src_rect.height = viewport.w << 16;

    dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );

    dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display,
                                       0/*layer*/, &dst_rect, 0/*src*/,
                                       &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);

    nativeviewport.element = dispman_element;
    nativeviewport.z = viewport.z;
    nativeviewport.w = viewport.w;
    vc_dispmanx_update_submit_sync( dispman_update );

    state->surface = eglCreateWindowSurface( state->display, config, &nativeviewport, NULL );
    assert(state->surface != EGL_NO_SURFACE);

    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);

    ///printf("OpenGL Initialize at %i,%i,%i,%i\n",viewport.x,viewport.y,viewport.z,viewport.w);
    initTime();
}

bool isGL(){
    return true;
}

bool getMouse(){
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
        int button = m.buttons&3;
        if (button)
            mouse.button = button;
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
        if (mouse.x > viewport.z) mouse.x = viewport.z;
        if (mouse.y > viewport.w) mouse.y = viewport.w;

        // Lunch events
        if(mouse.button == 0 && button != mouse.button){
            mouse.button = button;
            onMouseClick(mouse.x,mouse.y,mouse.button);
        } else {
            mouse.button = button;
        }

        if(mouse.velX != 0.0 || mouse.velY != 0.0){
            if (button != 0) {
                onMouseDrag(mouse.x,mouse.y,mouse.button);
            } else {
                onMouseMove(mouse.x,mouse.y);
            }
        }  

        return true;
    }
    return false;
}

int getKey() {
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
    updateTime();
    getMouse();

    int key = getKey();
    if ( key != 0 && key != keyPressed ){
        keyPressed = key;
        onKeyPress(key);
    }  
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

    // printf("\nOpenGL Closed\n");
}

#else  // PLATFORM_LINUX || PLATFORM_OSX

// OPENGL through GLFW on LINUX and OSX
//----------------------------------------------------
void handleError(const std::string& _message, int _exitStatus) {
    std::cerr << "ABORT: "<< _message << std::endl;
    exit(_exitStatus);
}

void handleKeypress(GLFWwindow* _window, int _key, int _scancode, int _action, int _mods) {
    onKeyPress(_key);
}

void fixDpiScale(){
    int window_width, window_height, framebuffer_width, framebuffer_height;
    glfwGetWindowSize(window, &window_width, &window_height);
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    dpiScale = framebuffer_width/window_width;
}

void handleResize(GLFWwindow* _window, int _w, int _h) {
    fixDpiScale();
    setWindowSize(_w*dpiScale,_h*dpiScale);
}

void handleCursor(GLFWwindow* _window, double x, double y) {

    // Update stuff
    x *= dpiScale;
    y *= dpiScale;

    mouse.velX = x - mouse.x;
    mouse.velY = (viewport.w - y) - mouse.y;
    mouse.x = x;
    mouse.y = viewport.w - y;

    if (mouse.x < 0) mouse.x=0;
    if (mouse.y < 0) mouse.y=0;
    if (mouse.x > viewport.z) mouse.x = viewport.z;
    if (mouse.y > viewport.w) mouse.y = viewport.w;

    int action1 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    int action2 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);
    int button = 0;

    if (action1 == GLFW_PRESS) {
        button = 1;
    } else if (action2 == GLFW_PRESS) {
        button = 2;
    }

    // Lunch events
    if(mouse.button == 0 && button != mouse.button){
        mouse.button = button;
        onMouseClick(mouse.x,mouse.y,mouse.button);
    } else {
        mouse.button = button;
    }

    if(mouse.velX != 0.0 || mouse.velY != 0.0){
        if (button != 0) {
            onMouseDrag(mouse.x,mouse.y,mouse.button);
        } else {
            onMouseMove(mouse.x,mouse.y);
        }
    }    
}

void initGL(int argc, char **argv){
    viewport.x = 0;
    viewport.y = 0;
    viewport.z = 500;
    viewport.w = 500;

    for (int i = 1; i < argc ; i++){
        if ( std::string(argv[i]) == "-w" || 
             std::string(argv[i]) == "--width" ) {
            i++;
            viewport.z = getInt(std::string(argv[i]));
        } else if ( std::string(argv[i]) == "-h" || 
                    std::string(argv[i]) == "--height") {
            i++;
            viewport.w = getInt(std::string(argv[i]));
        }
    }

    if(!glfwInit()) {
        handleError("GLFW init failed", -1);
    }

    window = glfwCreateWindow(viewport.z, viewport.w, "glslViewer", NULL, NULL);

    if(!window) {
        glfwTerminate();
        handleError("GLFW create window failed", -1);
    }

    fixDpiScale();
    viewport.z *= dpiScale;
    viewport.w *= dpiScale;
    setWindowSize(viewport.z,viewport.w);

    glfwMakeContextCurrent(window);

    glfwSetWindowSizeCallback(window, handleResize);
    glfwSetKeyCallback(window, handleKeypress);
    glfwSetCursorPosCallback(window, handleCursor);

    initTime();
}

bool isGL(){
    return !glfwWindowShouldClose(window);
}

void updateGL(){
    updateTime();
    keyPressed = -1;
    glfwPollEvents();
}

void renderGL(){
    glfwSwapBuffers(window);
}

void closeGL(){
    glfwSetWindowShouldClose(window, GL_TRUE);
    glfwTerminate();
}
#endif

void setWindowSize(int _width, int _height) {
    viewport.z = _width;
    viewport.w = _height;
    glViewport((float)viewport.x, (float)viewport.y, (float)viewport.z, (float)viewport.w);
    orthoMatrix = glm::ortho((float)viewport.x, (float)viewport.z, (float)viewport.y, (float)viewport.w);

    onViewportResize(viewport.z, viewport.w);
}

int getWindowWidth(){
    return viewport.z;
}

int getWindowHeight(){
    return viewport.w;
}

glm::mat4 getOrthoMatrix(){
    return orthoMatrix;
}

float getMouseX(){
    return mouse.x;
}

float getMouseY(){
    return mouse.y;
}

glm::vec2 getMousePosition() {
    return glm::vec2(mouse.x,mouse.y);
}

float getMouseVelX(){
    return mouse.velX;
}

float getMouseVelY(){
    return mouse.velY;
}

glm::vec2 getMouseVelocity() {
    return glm::vec2(mouse.velX,mouse.velY);
}

int getMouseButton(){
    return mouse.button;
}

unsigned char getKeyPressed(){
    return keyPressed;
}