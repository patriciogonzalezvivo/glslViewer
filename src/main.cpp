#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <signal.h>
#include <map>

#include <FreeImage.h>

#include "platform.h"
#include "utils.h"
#include "shader.h"
#include "vertexLayout.h"
#include "vboMesh.h"
#include "texture.h"

// Global varialbes
//============================================================================
//
bool* fragHasChanged;
std::string fragFile;
std::string fragHeader =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n"
"uniform float u_time;\n"
"uniform float iGlobalTime;\n"
"uniform vec2 u_mouse;\n"
"uniform vec4 iMouse;\n"
"uniform vec2 u_resolution;\n"
"uniform vec3 iResolution;\n"
"varying vec2 v_texcoord;\n";
bool fragAutoHeader = false;

std::string vertSource =
"attribute vec4 a_position;\n"
"attribute vec2 a_texcoord;\n"
"varying vec2 v_texcoord;\n"
"void main(void) {\n"
"    gl_Position = a_position*0.5;\n"
"    v_texcoord = a_texcoord;\n"
"}\n";

Shader shader;
VboMesh* mesh;
std::map<std::string,Texture*> textures;

//  Time
struct timeval tv;
unsigned long long timeStart;
unsigned long long timeNow;
float timeSec = 0.0f;
float timeLimit = 0.0f;

// Billboard
//============================================================================
VboMesh* rect (float _x, float _y, float _w, float _h) {
    std::vector<VertexLayout::VertexAttrib> attribs;
    attribs.push_back({"a_position", 3, GL_FLOAT, false, 0});
    attribs.push_back({"a_texcoord", 2, GL_FLOAT, false, 0});
    VertexLayout* vertexLayout = new VertexLayout(attribs);

    struct PosUvVertex {
        GLfloat pos_x;
        GLfloat pos_y;
        GLfloat pos_z;
        GLfloat texcoord_x;
        GLfloat texcoord_y;
    };

    std::vector<PosUvVertex> vertices;
    std::vector<GLushort> indices;

    float x = _x-1.0f;
    float y = _y-1.0f;
    float w = _w*2.0f;
    float h = _h*2.0f;

    vertices.push_back({ x, y, 1.0, 0.0, 0.0 });
    vertices.push_back({ x+w, y, 1.0, 1.0, 0.0 });
    vertices.push_back({ x+w, y+h, 1.0, 1.0, 1.0 });
    vertices.push_back({ x, y+h, 1.0, 0.0, 1.0 });
    
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(2); indices.push_back(3); indices.push_back(0);

    VboMesh* tmpMesh = new VboMesh(vertexLayout);
    tmpMesh->addVertices((GLbyte*)vertices.data(), vertices.size());
    tmpMesh->addIndices(indices.data(), indices.size());

    return tmpMesh;
}

// Rendering Thread
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
   
    if(fragAutoHeader){
        shader.load(fragHeader+fragSource,vertSource);
    } else {
        shader.load(fragSource,vertSource);
    }

    mesh = rect(0.0,0.0,1.0,1.0);
}

void draw(){

    if(*fragHasChanged) {
        std::string fragSource;
        if(loadFromPath(fragFile, &fragSource)){
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            if(fragAutoHeader){
                shader.load(fragHeader+fragSource,vertSource);
            } else {
                shader.load(fragSource,vertSource);
            }
            *fragHasChanged = false;
        }
    }

	gettimeofday(&tv, NULL);

    timeNow =   (unsigned long long)(tv.tv_sec) * 1000 +
                (unsigned long long)(tv.tv_usec) / 1000;

    timeSec = (timeNow - timeStart)*0.001;

    shader.use();
    shader.sendUniform("u_time", timeSec);
    shader.sendUniform("u_mouse", mouse.x, mouse.y);
    shader.sendUniform("u_resolution",viewport.width, viewport.height);
    
    // ShaderToy Specs
    shader.sendUniform("iGlobalTime", timeSec);
    shader.sendUniform("iMouse", mouse.x, mouse.y, (mouse.button==1)?1.0:0.0, (mouse.button==2)?1.0:0.0);
    shader.sendUniform("iResolution",viewport.width, viewport.height, 0.0f);

    unsigned int index = 0;
    for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
        shader.sendUniform(it->first,it->second,index);
        shader.sendUniform(it->first+"Resolution",it->second->getWidth(),it->second->getHeight());
        index++;
    }

    mesh->draw(&shader);
}

void exit_func(void){
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // close openGL instance
    closeGL();

    // delete resources
    delete mesh;
    for (std::map<std::string,Texture*>::iterator i = textures.begin(); i != textures.end(); ++i) {
        delete i->second;
        i->second = NULL;
    }
    textures.clear();
}

void renderThread(int argc, char **argv) {
    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT );
    
    resizeGL(viewport.width, viewport.height);

    // Setup
    setup();

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::string outputFile = "";
    std::string ditherOutputFile = "";

    //Load the the resources (textures)
    for (int i = 1; i < argc ; i++){
        if (std::string(argv[i]) == "-x" || 
            std::string(argv[i]) == "-y" || 
            std::string(argv[i]) == "-w" || 
            std::string(argv[i]) == "--width" || 
            std::string(argv[i]) == "-h" || 
            std::string(argv[i]) == "--height" ) {
            i++;
        } else if ( std::string(argv[i]) == "--square" ||
                    std::string(argv[i]) == "-l" || 
                    std::string(argv[i]) == "--life-coding"  ) {

        } else if ( std::string(argv[i]) == "-u" ) {
            fragAutoHeader = true;

            std::cout << "The following code is added at the top of " << fragFile << std::endl;
            std::cout << fragHeader << std::endl;

        } else if ( std::string(argv[i]) == "-d" || 
                    std::string(argv[i]) == "--dither" ) {
            i++;
            ditherOutputFile = std::string(argv[i]);
            std::cout << "Will save dither screenshot to " << ditherOutputFile << " on exit" << std::endl;

        } else if ( std::string(argv[i]) == "-s" ||
                    std::string(argv[i]) == "--sec") {
            i++;
            timeLimit = getFloat(std::string(argv[i]));
            std::cout << "Will exit in " << timeLimit << " seconds." << std::endl;

        } else if ( std::string(argv[i]) == "-o" ){

            i++;
            outputFile = std::string(argv[i]);
            std::cout << "Will save screenshot to " << outputFile  << " on exit."<< std::endl;

        } else if (std::string(argv[i]).find("-") == 0) {

            std::string parameterPair = std::string(argv[i]).substr(std::string(argv[i]).find_last_of('-')+1);
            i++;
            Texture* tex = new Texture();
            if( tex->load(std::string(argv[i])) ){
                textures[parameterPair] = tex;
                std::cout << "Loading " << std::string(argv[i]) << " uniforms are pass at: " << std::endl;
                std::cout << "    uniform sampler2D u_" << parameterPair  << "; // loaded"<< std::endl;
                std::cout << "    uniform vec2 u_" << parameterPair  << "Resolution;"<< std::endl;
            }

        }
    }

    bool bPlay = true;
    // Render Loop
    while (isGL() && bPlay) {
        // Update
        updateGL();
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw
        draw();
        
        // Swap the buffers
        renderGL();

        if( keypress == 'q' || keypress == 's'){
            std::cout << std::endl;

            if (ditherOutputFile != "") {
                unsigned char* pixels = new unsigned char[viewport.width*viewport.height*4];
                glReadPixels(0, 0, viewport.width, viewport.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                Texture::savePixels(ditherOutputFile, pixels, viewport.width, viewport.height, true );
            }

            if (outputFile != "") {
                unsigned char* pixels = new unsigned char[viewport.width*viewport.height*4];
                glReadPixels(0, 0, viewport.width, viewport.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                Texture::savePixels(outputFile, pixels, viewport.width, viewport.height, false );
            }
        }

        if ((timeLimit > 0.0 && timeSec > timeLimit) || keypress == 'q' ){
            bPlay = false;
        }
    }
}

//  Watching Thread
//============================================================================
void watchThread(const std::string& _file) {

    struct stat st;
    int ierr = stat(_file.c_str(), &st);
    if (ierr != 0) {
            std::cerr << "Error watching file " << _file << std::endl;
            return;
    }
    int date = st.st_mtime;
    while(1){
        if(!(*fragHasChanged)){

            ierr = stat(_file.c_str(), &st);
            int newdate = st.st_mtime;
            usleep(500000);
            if (newdate!=date){
                // std::cout << "Reloading " << _file << std::endl;
                *fragHasChanged = true;
                date = newdate;
            }
        }
    }
}
 
// Main program
//============================================================================
int main(int argc, char **argv){

    fragFile = "none";

    for (int i = 1; i < argc ; i++){
        if ( std::string(argv[i]).find(".frag") != std::string::npos) {
            fragFile = std::string(argv[i]);
            break;
        }
    }

    if(argc < 2 || fragFile == "none"){
		std::cerr << "GLSL render that updates changes instantly.\n";
		std::cerr << "Usage: " << argv[0] << " shader.frag [-textureNameA texture.png] [-x x] [-y y] [-w width] [-h height] [-l/--livecoding] [--square] [-s seconds] [-o screenshot.png] [-d ditheredScreenshot.png]\n";

		return EXIT_FAILURE;
	}

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
            // Initialize openGL context
            initGL(argc,argv);

            // OpenGL Render Loop
            renderThread(argc,argv);
	        
            //  Kill the iNotify watcher once you finish
            kill(pid, SIGKILL);
            exit_func();
        }
        break;
    }

    shmctl(shmId, IPC_RMID, NULL);
    return 0;
}
