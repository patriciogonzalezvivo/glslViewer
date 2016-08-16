/*
Copyright (c) 2014, Patricio Gonzalez Vivo
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// #include <sys/shm.h>
#include <sys/stat.h> 
#include <unistd.h>
// #include <signal.h>

#include <map>
#include <thread>
#include <mutex>

#include "app.h"
#include "utils.h"
#include "gl/shader.h"
#include "gl/vbo.h"
#include "gl/texture.h"
#include "gl/fbo.h"
#include "gl/pingpong.h"
#include "3d/camera.h"
#include "types/shapes.h"

#include "ui/cursor.h"

// GLOBAL VARIABLES
//============================================================================
//
std::atomic<bool> bRun(true);

//  List of FILES to watch and the variable to communicate that between process
struct WatchFile {
    std::string type;
    std::string path;
    int lastChange;
};
std::vector<WatchFile> files;
int fileChanged;
std::mutex fileMutex;

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
std::string outputFile = "";

// Textures
std::map<std::string,Texture*> textures;

// Backbuffer
PingPong buffer;
Vbo* buffer_vbo;
Shader buffer_shader;

//  CURSOR
Cursor cursor;
bool bCursor = false;

//  Time limit
float timeLimit = 0.0f;

//================================================================= Functions
void fileWatcherThread();
void onFileChange(int index);

void renderThread(int argc, char **argv);
void setup();
void draw();
void onExit();

// Main program
//============================================================================
int main(int argc, char **argv){

    // Load files to watch
    struct stat st;
    for (uint i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);

        if (iFrag == -1 && (haveExt(argument,"frag") || haveExt(argument,"fs"))) {
            int ierr = stat(argument.c_str(), &st);
            if (ierr != 0) {
                    std::cerr << "Error watching file " << argv[i] << std::endl;
            }
            else {
                WatchFile file;
                file.type = "fragment";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
                iFrag = files.size()-1;
            }
        }
        else if ( iVert == -1 && ( haveExt(argument,"vert") || haveExt(argument,"vs") ) ) {
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
        }
        else if (iGeom == -1 && 
                 (haveExt(argument,"ply") || haveExt(argument,"PLY") ||
                  haveExt(argument,"obj") || haveExt(argument,"OBJ"))) {
            int ierr = stat(argument.c_str(), &st);
            if (ierr != 0) {
                    std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                WatchFile file;
                file.type = "geometry";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
                iGeom = files.size()-1;
            }
        }
        else if (haveExt(argument,"png") || haveExt(argument,"PNG") ||
                 haveExt(argument,"jpg") || haveExt(argument,"JPG") || 
                 haveExt(argument,"jpeg") || haveExt(argument,"JPEG")) {
            int ierr = stat(argument.c_str(), &st);
            if (ierr != 0) {
                    // std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                WatchFile file;
                file.type = "image";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
            }
        }
    }

    // If no shader
    if (iFrag == -1 && iVert == -1 && iGeom == -1) {
        std::cerr << "Usage: " << argv[0] << " shader.frag [shader.vert] [mesh.(obj/.ply)] [texture.(png/jpg)] [-textureNameA texture.(png/jpg)] [-u] [-x x] [-y y] [-w width] [-h height] [-l/--livecoding] [--square] [-s seconds] [-o screenshot.png]\n";
        exit(EXIT_FAILURE);
    }

    fileChanged = -1;
    std::thread fileWatcher(&fileWatcherThread);

    // OpenGL Render Loop
    renderThread(argc,argv);
    fileWatcher.join();

    exit(0);
}

//  Watching Thread
//============================================================================
void fileWatcherThread() {
    struct stat st;
    while (bRun.load()) {
        for (uint i = 0; i < files.size(); i++) {
            if (fileChanged == -1) {
                stat(files[i].path.c_str(), &st);
                int date = st.st_mtime;
                if (date != files[i].lastChange ) {
                    fileMutex.lock();
                    fileChanged = i; 
                    files[i].lastChange = date;
                    fileMutex.unlock();
                }
                usleep(500000);
            }
        }
    }
    std::cout << "End fileWatcherThread" << std::endl;
}

void renderThread(int argc, char **argv) {
    // Initialize openGL context
    initGL(argc,argv);

    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
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

        } else if ( argument == "-m" ) {
            bCursor = true;
            cursor.init();
        } else if ( argument == "-s" || argument == "--sec") {
            i++;
            argument = std::string(argv[i]);
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
                std::cout << "    uniform sampler2D " << parameterPair  << "; // loaded"<< std::endl;
                std::cout << "    uniform vec2 " << parameterPair  << "Resolution;"<< std::endl;
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
    while (isGL() && bRun.load()) {
        // Update
        updateGL();
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Something change??
        if (fileChanged != -1) {
            onFileChange(fileChanged);
            fileMutex.lock();
            fileChanged = -1;
            fileMutex.unlock();
        }

        // Draw
        draw();
        
        // Swap the buffers
        renderGL();

        if (timeLimit > 0.0 && getTime() > timeLimit) {
            onKeyPress('s');
            bRun.store(false);
        }
    }

    // If is terminated by the windows manager, turn bRun off so the fileWatcher can stop
    if (!isGL()) {
        bRun.store(false);
    }

    onExit();

    std::cout << "End renderThread" << std::endl;
}

void setup() {
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    
    //  Load Geometry
    //
    if (iGeom == -1){
        vbo = rect(0.0,0.0,1.0,1.0).getVbo();
    }
    else {
        Mesh model;
        model.load(files[iGeom].path);
        vbo = model.getVbo();
        glm::vec3 toCentroid = getCentroid(model.getVertices());
        // model_matrix = glm::scale(glm::vec3(0.001));
        model_matrix = glm::translate(-toCentroid);
    }

    //  Build shader;
    //
    if (iFrag != -1) {
        fragSource = "";
        if(!loadFromPath(files[iFrag].path, &fragSource)) {
            return;
        }
    }
    else {
        fragSource = vbo->getVertexLayout()->getDefaultFragShader();
    }

    if (iVert != -1) {
        vertSource = "";
        loadFromPath(files[iVert].path, &vertSource);
    }
    else {
        vertSource = vbo->getVertexLayout()->getDefaultVertShader();
    }    

    shader.load(fragSource,vertSource);
    
    cam.setViewport(getWindowWidth(), getWindowHeight());
    cam.setPosition(glm::vec3(0.0,0.0,-3.));

    buffer.allocate(getWindowWidth(), getWindowHeight());

    buffer_vbo = rect(0.0,0.0,1.0,1.0).getVbo();
    std::string buffer_vert = "#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
attribute vec4 a_position;\n\
\n\
void main(void) {\n\
    gl_Position = a_position;\n\
}";

std::string buffer_frag = "#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform sampler2D u_buffer;\n\
uniform sampler2D u_bufferDepth;\n\
uniform vec2 u_resolution;\n\
\n\
void main() {\n\
    vec2 st = gl_FragCoord.xy/u_resolution.xy;\n\
    // if (st.x < .5)\n\
        gl_FragColor = texture2D(u_buffer, st);\n\
    // else\n\
        // gl_FragColor = texture2D(u_bufferDepth, st);\n\
}";
    buffer_shader.load(buffer_frag, buffer_vert);
}

void draw() {
    if (shader.needBackbuffer()) {
        buffer.swap();
        buffer.src->bind();
    }
    
    shader.use();
    shader.setUniform("u_resolution",getWindowWidth(), getWindowHeight());
    if (shader.needTime()) {
        shader.setUniform("u_time", getTime());
    }
    if (shader.needDelta()) {
        shader.setUniform("u_delta", getDelta());
    }
    if (shader.needDate()) {
        shader.setUniform("u_date", getDate());
    }
    if (shader.needMouse()) {
        shader.setUniform("u_mouse", getMouseX(), getMouseY());
    }
    
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
        shader.setUniform(it->first, it->second, index);
        shader.setUniform(it->first+"Resolution", it->second->getWidth(), it->second->getHeight());
        index++;
    }

    if (shader.needBackbuffer()) {
        shader.setUniform("u_backbuffer", buffer.dst, index);
    }

    vbo->draw(&shader);

    if (shader.needBackbuffer()) {
        buffer.src->unbind();
        
        buffer_shader.use();
        buffer_shader.setUniform("u_resolution",getWindowWidth(), getWindowHeight());
        buffer_shader.setUniform("u_modelViewProjectionMatrix", mvp);
        buffer_shader.setUniform("u_buffer", buffer.src, index++);
        buffer_vbo->draw(&buffer_shader);
    }

    if(bCursor){
        cursor.draw();
    }
}

// Rendering Thread
//============================================================================
void onFileChange(int index) {
    std::string type = files[index].type;
    std::string path = files[index].path;

    if (type == "fragment") {
        fragSource = "";
        if (loadFromPath(path, &fragSource)) {
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            shader.load(fragSource,vertSource);
        }
    } else if (type == "vertex") {
        vertSource = "";
        if (loadFromPath(path, &vertSource)) {
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            shader.load(fragSource,vertSource);
        }
    } else if (type == "geometry") {
        // TODO
    } else if (type == "image") {
        for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
            if (path == it->second->getFilePath()) {
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
        bRun.store(false);
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
    buffer.allocate(_newWidth,_newHeight);
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
