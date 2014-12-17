#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <map>

#include "inotify-cxx.h"

#include "gl.h"
#include "utils.h"
#include "shader.h"
#include "texture.h"

bool* fragHasChanged;
std::string fragFile;
std::string vertSource =
"attribute vec4 a_position;"
"varying vec2 v_texcoord;"
"void main(void) {"
"    gl_Position = a_position*0.5;"
"    v_texcoord = a_position.xy*0.5+0.5;"
"}";

GLuint quadBuffer;
Shader shader;
std::map<std::string,Texture*> textures;

//  Time
struct timeval tv;
unsigned long long timeStart;

//============================================================================

void setup() {

	gettimeofday(&tv, NULL);
	timeStart = (unsigned long long)(tv.tv_sec) * 1000 +
				(unsigned long long)(tv.tv_usec) / 1000; 
	
    //  Build shader;
    //
    std::string fragSource;
    if(!loadFromPath(fragFile, &fragSource)) {
        return;
    }
    shader.load(fragSource,vertSource);

    //  Make Quad
    //
    static const GLfloat vertex_data[] = {
        -1.0,-1.0,1.0,1.0,
        1.0,-1.0,1.0,1.0,
        1.0,1.0,1.0,1.0,
        -1.0,1.0,1.0,1.0
    };

    GLint posAttribut = shader.getAttribLocation("a_position");
   
    glClearColor(0.0, 1.0, 1.0, 1.0 );
    glGenBuffers(1, &quadBuffer);
    
	// Upload vertex data to a buffer
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data),
                 vertex_data, GL_STATIC_DRAW);
    glVertexAttribPointer(posAttribut, 4, GL_FLOAT, 0, 16, 0);
    glEnableVertexAttribArray(posAttribut);  
}

static void draw(){

    // GLenum error = glGetError();
    // if(error != GL_NO_ERROR){
    //     std::cerr << "Error: " << error << std::endl;
    // }

    if(*fragHasChanged) {
        std::string fragSource;
        if(loadFromPath(fragFile, &fragSource)){
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            shader.load(fragSource,vertSource);
            *fragHasChanged = false;
        }
    }

	gettimeofday(&tv, NULL);

	unsigned long long timeNow = 	(unsigned long long)(tv.tv_sec) * 1000 +
				    				(unsigned long long)(tv.tv_usec) / 1000;

 	float time = (timeNow - timeStart)*0.001;

    shader.use();
    shader.sendUniform("u_time", time);
    shader.sendUniform("u_mouse", state->mouse_x, state->mouse_y);
    shader.sendUniform("u_resolution",state->screen_width, state->screen_height);

    unsigned int index = 0;
    for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
        shader.sendUniform(it->first,it->second,index);
        index++;
    }

    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawThread() {
    
    while (true) {
		// Update
        updateMouse();

		// Draw
        draw();
		eglSwapBuffers(state->display, state->surface); 
    }
}  

static void exit_func(void){
   // clear screen
   glClear( GL_COLOR_BUFFER_BIT );
   eglSwapBuffers(state->display, state->surface);

   // Release OpenGL resources
   eglMakeCurrent( state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
   eglDestroySurface( state->display, state->surface );
   eglDestroyContext( state->display, state->context );
   eglTerminate( state->display );

   printf("\nOpenGL Closed\n");
} // exit_func()

//============================================================================

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

	if(argc < 2){
		std::cerr << "GLSL render that updates changes instantly.\n";
		std::cerr << "Usage: " << argv[0] << " shader.frag [--textureNameA=texture.png] [--textureNameB=texture.jpg]\n";

		return EXIT_FAILURE;
	}

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

            // Start OGLES
            initOpenGL();

            // Setup
            setup();

            // Clear the background (not really necessary I suppose)
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

            //Load the the resources (textures)
            for (int i=2; i<argc ; i++){
                if (std::string(argv[i]).find("-") == 0){
                    std::string parameterPair = std::string(argv[i]).substr(std::string(argv[i]).find_last_of('-')+1);
                    std::vector<std::string> parameter = splitString(parameterPair,"=");
                    if(parameter.size()==2){
                        Texture* tex = new Texture();

                        if( tex->load(parameter[1]) ){
                            textures[parameter[0]] = tex;
                        }
                    }
                }
            }

            // Render Loop
            drawThread();

            //  Kill the iNotify watcher once you finish
            kill(pid, SIGKILL);
            exit_func();
        }
        break;
    }

    shmctl(shmId, IPC_RMID, NULL);
    return 0;
}
