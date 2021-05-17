#include "window.h"

#include <time.h>
#if defined(PLATFORM_WINDOWS)
#define NOMINMAX
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif 

//#include "gl/gl.h"
#include "glm/gtc/matrix_transform.hpp"
#include "tools/text.h"

// Common global variables
//----------------------------------------------------
const std::string appTitle = "glslViewer";
static glm::mat4 orthoMatrix;
typedef struct {
    bool      entered;
    float     x,y;
    int       button;
    float     velX,velY;
    glm::vec2 drag;
} Mouse;
struct timeval tv;
static Mouse mouse;
static glm::vec4 mouse4 = {0.0, 0.0, 0.0, 0.0};
static glm::ivec4 viewport;
static double fTime = 0.0f;
static double fDelta = 0.0f;
static double fFPS = 0.0f;
static float fPixelDensity = 1.0;
static float fRestSec = 0.0167f; // default 60fps 

extern void pal_sleep(uint64_t);

#if defined(DRIVER_GLFW)
// GLWF globals
#include "GLFW/glfw3.h"
//----------------------------------------------------
static bool left_mouse_button_down = false;
static GLFWwindow* window;

#ifdef PLATFORM_RPI
#include "GLFW/glfw3native.h"
EGLDisplay getEGLDisplay() { return glfwGetEGLDisplay(); }
EGLContext getEGLContext() { return glfwGetEGLContext(window); }
#endif

#else
// NON GLWF globals (we have to do all brute force)
// --------------------------------------------------
#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <fstream>

#define check() assert(glGetError() == 0)

// EGL context globals
EGLDisplay display;
EGLContext context;
EGLSurface surface;

EGLDisplay getEGLDisplay() { return display; }
EGLContext getEGLContext() { return context; }

// unsigned long long timeStart;
std::string device_mouse = "/dev/input/mice";
std::string device_screen = "/dev/dri/card1";

// get Time Function
struct timespec time_start;
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

// Get the EGL error back as a string. Useful for debugging.
static const char *eglGetErrorStr() {
    switch (eglGetError()) {
    case EGL_SUCCESS:
        return "The last function succeeded without error.";
    case EGL_NOT_INITIALIZED:
        return "EGL is not initialized, or could not be initialized, for the "
               "specified EGL display connection.";
    case EGL_BAD_ACCESS:
        return "EGL cannot access a requested resource (for example a context "
               "is bound in another thread).";
    case EGL_BAD_ALLOC:
        return "EGL failed to allocate resources for the requested operation.";
    case EGL_BAD_ATTRIBUTE:
        return "An unrecognized attribute or attribute value was passed in the "
               "attribute list.";
    case EGL_BAD_CONTEXT:
        return "An EGLContext argument does not name a valid EGL rendering "
               "context.";
    case EGL_BAD_CONFIG:
        return "An EGLConfig argument does not name a valid EGL frame buffer "
               "configuration.";
    case EGL_BAD_CURRENT_SURFACE:
        return "The current surface of the calling thread is a window, pixel "
               "buffer or pixmap that is no longer valid.";
    case EGL_BAD_DISPLAY:
        return "An EGLDisplay argument does not name a valid EGL display "
               "connection.";
    case EGL_BAD_SURFACE:
        return "An EGLSurface argument does not name a valid surface (window, "
               "pixel buffer or pixmap) configured for GL rendering.";
    case EGL_BAD_MATCH:
        return "Arguments are inconsistent (for example, a valid context "
               "requires buffers not supplied by a valid surface).";
    case EGL_BAD_PARAMETER:
        return "One or more argument values are invalid.";
    case EGL_BAD_NATIVE_PIXMAP:
        return "A NativePixmapType argument does not refer to a valid native "
               "pixmap.";
    case EGL_BAD_NATIVE_WINDOW:
        return "A NativeWindowType argument does not refer to a valid native "
               "window.";
    case EGL_CONTEXT_LOST:
        return "A power management event has occurred. The application must "
               "destroy all contexts and reinitialise OpenGL ES state and "
               "objects to continue rendering.";
    default:
        break;
    }
    return "Unknown error!";
}
#endif

#if defined(DRIVER_LEGACY)
DISPMANX_DISPLAY_HANDLE_T dispman_display;

#elif defined(DRIVER_FAKE_KMS)
// https://github.com/matusnovak/rpi-opengl-without-x/blob/master/triangle_rpi4.c

int device;
drmModeModeInfo mode;
struct gbm_device *gbmDevice;
struct gbm_surface *gbmSurface;
drmModeCrtc *crtc;
uint32_t connectorId;

static drmModeConnector *getConnector(drmModeRes *resources) {
    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector *connector = drmModeGetConnector(device, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED)
            return connector;
        drmModeFreeConnector(connector);
    }
    return NULL;
}

static drmModeEncoder *findEncoder(drmModeRes *resources, drmModeConnector *connector) {
    if (connector->encoder_id)
        return drmModeGetEncoder(device, connector->encoder_id);
    return NULL;
}

static struct gbm_bo *previousBo = NULL;
static uint32_t previousFb;

static void gbmSwapBuffers() {
    struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbmSurface);
    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t pitch = gbm_bo_get_stride(bo);
    uint32_t fb;
    drmModeAddFB(device, mode.hdisplay, mode.vdisplay, 24, 32, pitch, handle, &fb);
    drmModeSetCrtc(device, crtc->crtc_id, fb, 0, 0, &connectorId, 1, &mode);
    if (previousBo) {
        drmModeRmFB(device, previousFb);
        gbm_surface_release_buffer(gbmSurface, previousBo);
    }
    previousBo = bo;
    previousFb = fb;
}

static void gbmClean() {
    // set the previous crtc
    drmModeSetCrtc(device, crtc->crtc_id, crtc->buffer_id, crtc->x, crtc->y, &connectorId, 1, &crtc->mode);
    drmModeFreeCrtc(crtc);
    if (previousBo) {
        drmModeRmFB(device, previousFb);
        gbm_surface_release_buffer(gbmSurface, previousBo);
    }
    gbm_surface_destroy(gbmSurface);
    gbm_device_destroy(gbmDevice);
}
#endif

#if !defined(DRIVER_GLFW)
static bool bHostInited = false;
static void initHost() {
    if (bHostInited)
        return;

    #if defined(DRIVER_LEGACY)
    bcm_host_init();

    #elif defined(DRIVER_FAKE_KMS)
    // You can try chaning this to "card0" if "card1" does not work.
    device = open(device_screen.c_str(), O_RDWR | O_CLOEXEC);

    drmModeRes *resources = drmModeGetResources(device);
    if (resources == NULL) {
        std::cerr << "Unable to get DRM resources" << std::endl;
        return EXIT_FAILURE;
    }

    drmModeConnector *connector = getConnector(resources);
    if (connector == NULL) {
        std::cerr << "Unable to get connector" << std::endl;
        drmModeFreeResources(resources);
        return EXIT_FAILURE;
    }

    connectorId = connector->connector_id;
    mode = connector->modes[0];

    drmModeEncoder *encoder = findEncoder(resources, connector);
    if (connector == NULL) {
        std::cerr << "Unable to get encoder" << std::endl;
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        return EXIT_FAILURE;
    }

    crtc = drmModeGetCrtc(device, encoder->crtc_id);
    drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
    gbmDevice = gbm_create_device(device);
    gbmSurface = gbm_surface_create(gbmDevice, mode.hdisplay, mode.vdisplay, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    #endif

    bHostInited = true;
}

static EGLDisplay getDisplay() {
    initHost();
    // printf("resolution: %ix%i\n", mode.hdisplay, mode.vdisplay);

    #if defined(DRIVER_LEGACY)
    return eglGetDisplay(EGL_DEFAULT_DISPLAY);

    #elif defined(DRIVER_FAKE_KMS)

    return eglGetDisplay(gbmDevice);
    #endif
}
#endif

void initGL (glm::ivec4 &_viewport, WindowStyle _style) {

    // NON GLFW
    #if !defined(DRIVER_GLFW)
        clock_gettime(CLOCK_MONOTONIC, &time_start);

        // Clear application state
        EGLBoolean result;

        display = getDisplay();
        assert(display != EGL_NO_DISPLAY);
        check();

        result = eglInitialize(display, NULL, NULL);
        assert(EGL_FALSE != result);
        check();

        // Make sure that we can use OpenGL in this EGL app.
        // result = eglBindAPI(EGL_OPENGL_API);
        result = eglBindAPI(EGL_OPENGL_ES_API);
        assert(EGL_FALSE != result);
        check();
       
        static const EGLint configAttribs[] = {
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

        static const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        EGLConfig config;
        EGLint numConfigs;

        // get an appropriate EGL frame buffer configuration
        if (eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) == EGL_FALSE) {
            std::cerr << "Failed to get EGL configs! Error: " << eglGetErrorStr() << std::endl;
            eglTerminate(display);
            #ifdef DRIVER_FAKE_KMS
            gbmClean();
            #endif
            return EXIT_FAILURE;
        }

        // create an EGL rendering context
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        if (context == EGL_NO_CONTEXT) {
            std::cerr << "Failed to create EGL context! Error: " << eglGetErrorStr() << std::endl;
            eglTerminate(display);
            #ifdef DRIVER_FAKE_KMS
            gbmClean();
            #endif
            return EXIT_FAILURE;
        }

        #ifdef DRIVER_LEGACY
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

        if (_style == HEADLESS) {
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

        #elif defined(DRIVER_FAKE_KMS)
        surface = eglCreateWindowSurface(display, config, gbmSurface, NULL);
        if (surface == EGL_NO_SURFACE) {
            std::cerr << "Failed to create EGL surface! Error: " << eglGetErrorStr() << std::endl;
            eglDestroyContext(display, context);
            eglTerminate(display);
            gbmClean();
            return EXIT_FAILURE;
        }

        #endif

        // connect the context to the surface
        result = eglMakeCurrent(display, surface, surface, context);
        assert(EGL_FALSE != result);
        check();

    // GLFW
    #else

    
        glfwSetErrorCallback([](int err, const char* msg)->void {
            std::cerr << "GLFW error 0x"<<std::hex<<err<<std::dec<<": "<<msg<<"\n";
        });
        if(!glfwInit()) {
            std::cerr << "ABORT: GLFW init failed" << std::endl;
            exit(-1);
        }

        if (_style == HEADLESS)
            glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
            
        else if (_style == ALLWAYS_ON_TOP)
            glfwWindowHint(GLFW_FLOATING, GL_TRUE);

        if (_style == FULLSCREEN) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            _viewport.z = mode->width;
            _viewport.w = mode->height;
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            window = glfwCreateWindow(_viewport.z, _viewport.w, appTitle.c_str(), monitor, NULL);
        }

        else if (_style == HOLOPLAY) {
            int count;
            GLFWmonitor **monitors = glfwGetMonitors(&count);
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[1]);
            _viewport.z = mode->width;
            _viewport.w = mode->height;
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            window = glfwCreateWindow(_viewport.z, _viewport.w, appTitle.c_str(), monitors[1], NULL);
        }
        else
            window = glfwCreateWindow(_viewport.z, _viewport.w, appTitle.c_str(), NULL, NULL);

        if (!window) {
            glfwTerminate();
            std::cerr << "ABORT: GLFW create window failed" << std::endl;
            exit(-1);
        }

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

        glfwMakeContextCurrent(window);
#ifdef PLATFORM_WINDOWS
        glewInit();
#endif//
        glfwSetWindowSizeCallback(window, [](GLFWwindow* _window, int _w, int _h) {
            setViewport(_w,_h);
        });

        glfwSetKeyCallback(window, [](GLFWwindow* _window, int _key, int _scancode, int _action, int _mods) {
            onKeyPress(_key);
        });

        // callback when a mouse button is pressed or released
        glfwSetMouseButtonCallback(window, [](GLFWwindow* _window, int button, int action, int mods) {
            if (button == GLFW_MOUSE_BUTTON_1) {
                // update mouse4 when left mouse button is pressed or released
                if (action == GLFW_PRESS && !left_mouse_button_down) {
                    left_mouse_button_down = true;
                    mouse4.x = mouse.x;
                    mouse4.y = mouse.y;
                    mouse4.z = mouse.x;
                    mouse4.w = mouse.y;
                } else if (action == GLFW_RELEASE && left_mouse_button_down) {
                    left_mouse_button_down = false;
                    mouse4.z = -mouse4.z;
                    mouse4.w = -mouse4.w;
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

        // callback when the mouse cursor enters/leaves
        glfwSetCursorEnterCallback(window, [](GLFWwindow* _window, int entered) {
            mouse.entered = (bool)entered;
        });

        // callback when the mouse cursor moves
        glfwSetCursorPosCallback(window, [](GLFWwindow* _window, double x, double y) {
            // Convert x,y to pixel coordinates relative to viewport.
            // (0,0) is lower left corner.
            y = viewport.w - y;
            x *= fPixelDensity;
            y *= fPixelDensity;
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
            if (mouse.x > viewport.z * fPixelDensity) mouse.x = viewport.z * fPixelDensity;
            if (mouse.y > viewport.w * fPixelDensity) mouse.y = viewport.w * fPixelDensity;

            // update mouse4 when cursor moves
            if (left_mouse_button_down) {
                mouse4.x = mouse.x;
                mouse4.y = mouse.y;
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
                onMouseClick(mouse.x, mouse.y, mouse.button);
            }
            else {
                mouse.button = button;
            }

            if (mouse.velX != 0.0 || mouse.velY != 0.0) {
                if (button != 0) onMouseDrag(mouse.x, mouse.y, mouse.button);
                else onMouseMove(mouse.x, mouse.y);
            }
        });

        glfwSetWindowPosCallback(window, [](GLFWwindow* _window, int x, int y) {
            if (fPixelDensity != getPixelDensity()) {
                updateViewport();
            }
        });

        glfwSwapInterval(1);

        if (_viewport.x > 0 || _viewport.y > 0) {
            glfwSetWindowPos(window, _viewport.x, _viewport.y);
        }
    #endif
    setViewport(_viewport.z,_viewport.w);
}

bool isGL(){
 
    #if defined(DRIVER_GLFW)
        return !glfwWindowShouldClose(window);

    #elif defined(DRIVER_LEGACY)
        return bHostInited;

    #elif defined(DRIVER_FAKE_KMS)
        return true;

    #endif
}

#if defined(DRIVER_GLFW) 
void debounceSetWindowTitle(std::string title){
    static double lastUpdated;

    double now = glfwGetTime();

    if ((now - lastUpdated) < 1.) {
        return;
    }

    glfwSetWindowTitle(window, title.c_str());

    lastUpdated = now;
}
#endif

void updateGL(){
    // Update time
    // --------------------------------------------------------------------

    #if defined(DRIVER_GLFW)
        double now = glfwGetTime();

        float diff = now - fTime;
        if (diff < fRestSec) {
            pal_sleep(int((fRestSec - diff) * 1000000));
            now = glfwGetTime();
        }

    #else 
        // NON GLFW (VC or GBM) 
        double now = getTimeSec();       
    
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
        #if defined(DRIVER_GLFW)
        std::string title = appTitle + ":..: FPS:" + toString(fFPS);
        debounceSetWindowTitle(title);
        glfwPollEvents();
        
        #else
        const int XSIGN = 1<<4, YSIGN = 1<<5;
        static int fd = -1;
        if (fd<0) {
            fd = open(device_mouse.c_str(),O_RDONLY|O_NONBLOCK);
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
    #endif
}

void renderGL(){
    // NON GLFW
#if defined(DRIVER_GLFW)
    glfwSwapBuffers(window);

#else
    eglSwapBuffers(display, surface);
    #if defined(DRIVER_FAKE_KMS)
    gbmSwapBuffers();
    #endif

#endif
}

void closeGL(){
    // NON GLFW
    #if defined(DRIVER_GLFW)
        glfwSetWindowShouldClose(window, GL_TRUE);
        glfwTerminate();

    #else
        eglSwapBuffers(display, surface);

        // Release OpenGL resources
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(display, surface);
        eglDestroyContext(display, context);
        eglTerminate(display);
        eglReleaseThread();

        #if defined(DRIVER_LEGACY)
        vc_dispmanx_display_close(dispman_display);
        bcm_host_deinit();

        #elif defined(DRIVER_FAKE_KMS)
        gbmClean();
        close(device);
        #endif

    #endif
}
//-------------------------------------------------------------
void updateViewport() {
    fPixelDensity = getPixelDensity();
    glViewport( (float)viewport.x * fPixelDensity, (float)viewport.y * fPixelDensity,
                (float)viewport.z * fPixelDensity, (float)viewport.w * fPixelDensity);
    orthoMatrix = glm::ortho(   (float)viewport.x * fPixelDensity, (float)viewport.z * fPixelDensity, 
                                (float)viewport.y * fPixelDensity, (float)viewport.w * fPixelDensity);

    onViewportResize(getWindowWidth(), getWindowHeight());
}

void setViewport(float _width, float _height) {
    viewport.z = _width;
    viewport.w = _height;
    updateViewport();
}

void setWindowSize(int _width, int _height) {
#if defined(DRIVER_GLFW)
    glfwSetWindowSize(window, _width, _height);
#endif
    setViewport(_width, _height);
}

glm::ivec2 getScreenSize() {
    glm::ivec2 screen;

    #if defined(DRIVER_GLFW)
        // glfwGetMonitorPhysicalSize(glfwGetPrimaryMonitor(), &screen.x, &screen.y);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        screen.x = mode->width;
        screen.y = mode->height;

    #elif defined(DRIVER_LEGACY)
        if (!bHostInited)
            initHost();

        uint32_t screen_width;
        uint32_t screen_height;
        int32_t success = graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);
        assert(success >= 0);
        screen = glm::ivec2(screen_width, screen_height);

    #elif defined(DRIVER_FAKE_KMS)
        if (!bHostInited)
            initHost();

        screen = glm::ivec2(mode.hdisplay, mode.vdisplay);

    #endif

    return screen;
}

float getPixelDensity() {
    #if defined(DRIVER_GLFW)
        int window_width, window_height, framebuffer_width, framebuffer_height;
        glfwGetWindowSize(window, &window_width, &window_height);
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
        return float(framebuffer_width)/float(window_width);
    #else
        return 1.;
    #endif
}

glm::ivec4 getViewport() {
    return viewport;
}

int getWindowWidth() {
    return viewport.z * fPixelDensity;
}

int getWindowHeight() {
    return viewport.w * fPixelDensity;
}

glm::mat4 getOrthoMatrix() {
    return orthoMatrix;
}



glm::vec4 getDate() {
#ifdef _MSC_VER
    time_t tv = time(NULL);

    struct tm tm_struct;
    struct tm* tm = &tm_struct;
    errno_t err = localtime_s(tm, &tv);
    if (err)
    {
              
    }

    return glm::vec4(tm->tm_year + 1900,
        tm->tm_mon,
        tm->tm_mday,
        tm->tm_hour * 3600.0f + tm->tm_min * 60.0f + tm->tm_sec);
#else
    gettimeofday(&tv, NULL);
    struct tm *tm;
    tm = localtime(&tv.tv_sec);
    // std::cout << "y: " << tm->tm_year+1900 << " m: " << tm->tm_mon << " d: " << tm->tm_mday << " s: " << tm->tm_hour*3600.0f+tm->tm_min*60.0f+tm->tm_sec+tv.tv_usec*0.000001 << std::endl;
    return glm::vec4(tm->tm_year + 1900,
        tm->tm_mon,
        tm->tm_mday,
        tm->tm_hour * 3600.0f + tm->tm_min * 60.0f + tm->tm_sec + tv.tv_usec * 0.000001);
#endif 
  
}

double getTime() {
    return fTime;
}

double getDelta() {
    return fDelta;
}

void setFps(int _fps) {
    fRestSec = 1.0f/(float)_fps;
}

double getFps() {
    return fFPS;
}

float  getRestSec() {
    return fRestSec;
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

glm::vec4 getMouse4() {
    return mouse4;
}

bool getMouseEntered(){
    return mouse.entered;
}
