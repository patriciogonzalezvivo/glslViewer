#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>

#include "bcm_host.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "utils.h"
#include "shader.h"

#include "inotify-cxx.h"

typedef struct {
    uint32_t screen_width;
    uint32_t screen_height;

    int mouse_x;
    int mouse_y;

    // OpenGL|ES objects
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

} CUBE_STATE_T;
static CUBE_STATE_T _state, *state=&_state;

bool* fragHasChanged;
std::string fragFile;
std::string vertSource =
"attribute vec4 a_position;"
"varying vec2 v_texcoord;"
"void main(void) {"
"    gl_Position = a_position;"
"    v_texcoord = a_position.xy*0.5+0.5;"
"}";

GLuint quadBuffer;

Shader shader;

static void init_ogl(CUBE_STATE_T *state){
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
    dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
                                               0/*layer*/, &dst_rect, 0/*src*/,
                                               &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
    
    nativewindow.element = dispman_element;
    nativewindow.width = state->screen_width;
    nativewindow.height = state->screen_height;
    vc_dispmanx_update_submit_sync( dispman_update );
    
    
    state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
    assert(state->surface != EGL_NO_SURFACE);
    
    
    // connect the context to the surface
    result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
    assert(EGL_FALSE != result);
    
    // Set background color and clear buffers
    glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT );
}

void setup(CUBE_STATE_T *state) {
    //  Build shader;
    //
    std::string fragSource;
    if(!loadFromPath(fragFile, &fragSource)) {
        return;
    }
    shader.build(fragSource,vertSource);

    //  Make Quad
    //
    static const GLfloat vertex_data[] = {
        -1.0,-1.0,1.0,1.0,
        1.0,-1.0,1.0,1.0,
        1.0,1.0,1.0,1.0,
        -1.0,1.0,1.0,1.0
    };
    GLint posAttribut = shader.getAttribLocation("a_position");
   
    glClearColor ( 0.0, 1.0, 1.0, 1.0 );
    glGenBuffers(1, &quadBuffer);
    
    // Prepare viewport
    glViewport( 0, 0, state->screen_width, state->screen_height );
    
    // Upload vertex data to a buffer
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data),
                 vertex_data, GL_STATIC_DRAW);
    glVertexAttribPointer(posAttribut, 4, GL_FLOAT, 0, 16, 0);
    glEnableVertexAttribArray(posAttribut);  
}

static void draw(CUBE_STATE_T *state){
    if(*fragHasChanged) {
        std::string fragSource;
        if(loadFromPath(fragFile, &fragSource)){
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            shader.build(fragSource,vertSource);
            *fragHasChanged = false;
        }
    }

    // Now render to the main frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    // Clear the background (not really necessary I suppose)
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    shader.use();
    shader.sendUniform("u_time", ((float)clock())/CLOCKS_PER_SEC);
    shader.sendUniform("u_mouse", state->mouse_x, state->mouse_y);
    shader.sendUniform("u_resolution",state->screen_width, state->screen_height);
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glFlush();
    glFinish();
    
    eglSwapBuffers(state->display, state->surface); 
}

static int get_mouse(CUBE_STATE_T *state){
    static int fd = -1;
    const int width=state->screen_width, height=state->screen_height;
    static int x=width, y=height;
    const int XSIGN = 1<<4, YSIGN = 1<<5;
    if (fd<0) {
       fd = open("/dev/input/mouse0",O_RDONLY|O_NONBLOCK);
    }
    if (fd>=0) {
        struct {char buttons, dx, dy; } m;
        while (1) {
           int bytes = read(fd, &m, sizeof m);
           if (bytes < (int)sizeof m) goto _exit;
           if (m.buttons&8) {
              break; // This bit should always be set
           }
           read(fd, &m, 1); // Try to sync up again
        }
        if (m.buttons&3)
           return m.buttons&3;
        x+=m.dx;
        y+=m.dy;
        if (m.buttons&XSIGN)
           x-=256;
        if (m.buttons&YSIGN)
           y-=256;
        if (x<0) x=0;
        if (y<0) y=0;
        if (x>width) x=width;
        if (y>height) y=height;
   }

_exit:
   state->mouse_x = x;
   state->mouse_y = y;
   return 0;
}     

void drawThread() {
    bcm_host_init();

    // Clear application state
    memset( state, 0, sizeof( *state ) );
      
    // Start OGLES
    init_ogl(state);

    setup(state);
    while (1) {
        if (get_mouse(state)) break;
        draw(state);
    }
}  

void watchThread(const std::string& _file) {
    
    unsigned found = _file.find_last_of("/\\");

    std::string folder = _file.substr(0,found);
    std::string file = _file.substr(found+1);

    if(folder == file){
        folder = ".";
    }
    
    std::cout << "Watching on folder " << folder << " for file " << file << std::endl;


    Inotify notify;
    InotifyWatch watch(folder, IN_MODIFY);//IN_ALL_EVENTS);
    notify.Add(watch);

    for (;;) {

        if(!(*fragHasChanged)){
            notify.WaitForEvents();

            size_t count = notify.GetEventCount();
            while(count-- > 0) {
                InotifyEvent event;
                bool got_event = notify.GetEvent(&event);

                if(got_event){  
                    std::string mask_str;
                    event.DumpTypes(mask_str);
                    std::string filename = event.GetName();
                    std::cout << "Child: " << filename << " have change " << mask_str << std::endl;
                    if (filename == file){
                        *fragHasChanged = true;
                    }
                }
            } 
        }
    }
}
 
//==============================================================================

int main(int argc, char **argv){
    fragFile = std::string(argv[1]);

    int shmId = shmget(IPC_PRIVATE, sizeof(bool), 0666);
    pid_t pid = fork();
    fragHasChanged = (bool *) shmat(shmId, NULL, 0);

    switch(pid) {
        case -1: //error
        break;

        case 0: // child
        {
            *fragHasChanged = false;
            watchThread(fragFile);
        }
        break;

        default: 
        {
            drawThread();

            kill(pid, SIGKILL);
        }
        break;
    }

    shmctl(shmId, IPC_RMID, NULL);
    return 0;
}

