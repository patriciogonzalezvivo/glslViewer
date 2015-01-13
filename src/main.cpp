#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <map>

#include "inotify-cxx.h"

#include "gl.h"
#include "utils.h"
#include "shader.h"
#include "vertexLayout.h"
#include "vboMesh.h"
#include "texture.h"

bool* fragHasChanged;
std::string fragFile;
std::string vertSource =
"attribute vec4 a_position;"
"attribute vec2 a_texcoord;"
"varying vec2 v_texcoord;"
"void main(void) {"
"    gl_Position = a_position*0.5;"
"    v_texcoord = a_texcoord;"
"}";


Shader shader;
std::map<std::string,Texture*> textures;

struct PosUvVertex {
    // Position Data
    GLfloat pos_x;
    GLfloat pos_y;
    GLfloat pos_z;
    // UV Data
    GLfloat texcoord_x;
    GLfloat texcoord_y;
};

VboMesh* mesh;

//  Time
struct timeval tv;
unsigned long long timeStart;

//============================================================================

void setup(float _x, float _y, float _w, float _h) {

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

    std::vector<VertexLayout::VertexAttrib> attribs;
    attribs.push_back({"a_position", 3, GL_FLOAT, false, 0});
    attribs.push_back({"a_texcoord", 2, GL_FLOAT, false, 0});
    VertexLayout* vertexLayout = new VertexLayout(attribs);

    std::vector<PosUvVertex> vertices;
    std::vector<GLushort> indices;

    float x = mapValue(_x,0,state->screen_width,-1,1);
    float y = mapValue(_y,0,state->screen_height,-1,1);
    float w = mapValue(_w,0,state->screen_width,0,2);
    float h = mapValue(_h,0,state->screen_height,0,2);

    vertices.push_back({ x, y, 1.0, 0.0, 0.0 });
    vertices.push_back({ x+w, y, 1.0, 1.0, 0.0 });
    vertices.push_back({ x+w, y+h, 1.0, 1.0, 1.0 });
    vertices.push_back({ x, y+h, 1.0, 0.0, 1.0 });
    
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(2); indices.push_back(3); indices.push_back(0);

    mesh = new VboMesh(vertexLayout);
    mesh->addVertices((GLbyte*)vertices.data(), vertices.size());
    mesh->addIndices(indices.data(), indices.size());
}

static void draw(float _x, float _y, float _w, float _h){

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
    shader.sendUniform("u_mouse", mouse.x, mouse.y);
    shader.sendUniform("u_resolution",_w, _h);
    
    // ShaderToy Specs
    shader.sendUniform("iGlobalTime", time);
    shader.sendUniform("iMouse", mouse.x, mouse.y, (float)mouse.button);
    shader.sendUniform("iResolution",_w, _h);

    unsigned int index = 0;
    for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
        shader.sendUniform(it->first,it->second,index);
        shader.sendUniform(it->first+"Resolution",it->second->getWidth(),it->second->getHeight());
        index++;
    }

    mesh->draw(&shader);
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

   delete mesh;
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
		std::cerr << "Usage: " << argv[0] << " shader.frag [--textureNameA texture.png] [--textureNameB=texture.jpg]\n";

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

            float x = 0;
            float y = 0;
            float w = state->screen_width;
            float h = state->screen_height;

            //Load the the resources (textures)
            for (int i = 2; i < argc ; i++){

                if ( std::string(argv[i]) == "-x" ){
                    i++;
                    x = getInt(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "-y" ){
                    i++;
                    y = getInt(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "-w" || std::string(argv[i]) == "--width" ){
                    i++;
                    w = getInt(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "-h" || std::string(argv[i]) == "--height"){
                    i++;
                    h = getInt(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "-s" || std::string(argv[i]) == "--square"){
                    if(state->screen_width > state->screen_height){
                        x = state->screen_width*0.5-state->screen_height*0.5;
                    } else {
                        y = state->screen_height*0.5-state->screen_width*0.5;
                    }
                    w = h = MIN(state->screen_width,state->screen_height);
                } else if (std::string(argv[i]).find("-") == 0){
                    std::string parameterPair = std::string(argv[i]).substr(std::string(argv[i]).find_last_of('-')+1);
                    i++;
                    Texture* tex = new Texture();
                    if( tex->load(std::string(argv[i])) ){
                        textures[parameterPair] = tex;
                    }
                }
            }

            // Setup
            setup(x,y,w,h);

            // Clear the background (not really necessary I suppose)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Render Loop
            while (true) {
                // Update
                updateMouse();

                // Draw
                draw(x,y,w,h);
                eglSwapBuffers(state->display, state->surface); 
            }

            //  Kill the iNotify watcher once you finish
            kill(pid, SIGKILL);
            exit_func();
        }
        break;
    }

    shmctl(shmId, IPC_RMID, NULL);
    return 0;
}
