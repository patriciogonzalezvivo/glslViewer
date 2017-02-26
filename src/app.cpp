#include "app.h"

#include <time.h>
#include <sys/time.h>
#include "glm/gtc/matrix_transform.hpp"
#include "utils.h"

// Common global variables
//----------------------------------------------------
const std::string appTitle = "glslViewer";
static glm::mat4 orthoMatrix;
typedef struct {
    float     x,y;
    int       button;
    float     velX,velY;
    glm::vec2 drag;
} Mouse;
struct timeval tv;
static Mouse mouse;
static bool left_mouse_button_down = false;
static glm::vec4 iMouse = {0.0, 0.0, 0.0, 0.0};
static glm::ivec4 viewport;
static double fTime = 0.0f;
static double fDelta = 0.0f;
static double fFPS = 0.0f;
static float fPixelDensity = 1.0;

#ifdef PLATFORM_RPI
#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <string>
#include <fstream>

#define check() assert(glGetError() == 0)

// Raspberry globals
//----------------------------------------------------
DISPMANX_DISPLAY_HANDLE_T dispman_display;

EGLDisplay display;
EGLSurface surface;
EGLContext context;

// unsigned long long timeStart;
struct timespec time_start;
static bool bBcm = false;

double getTimeSec() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    timespec temp;
    if ((now.tv_nsec-time_start.tv_nsec)<0) {
        temp.tv_sec = now.tv_sec-time_start.tv_sec-1;
        temp.tv_nsec = 1000000000+now.tv_nsec-time_start.tv_nsec;
    } else {
        temp.tv_sec = now.tv_sec-time_start.tv_sec;
        temp.tv_nsec = now.tv_nsec-time_start.tv_nsec;
    }
    return double(temp.tv_sec) + double(temp.tv_nsec/1000000000.);
}

#else

// OSX/Linux globals
//----------------------------------------------------
static GLFWwindow* window;
#endif

void initGL (glm::ivec4 &_viewport, bool _headless) {

    #ifdef PLATFORM_RPI
        // RASPBERRY_PI

        // Start clock
        // gettimeofday(&tv, NULL);
        // timeStart = (unsigned long long)(tv.tv_sec) * 1000 +
        //             (unsigned long long)(tv.tv_usec) / 1000; 
        clock_gettime(CLOCK_MONOTONIC, &time_start);

        // Start OpenGL ES
        if (!bBcm) {
            bcm_host_init();
            bBcm = true;
        }

        // Clear application state
        EGLBoolean result;
        EGLint num_config;

        static const EGLint attribute_list[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            // EGL_SAMPLE_BUFFERS, 1, EGL_SAMPLES, 4,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
        };

        static const EGLint context_attributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        EGLConfig config;

        // get an EGL display connection
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        assert(display!=EGL_NO_DISPLAY);
        check();

        // initialize the EGL display connection
        result = eglInitialize(display, NULL, NULL);
        assert(EGL_FALSE != result);
        check();

        // get an appropriate EGL frame buffer configuration
        result = eglChooseConfig(display, attribute_list, &config, 1, &num_config);
        assert(EGL_FALSE != result);
        check();

        // get an appropriate EGL frame buffer configuration
        result = eglBindAPI(EGL_OPENGL_ES_API);
        assert(EGL_FALSE != result);
        check();

        // create an EGL rendering context
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
        assert(context!=EGL_NO_CONTEXT);
        check();

        static EGL_DISPMANX_WINDOW_T nativeviewport;

        VC_RECT_T dst_rect;
        VC_RECT_T src_rect;

        //  Initially the viewport is for all the screen
        dst_rect.x = _viewport.x;
        dst_rect.y = _viewport.y;
        dst_rect.width = _viewport.z;
        dst_rect.height = _viewport.w;

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = _viewport.z << 16;
        src_rect.height = _viewport.w << 16;

        DISPMANX_ELEMENT_HANDLE_T dispman_element;
        DISPMANX_UPDATE_HANDLE_T dispman_update;

        if (_headless) {
            uint32_t dest_image_handle;
            DISPMANX_RESOURCE_HANDLE_T dispman_resource;
            dispman_resource = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, _viewport.z, _viewport.w, &dest_image_handle);
            dispman_display = vc_dispmanx_display_open_offscreen(dispman_resource, DISPMANX_NO_ROTATE);
        } else {
            dispman_display = vc_dispmanx_display_open(0); // LCD
        }
        
        dispman_update = vc_dispmanx_update_start(0);
        dispman_element = vc_dispmanx_element_add(  dispman_update, dispman_display,
                                                    0/*layer*/, &dst_rect, 0/*src*/,
                                                    &src_rect, DISPMANX_PROTECTION_NONE, 
                                                    0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);

        nativeviewport.element = dispman_element;
        nativeviewport.width = _viewport.z;
        nativeviewport.height = _viewport.w;
        vc_dispmanx_update_submit_sync( dispman_update );
        check();

        surface = eglCreateWindowSurface( display, config, &nativeviewport, NULL );
        assert(surface != EGL_NO_SURFACE);
        check();

        // connect the context to the surface
        result = eglMakeCurrent(display, surface, surface, context);
        assert(EGL_FALSE != result);
        check();

        setWindowSize(_viewport.z,_viewport.w);
    #else
        // OSX/LINUX use GLFW
        // ---------------------------------------------
        glfwSetErrorCallback([](int err, const char* msg)->void {
            std::cerr << "GLFW error 0x"<<std::hex<<err<<std::dec<<": "<<msg<<"\n";
        });
        if(!glfwInit()) {
            std::cerr << "ABORT: GLFW init failed" << std::endl;
            exit(-1);
        }

        if (_headless) {
            glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        }

        window = glfwCreateWindow(_viewport.z, _viewport.w, appTitle.c_str(), NULL, NULL);
        
        if(!window) {
            glfwTerminate();
            std::cerr << "ABORT: GLFW create window failed" << std::endl;
            exit(-1);
        }

        setWindowSize(_viewport.z, _viewport.w);

        glfwMakeContextCurrent(window);
        glfwSetWindowSizeCallback(window, [](GLFWwindow* _window, int _w, int _h) {
            setWindowSize(_w,_h);
        });

        glfwSetKeyCallback(window, [](GLFWwindow* _window, int _key, int _scancode, int _action, int _mods) {
            onKeyPress(_key);
        });

        // callback when a mouse button is pressed or released
        glfwSetMouseButtonCallback(window, [](GLFWwindow* _window, int button, int action, int mods) {
            if (button == GLFW_MOUSE_BUTTON_1) {
                // update iMouse when left mouse button is pressed or released
                if (action == GLFW_PRESS && !left_mouse_button_down) {
                    left_mouse_button_down = true;
                    iMouse.x = mouse.x;
                    iMouse.y = mouse.y;
                    iMouse.z = mouse.x;
                    iMouse.w = mouse.y;
                } else if (action == GLFW_RELEASE && left_mouse_button_down) {
                    left_mouse_button_down = false;
                    iMouse.z = -iMouse.z;
                    iMouse.w = -iMouse.w;
                }
            }
            if (action == GLFW_PRESS) {
                mouse.drag.x = mouse.x;
                mouse.drag.y = mouse.y;
            }
        });

        glfwSetScrollCallback(window, [](GLFWwindow* _window, double xoffset, double yoffset) {
            onScroll(-yoffset * fPixelDensity);
        });

        // callback when the mouse cursor moves
        glfwSetCursorPosCallback(window, [](GLFWwindow* _window, double x, double y) {
            // Convert x,y to pixel coordinates relative to viewport.
            // (0,0) is lower left corner.
            x *= fPixelDensity;
            y *= fPixelDensity;
            y = viewport.w - y;

            // mouse.velX,mouse.velY is the distance the mouse cursor has moved
            // since the last callback, during a drag gesture.
            // mouse.drag is the previous mouse position, during a drag gesture.
            // Note that mouse.drag is *not* constrained to the viewport.
            mouse.velX = x - mouse.drag.x;
            mouse.velY = y - mouse.drag.y;
            mouse.drag.x = x;
            mouse.drag.y = y;

            // mouse.x,mouse.y is the current cursor position, constrained
            // to the viewport.
            mouse.x = x;
            mouse.y = y;
            if (mouse.x < 0) mouse.x = 0;
            if (mouse.y < 0) mouse.y = 0;
            if (mouse.x > viewport.z) mouse.x = viewport.z;
            if (mouse.y > viewport.w) mouse.y = viewport.w;

            // update iMouse when cursor moves
            if (left_mouse_button_down) {
                iMouse.x = mouse.x;
                iMouse.y = mouse.y;
            }

            /*
             * TODO: the following code would best be moved into the
             * mouse button callback. If you click the mouse button without
             * moving the mouse, then using this code, the mouse click doesn't
             * register until the cursor is moved. (@doug-moen)
             */
            int action1 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
            int action2 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);
            int button = 0;

            if (action1 == GLFW_PRESS) button = 1;
            else if (action2 == GLFW_PRESS) button = 2;

            // Lunch events
            if (mouse.button == 0 && button != mouse.button) {
                mouse.button = button;
                onMouseClick(mouse.x,mouse.y,mouse.button);
            } 
            else {
                mouse.button = button;
            }

            if (mouse.velX != 0.0 || mouse.velY != 0.0) {
                if (button != 0) onMouseDrag(mouse.x,mouse.y,mouse.button);
                else onMouseMove(mouse.x,mouse.y);
            }
        });

        glfwSetWindowPosCallback(window, [](GLFWwindow* _window, int x, int y) {
            if (fPixelDensity != getPixelDensity()) {
                setWindowSize(viewport.z, viewport.w);
            }
        });

        glfwSwapInterval(1);
    #endif
}

bool isGL(){
    #ifdef PLATFORM_RPI
        // RASPBERRY_PI
        return bBcm;
    #else
        // OSX/LINUX
        return !glfwWindowShouldClose(window);
    #endif
}

void updateGL(){
    // Update time
    // --------------------------------------------------------------------
    #ifdef PLATFORM_RPI
        // RASPBERRY_PI
        // gettimeofday(&tv, NULL);
        // unsigned long long timeNow =    (unsigned long long)(tv.tv_sec) * 1000 +
        //                                 (unsigned long long)(tv.tv_usec) / 1000;
        // double now = (timeNow - timeStart)*0.001;
        double now = getTimeSec();
    #else
        // OSX/LINUX
        double now = glfwGetTime();
    #endif
    fDelta = now - fTime;
    fTime = now;

    static int frame_count = 0;
    static double lastTime = 0.0;
    frame_count++;
    lastTime += fDelta;
    if (lastTime >= 1.) {
        fFPS = double(frame_count);
        frame_count = 0;
        lastTime -= 1.;
    }

    // EVENTS
    // --------------------------------------------------------------------
    #ifdef PLATFORM_RPI
        // RASPBERRY_PI
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
                    return;
                } else if (m.buttons&8) {
                    break; // This bit should always be set
                }
                
                read(fd, &m, 1); // Try to sync up again
            }
            
            // Set button value
            int button = m.buttons&3;
            if (button) mouse.button = button;
            else mouse.button = 0;
            
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
            if (mouse.button == 0 && button != mouse.button) {
                mouse.button = button;
                onMouseClick(mouse.x, mouse.y, mouse.button);
            } 
            else {
                mouse.button = button;
            }

            if (mouse.velX != 0.0 || mouse.velY != 0.0) {
                if (button != 0) onMouseDrag(mouse.x, mouse.y, mouse.button);
                else onMouseMove(mouse.x, mouse.y);
            }
        }
    #else
        std::string title = appTitle + ":..: FPS:" + toString(fFPS);
        glfwSetWindowTitle(window, title.c_str());

        // OSX/LINUX
        glfwPollEvents();
    #endif
}

void renderGL(){
    #ifdef PLATFORM_RPI
        // RASPBERRY_PI
        eglSwapBuffers(display, surface);
    #else
        // OSX/LINUX
        glfwSwapBuffers(window);
    #endif
}

void closeGL(){
    #ifdef PLATFORM_RPI
        // RASPBERRY_PI
        eglSwapBuffers(display, surface);
        // Release OpenGL resources
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(display, surface);
        eglDestroyContext(display, context);
        eglTerminate(display);
        eglReleaseThread();
        vc_dispmanx_display_close(dispman_display);
        bcm_host_deinit();
    #else
        // OSX/LINUX
        glfwSetWindowShouldClose(window, GL_TRUE);
        glfwTerminate();
    #endif
}
//-------------------------------------------------------------

void setWindowSize(int _width, int _height) {
    viewport.z = _width;
    viewport.w = _height;
    fPixelDensity = getPixelDensity();
    glViewport(0.0, 0.0, (float)getWindowWidth(), (float)getWindowHeight());
    orthoMatrix = glm::ortho((float)viewport.x, (float)getWindowWidth(), (float)viewport.y, (float)getWindowHeight());

    onViewportResize(getWindowWidth(), getWindowHeight());
}

glm::ivec2 getScreenSize() {
    glm::ivec2 screen;
    
    #ifdef PLATFORM_RPI
        // RASPBERRYPI
        
        if (!bBcm) {
            bcm_host_init();
            bBcm = true;
        }
        uint32_t screen_width;
        uint32_t screen_height;
        int32_t success = graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);
        assert(success >= 0);
        screen = glm::ivec2(screen_width, screen_height);
    #else
        // OSX/Linux
        glfwGetMonitorPhysicalSize(glfwGetPrimaryMonitor(), &screen.x, &screen.y);
    #endif

    return screen;
}

float getPixelDensity() {
    #ifdef PLATFORM_RPI
        // RASPBERRYPI
        return 1.;
    #else
        // OSX/LINUX
        int window_width, window_height, framebuffer_width, framebuffer_height;
        glfwGetWindowSize(window, &window_width, &window_height);
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
        return float(framebuffer_width)/float(window_width);
    #endif
}

glm::ivec4 getViewport() {
    return viewport;
}

int getWindowWidth() {
    return viewport.z*fPixelDensity;
}

int getWindowHeight() {
    return viewport.w*fPixelDensity;
}

glm::mat4 getOrthoMatrix() {
    return orthoMatrix;
}

glm::vec4 getDate() {
    gettimeofday(&tv, NULL);
    struct tm *tm;
    tm = localtime(&tv.tv_sec);
    // std::cout << "y: " << tm->tm_year+1900 << " m: " << tm->tm_mon << " d: " << tm->tm_mday << " s: " << tm->tm_hour*3600.0f+tm->tm_min*60.0f+tm->tm_sec+tv.tv_usec*0.000001 << std::endl;
    return glm::vec4(tm->tm_year+1900,
                     tm->tm_mon,
                     tm->tm_mday,
                     tm->tm_hour*3600.0f+tm->tm_min*60.0f+tm->tm_sec+tv.tv_usec*0.000001);
}

double getTime() {
    return fTime;
}

double getDelta() {
    return fDelta;
}

double getFPS() {
    return fFPS;
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

glm::vec4 get_iMouse() {
    return iMouse;
}
