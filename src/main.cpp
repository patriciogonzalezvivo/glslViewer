#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <sys/shm.h>

#include "gl.h"
#include "utils.h"
#include "shader.h"

#include "inotify-cxx.h"

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

void setup() {
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
    
	// Upload vertex data to a buffer
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data),
                 vertex_data, GL_STATIC_DRAW);
    glVertexAttribPointer(posAttribut, 4, GL_FLOAT, 0, 16, 0);
    glEnableVertexAttribArray(posAttribut);  
}

static void draw(){
    if(*fragHasChanged) {
        std::string fragSource;
        if(loadFromPath(fragFile, &fragSource)){
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            shader.build(fragSource,vertSource);
            *fragHasChanged = false;
        }
    }

    // Clear the background (not really necessary I suppose)
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    shader.use();
    shader.sendUniform("u_time", ((float)clock())/CLOCKS_PER_SEC);
    shader.sendUniform("u_mouse", state->mouse_x, state->mouse_y);
    shader.sendUniform("u_resolution",state->screen_width, state->screen_height);
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawThread() {
     
    // Start OGLES
    init_ogl();

    setup();
    while (true) {
        get_mouse();
        draw();

		eglSwapBuffers(state->display, state->surface); 
    }
}  

void watchThread(const std::string& _file) {
    
    unsigned found = _file.find_last_of("/\\");

    std::string folder = _file.substr(0,found);
    std::string file = _file.substr(found+1);

    if(folder == file){
        folder = ".";
    }
    
    //std::cout << "Watching on " << folder << " for " << file << std::endl;

    Inotify notify;
    InotifyWatch watch(folder, IN_MODIFY);
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
                    //std::cout << "Child: " << filename << " change " << mask_str << std::endl;
                    if (filename == file){
                        *fragHasChanged = true;
                    }
                }
            } 
        }
    }
}
 
//============================================================================

int main(int argc, char **argv){

	// Get path
	//
    fragFile = std::string(argv[1]);

    // Fork process with a shared variable
    //
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
