#include <sys/shm.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <signal.h>
#include <map>

#include "app.h"
#include "utils.h"
#include "gl/shader.h"
#include "gl/vbo.h"
#include "gl/texture.h"
#include "3d/camera.h"
#include "types/shapes.h"

#include "ui/cursor.h"

// GLOBAL VARIABLES
//============================================================================
//
static bool bPlay = true;

//  List of FILES to watch and the variable to communicate that between process
struct WatchFile {
    std::string type;
    std::string path;
    int lastChange;
};
std::vector<WatchFile> files;
int* iHasChanged;

//  SHADER
Shader shader;
int iFrag = -1;
std::string fragSource = "";
int iVert = -1;
std::string vertSource = "";

//  CAMERA
Camera cam;
float lat = 180.0;
float lon = 0.0;

//  ASSETS
Vbo* vbo;
int iGeom = -1;
glm::mat4 model_matrix = glm::mat4(1.);

std::map<std::string,Texture*> textures;
std::string outputFile = "";

//  CURSOR
Cursor cursor;

//  Time limit
float timeLimit = 0.0f;

//================================================================= Functions
void watchThread();
void onFileChange();

void renderThread(int argc, char **argv);
void setup();
void draw();
void onExit();

// Main program
//============================================================================
int main(int argc, char **argv){

    // Load files to watch
    struct stat st;
    for (uint i = 1; i < argc ; i++){
        std::string argument = std::string(argv[i]);

        if ( iFrag == -1 && ( haveExt(argument,"frag") || haveExt(argument,"fs") ) ) {
            int ierr = stat(argument.c_str(), &st);
            if (ierr != 0) {
                    std::cerr << "Error watching file " << argv[i] << std::endl;
            } else {
                WatchFile file;
                file.type = "fragment";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
                iFrag = files.size()-1;
            }
        } else if ( iVert == -1 && ( haveExt(argument,"vert") || haveExt(argument,"vs") ) ) {
            int ierr = stat(argument.c_str(), &st);
            if (ierr != 0) {
                    std::cerr << "Error watching file " << argument << std::endl;
            } else {
                WatchFile file;
                file.type = "vertex";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
                iVert = files.size()-1;
            }
        } else if ( iGeom == -1 && 
                    ( haveExt(argument,"ply") || haveExt(argument,"PLY") ||
                      haveExt(argument,"obj") || haveExt(argument,"OBJ") ) ) {
            int ierr = stat(argument.c_str(), &st);
            if (ierr != 0) {
                    std::cerr << "Error watching file " << argument << std::endl;
            } else {
                WatchFile file;
                file.type = "geometry";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
                iGeom = files.size()-1;
            }
        } else if ( haveExt(argument,"png") || haveExt(argument,"PNG") ||
                    haveExt(argument,"jpg") || haveExt(argument,"JPG") || 
                    haveExt(argument,"jpeg") || haveExt(argument,"JPEG") ){
            int ierr = stat(argument.c_str(), &st);
            if (ierr != 0) {
                    // std::cerr << "Error watching file " << argument << std::endl;
            } else {
                WatchFile file;
                file.type = "image";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
            }
        }
    }

    // If no shader
    if( iFrag == -1 && iVert == -1 && iGeom == -1) {
        std::cerr << "Usage: " << argv[0] << " shader.frag [shader.vert] [mesh.(obj/.ply)] [texture.(png/jpg)] [-textureNameA texture.(png/jpg)] [-u] [-x x] [-y y] [-w width] [-h height] [-l/--livecoding] [--square] [-s seconds] [-o screenshot.png]\n";
        return EXIT_FAILURE;
    }

    // Fork process with a shared variable
    //
    int shmId = shmget(IPC_PRIVATE, sizeof(bool), 0666);
    pid_t pid = fork();
    iHasChanged = (int *) shmat(shmId, NULL, 0);

    switch(pid) {
        case -1: //error
        break;

        case 0: // child
        {
            *iHasChanged = -1;
            watchThread();
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
            onExit();
        }
        break;
    }
    
    shmctl(shmId, IPC_RMID, NULL);
    return 0;
}

//  Watching Thread
//============================================================================
void watchThread() {
    struct stat st;
    while(1){
        for(uint i = 0; i < files.size(); i++){
            if( *iHasChanged == -1 ){
                stat(files[i].path.c_str(), &st);
                int date = st.st_mtime;
                if (date != files[i].lastChange ){
                    *iHasChanged = i;
                    files[i].lastChange = date;
                }
                usleep(500000);
            }
        }
    }
}

void renderThread(int argc, char **argv) {
    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT );
    
    // Setup
    setup();

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int textureCounter = 0;

    //Load the the resources (textures)
    for (uint i = 1; i < argc ; i++){
        std::string argument = std::string(argv[i]);

        if (argument == "-x" || argument == "-y" || 
            argument == "-w" || argument == "--width" || 
            argument == "-h" || argument == "--height" ) {
            i++;
        } else if ( argument == "--square" ||
                    argument == "-l" || 
                    argument == "--life-coding"  ) {

        } else if ( argument == "-s" || argument == "--sec") {
            i++;
            timeLimit = getFloat(argument);
            std::cout << "Will exit in " << timeLimit << " seconds." << std::endl;

        } else if ( argument == "-o" ){
            i++;
            argument = std::string(argv[i]);
            if( haveExt(argument,"png") ){
                outputFile = argument;
                std::cout << "Will save screenshot to " << outputFile  << " on exit." << std::endl; 
            } else {
                std::cout << "At the moment screenshots only support PNG formats" << std::endl;
            }

        } else if (argument.find("-") == 0) {
            std::string parameterPair = argument.substr(argument.find_last_of('-')+1);

            i++;
            argument = std::string(argv[i]);
            Texture* tex = new Texture();
            if( tex->load(argument) ){
                textures[parameterPair] = tex;
                std::cout << "Loading " << argument << " as the following uniform: " << std::endl;
                std::cout << "    uniform sampler2D u_" << parameterPair  << "; // loaded"<< std::endl;
                std::cout << "    uniform vec2 u_" << parameterPair  << "Resolution;"<< std::endl;
            }

        } else if ( haveExt(argument,"png") || haveExt(argument,"PNG") ||
                    haveExt(argument,"jpg") || haveExt(argument,"JPG") || 
                    haveExt(argument,"jpeg") || haveExt(argument,"JPEG") ) {

            Texture* tex = new Texture();
            if( tex->load(argument) ){
                std::string name = "u_tex"+getString(textureCounter);
                textures[name] = tex;
                std::cout << "Loading " << argument << " as the following uniform: " << std::endl;
                std::cout << "    uniform sampler2D " << name  << "; // loaded"<< std::endl;
                std::cout << "    uniform vec2 " << name  << "Resolution;"<< std::endl;
                textureCounter++;
            }
        }
    }

    // Render Loop
    while (isGL() && bPlay) {
        // Update
        updateGL();
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw
        draw();
        
        // Swap the buffers
        renderGL();

        if ( timeLimit > 0.0 && getTime() > timeLimit ) {
            onKeyPress('s');
            bPlay = false;
        }
    }
}

void setup() {
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    
    //  Load Geometry
    //
    if ( iGeom == -1 ){
        vbo = rect(0.0,0.0,1.0,1.0).getVbo();
    } else {
        Mesh model;
        model.load(files[iGeom].path);
        vbo = model.getVbo();
        glm::vec3 toCentroid = getCentroid(model.getVertices());
        // model_matrix = glm::scale(glm::vec3(0.001));
        model_matrix = glm::translate(-toCentroid);
    }

    //  Build shader;
    //
    if ( iFrag != -1 ) {
        fragSource = "";
        if(!loadFromPath(files[iFrag].path, &fragSource)) {
            return;
        }
    } else {
        fragSource = vbo->getVertexLayout()->getDefaultFragShader();
    }

    if ( iVert != -1 ) {
        vertSource = "";
        loadFromPath(files[iVert].path, &vertSource);
    } else {
        vertSource = vbo->getVertexLayout()->getDefaultVertShader();
    }    
    shader.load(fragSource,vertSource);
    
    cam.setViewport(getWindowWidth(),getWindowHeight());
    cam.setPosition(glm::vec3(0.0,0.0,-3.));

    cursor.init();
}

void draw(){

    // Something change??
    if(*iHasChanged != -1) {
        onFileChange();
        *iHasChanged = -1;
    }

    shader.use();
    shader.setUniform("u_time", getTime());
    shader.setUniform("u_mouse", getMouseX(), getMouseY());
    shader.setUniform("u_resolution",getWindowWidth(), getWindowHeight());

    glm::mat4 mvp = glm::mat4(1.);
    if (iGeom != -1) {
        shader.setUniform("u_eye", -cam.getPosition());
        shader.setUniform("u_normalMatrix", cam.getNormalMatrix());

        shader.setUniform("u_modelMatrix", model_matrix);
        shader.setUniform("u_viewMatrix", cam.getViewMatrix());
        shader.setUniform("u_projectionMatrix", cam.getProjectionMatrix());
        
        mvp = cam.getProjectionViewMatrix() * model_matrix;
    }
    shader.setUniform("u_modelViewProjectionMatrix", mvp);

    unsigned int index = 0;
    for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
        shader.setUniform(it->first,it->second,index);
        shader.setUniform(it->first+"Resolution",it->second->getWidth(),it->second->getHeight());
        index++;
    }

    vbo->draw(&shader);

    cursor.draw();

}

// Rendering Thread
//============================================================================
void onFileChange(){
    std::string type = files[*iHasChanged].type;
    std::string path = files[*iHasChanged].path;

    if ( type == "fragment" ){
        fragSource = "";
        if(loadFromPath(path, &fragSource)){
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            shader.load(fragSource,vertSource);
        }
    } else if ( type == "vertex" ){
        vertSource = "";
        if(loadFromPath(path, &vertSource)){
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            shader.load(fragSource,vertSource);
        }
    } else if ( type == "geometry" ){
        // TODO
    } else if ( type == "image" ){
        for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
            if ( path == it->second->getFilePath() ){
                it->second->load(path);
                break;
            }
        }
    }   
}

void onKeyPress(int _key) {
    if( _key == 'q' || _key == 'Q' || _key == 's' || _key == 'S' ){
        if (outputFile != "") {
            unsigned char* pixels = new unsigned char[getWindowWidth()*getWindowHeight()*4];
            glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            Texture::savePixels(outputFile, pixels, getWindowWidth(), getWindowHeight());
        }
    }

    if ( _key == 'q' || _key == 'Q'){
        bPlay = false;
    }
}

void onMouseMove(float _x, float _y) {

}

void onMouseClick(float _x, float _y, int _button) {

}

void onMouseDrag(float _x, float _y, int _button) {
    if (_button == 1){
        float dist = glm::length(cam.getPosition());
        lat -= getMouseVelX();
        lon -= getMouseVelY()*0.5;
        cam.orbit(lat,lon,dist);
        cam.lookAt(glm::vec3(0.0));
    } else {
        float dist = glm::length(cam.getPosition());
        dist += (-.008f * getMouseVelY());
        if(dist > 0.0f){
            cam.setPosition( -dist * cam.getZAxis() );
            cam.lookAt(glm::vec3(0.0));
        }
    }
}

void onViewportResize(int _newWidth, int _newHeight) {
    cam.setViewport(_newWidth,_newHeight);
}

void onExit() {
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // close openGL instance
    closeGL();

    // DELETE RESOURCES
    for (std::map<std::string,Texture*>::iterator i = textures.begin(); i != textures.end(); ++i) {
        delete i->second;
        i->second = NULL;
    }
    textures.clear();
    delete vbo;
}