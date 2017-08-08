#include <sys/stat.h> 
#include <unistd.h>

#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

#include "app.h"
#include "utils.h"
#include "gl/shader.h"
#include "gl/vbo.h"
#include "gl/texture.h"
#include "gl/pingpong.h"
#include "gl/uniform.h"
#include "3d/camera.h"
#include "types/shapes.h"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/rotate_vector.hpp"

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
std::mutex filesMutex;
int fileChanged;

std::vector<std::string> defines;

UniformList uniforms;
std::mutex uniformsMutex;

//  SHADER
Shader shader;
int iFrag = -1;
std::string fragSource = "";
const std::string* fragPath = nullptr;
int iVert = -1;
std::string vertSource = "";
const std::string* vertPath = nullptr;

//  CAMERA
Camera cam;
float lat = 180.0;
float lon = 0.0;
glm::mat3 u_view2d = glm::mat3(1.);
// These are the 'view3d' uniforms.
// Note: the up3d vector must be orthogonal to (eye3d - centre3d),
// or rotation doesn't work correctly.
glm::vec3 u_centre3d = glm::vec3(0.,0.,0.);
// The following initial value for 'eye3d' is derived by starting with [0,0,6],
// then rotating 30 degrees around the X and Y axes.
glm::vec3 u_eye3d = glm::vec3(2.598076,3.0,4.5);
// The initial value for up3d is derived by starting with [0,1,0], then
// applying the same rotations as above, so that up3d is orthogonal to eye3d.
glm::vec3 u_up3d = glm::vec3(-0.25,0.866025,-0.433013);

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

//================================================================= Threads
void fileWatcherThread();
void cinWatcherThread();

//================================================================= Functions
void setup();
void draw();

void screenshot();

void onFileChange(int index);
void onExit();

// Main program
//============================================================================
int main(int argc, char **argv){

    // Set the size
    glm::ivec4 windowPosAndSize = glm::ivec4(0.);
    #ifdef PLATFORM_RPI
        // RASPBERRYPI default windows size (fullscreen)
        glm::ivec2 screen = getScreenSize();
        windowPosAndSize.z = screen.x;
        windowPosAndSize.w = screen.y;
    #else
        // OSX/LINUX default windows size
        windowPosAndSize.z = 500;
        windowPosAndSize.w = 500;
    #endif

    bool headless = false;
    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);

        if (        std::string(argv[i]) == "-x" ) {
            i++;
            windowPosAndSize.x = getInt(std::string(argv[i]));
        } 
        else if (   std::string(argv[i]) == "-y" ) {
            i++;
            windowPosAndSize.y = getInt(std::string(argv[i]));
        } 
        else if (   std::string(argv[i]) == "-w" || 
                    std::string(argv[i]) == "--width" ) {
            i++;
            windowPosAndSize.z = getInt(std::string(argv[i]));
        } 
        else if (   std::string(argv[i]) == "-h" || 
                    std::string(argv[i]) == "--height") {
            i++;
            windowPosAndSize.w = getInt(std::string(argv[i]));
        }
        else if (   std::string(argv[i]) == "--headless" ) {
            headless = true;
        }
        #ifdef PLATFORM_RPI 
        else if (   std::string(argv[i]) == "-l" || 
                    std::string(argv[i]) == "--life-coding" ){
            windowPosAndSize.x = windowPosAndSize.z-500;
            windowPosAndSize.z = windowPosAndSize.w = 500;
        }
        #endif
    }

    // Initialize openGL context
    initGL (windowPosAndSize, headless);
    
    Cursor cursor;  // Cursor
    struct stat st; // for files to watch
    float timeLimit = -1.0f; //  Time limit
    int textureCounter = 0; // Number of textures to load

    // Adding default deines
    defines.push_back("GLSLVIEWER 1")
    // Define PLATFORM
    #ifdef PLATFORM_OSX
    defines.push_back("PLATFORM_OSX");
    #endif

    #ifdef PLATFORM_LINUX
    defines.push_back("PLATFORM_LINUX)";
    #endif

    #ifdef PLATFORM_RPI
    defines.push_back("PLATFORM_RPI");
    #endif

    //Load the the resources (textures)
    for (int i = 1; i < argc ; i++){
        std::string argument = std::string(argv[i]);

        if (argument == "-x" || argument == "-y" || 
            argument == "-w" || argument == "--width" || 
            argument == "-h" || argument == "--height" ) {
            i++;
        }
        else if (argument == "--headless") {
        } 
        else if (argument == "-l") {
        } 
        else if (argument == "-m") {
            cursor.init();
        } 
        else if (argument == "-s" || argument == "--sec") {
            i++;
            argument = std::string(argv[i]);
            timeLimit = getFloat(argument);
            std::cout << "Will exit in " << timeLimit << " seconds." << std::endl;
        } 
        else if (argument == "-o") {
            i++;
            argument = std::string(argv[i]);
            if (haveExt(argument, "png")){
                outputFile = argument;
                std::cout << "Will save screenshot to " << outputFile  << " on exit." << std::endl; 
            }
            else {
                std::cout << "At the moment screenshots only support PNG formats" << std::endl;
            }
        } 
        else if (iFrag == -1 && (haveExt(argument,"frag") || haveExt(argument,"fs"))) {
            if (stat(argument.c_str(), &st) != 0) {
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
            if (stat(argument.c_str(), &st) != 0) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                WatchFile file;
                file.type = "vertex";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
                iVert = files.size()-1;
            }
        } 
        else if (iGeom == -1 && (   haveExt(argument,"ply") || haveExt(argument,"PLY") ||
                                    haveExt(argument,"obj") || haveExt(argument,"OBJ"))) {
            if (stat(argument.c_str(), &st) != 0) {
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
        else if (   haveExt(argument,"png") || haveExt(argument,"PNG") ||
                    haveExt(argument,"jpg") || haveExt(argument,"JPG") || 
                    haveExt(argument,"jpeg") || haveExt(argument,"JPEG")) {
            if (stat(argument.c_str(), &st) != 0) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                Texture* tex = new Texture();

                if (tex->load(argument)) {
                    std::string name = "u_tex"+getString(textureCounter);
                    textures[name] = tex;
 
                    WatchFile file;
                    file.type = "image";
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    files.push_back(file);

                    std::cout << "Loading " << argument << " as the following uniform: " << std::endl;
                    std::cout << "    uniform sampler2D " << name  << "; // loaded"<< std::endl;
                    std::cout << "    uniform vec2 " << name  << "Resolution;"<< std::endl;
                    textureCounter++;
                }
            }
        }
        else if (argument.find("-D") == 0) {
            std::string define = argument.substr(2);
            defines.push_back(define);
        }
        else if (argument.find("-") == 0) {
            std::string parameterPair = argument.substr(argument.find_last_of('-')+1);
            i++;
            argument = std::string(argv[i]);
            if (stat(argument.c_str(), &st) != 0) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                Texture* tex = new Texture();
                if (tex->load(argument)) {
                    textures[parameterPair] = tex;

                    WatchFile file;
                    file.type = "image";
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    files.push_back(file);

                    std::cout << "Loading " << argument << " as the following uniform: " << std::endl;
                    std::cout << "    uniform sampler2D " << parameterPair  << "; // loaded"<< std::endl;
                    std::cout << "    uniform vec2 " << parameterPair  << "Resolution;"<< std::endl;
                }
            }
        }
    }

    // If no shader
    if (iFrag == -1 && iVert == -1 && iGeom == -1) {
        std::cerr << "Usage: " << argv[0] << " shader.frag [shader.vert] [mesh.(obj/.ply)] [texture.(png/jpg)] [-textureNameA texture.(png/jpg)] [-u] [-x x] [-y y] [-w width] [-h height] [-l/--livecoding] [--square] [-s seconds] [-o screenshot.png]\n";
        onExit();
        exit(EXIT_FAILURE);
    }

    // Start watchers
    fileChanged = -1;
    std::thread fileWatcher(&fileWatcherThread);
    std::thread cinWatcher(&cinWatcherThread);

    // Start working on the GL context
    setup();

    // Render Loop
    while (isGL() && bRun.load()) {
        // Update
        updateGL();
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Something change??
        if (fileChanged != -1) {
            onFileChange(fileChanged);
            filesMutex.lock();
            fileChanged = -1;
            filesMutex.unlock();
        }

        // Draw
        draw();

        // Draw Cursor
        cursor.draw();
        
        // Swap the buffers
        renderGL();

        if (timeLimit >= 0.0 && getTime() >= timeLimit) {
            bRun.store(false);
        }
    }

    // If is terminated by the windows manager, turn bRun off so the fileWatcher can stop
    if (!isGL()) {
        bRun.store(false);
    }

    onExit();

    // Wait for watchers to end
    fileWatcher.join();

    // Force cinWatcher to finish (because is waiting for input)
    pthread_t handler = cinWatcher.native_handle();
    pthread_cancel(handler);

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
                    filesMutex.lock();
                    fileChanged = i; 
                    files[i].lastChange = date;
                    filesMutex.unlock();
                }
                usleep(500000);
            }
        }
    }
}

void cinWatcherThread() {
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line == "q" || line == "quit" || line == "exit") {
            bRun.store(false);
        }
        else if (line == "fps") {
            // std::cout << getFPS() << std::endl;
            printf("%f\n",getFPS());
        }
        else if (line == "delta") {
            // std::cout << getDelta() << std::endl;
            printf("%f\n",getDelta());
        }
        else if (line == "time") {
            // std::cout << getTime() << std::endl;
            printf("%f\n",getTime());
        }
        else if (line == "date") {
            glm::vec4 date = getDate();
            std::cout << date.x << ',' << date.y << ',' << date.z << ',' << date.w << std::endl;
        }
        else if (line == "window_width") {
            std::cout << getWindowWidth() << std::endl;
        }
        else if (line == "window_height") {
            std::cout << getWindowHeight() << std::endl;
        }
        else if (line == "pixel_density") {
            std::cout << getPixelDensity() << std::endl;
        }
        else if (line == "screen_size") {
            glm::ivec2 screen_size = getScreenSize();
            std::cout << screen_size.x << ',' << screen_size.y << std::endl;
        }
        else if (line == "viewport") {
            glm::ivec4 viewport = getViewport();
            std::cout << viewport.x << ',' << viewport.y << ',' << viewport.z << ',' << viewport.w << std::endl;
        }
        else if (line == "mouse_x") {
            std::cout << getMouseX() << std::endl;
        }
        else if (line == "mouse_y") {
            std::cout << getMouseY() << std::endl;
        }
        else if (line == "mouse") {
            glm::vec2 pos = getMousePosition();
            std::cout << pos.x << "," << pos.y << std::endl;
        }
        else if (line == "view3d") {
            std::cout
                << "eye=("
                    << u_eye3d.x << "," << u_eye3d.y << "," << u_eye3d.z << ") "
                << "centre=("
                    << u_centre3d.x << "," << u_centre3d.y << ","
                    << u_centre3d.z << ") "
                << "up=("
                    << u_up3d.x << "," << u_up3d.y << "," << u_up3d.z << ")"
                << std::endl;
        }
        else {
            uniformsMutex.lock();
            parseUniforms(line, &uniforms);
            uniformsMutex.unlock();
        }
    }
}

//  MAIN RENDER Thread (needs a GL context)
//============================================================================
void setup() {
    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

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
        fragPath = &files[iFrag].path;
        fragSource = "";
        if(!loadFromPath(*fragPath, &fragSource)) {
            return;
        }
    }
    else {
        fragSource = vbo->getVertexLayout()->getDefaultFragShader();
    }

    if (iVert != -1) {
        vertPath = &files[iVert].path;
        vertSource = "";
        loadFromPath(*vertPath, &vertSource);
    }
    else {
        vertSource = vbo->getVertexLayout()->getDefaultVertShader();
    }    

    shader.load(fragSource, vertSource, defines, defines, true);
    
    cam.setViewport(getWindowWidth(), getWindowHeight());
    cam.setPosition(glm::vec3(0.0,0.0,-3.));

    buffer.allocate(getWindowWidth(), getWindowHeight(), false);

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
uniform vec2 u_resolution;\n\
\n\
void main() {\n\
    vec2 st = gl_FragCoord.xy/u_resolution.xy;\n\
    gl_FragColor = texture2D(u_buffer, st);\n\
}";
    buffer_shader.load(buffer_frag, buffer_vert, defines, false);

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void draw() {
    if (shader.needBackbuffer()) {
        buffer.swap();
        buffer.src->bind();
    }
    
    shader.use();

    // Pass uniforms
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
    if (shader.need_iMouse()) {
        shader.setUniform("iMouse", get_iMouse());
    }
    if (shader.needView2d()) {
        shader.setUniform("u_view2d", u_view2d);
    }
    if (shader.needView3d()) {
        shader.setUniform("u_eye3d", u_eye3d);
        shader.setUniform("u_centre3d", u_centre3d);
        shader.setUniform("u_up3d", u_up3d);
    }
    
    for (UniformList::iterator it=uniforms.begin(); it!=uniforms.end(); ++it) {
        shader.setUniform(it->first, it->second.value, it->second.size);
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

    // Pass Textures Uniforms
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
            shader.load(fragSource, vertSource, defines, true);
        }
    }
    else if (type == "vertex") {
        vertSource = "";
        if (loadFromPath(path, &vertSource)) {
            shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
            shader.load(fragSource, vertSource, defines, true);
        }
    }
    else if (type == "geometry") {
        // TODO
    }
    else if (type == "image") {
        for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
            if (path == it->second->getFilePath()) {
                it->second->load(path);
                break;
            }
        }
    }   
}

void onKeyPress(int _key) {
    if (_key == 's' || _key == 'S') {
        screenshot();
    }

    if (_key == 'q' || _key == 'Q') {
        bRun = false;
        bRun.store(false);
    }
}

void onMouseMove(float _x, float _y) {

}

void onMouseClick(float _x, float _y, int _button) {

}

void onScroll(float _yoffset) {
    // Vertical scroll button zooms u_view2d and view3d.
    /* zoomfactor 2^(1/4): 4 scroll wheel clicks to double in size. */
    constexpr float zoomfactor = 1.1892;
    if (_yoffset != 0) {
        float z = pow(zoomfactor, _yoffset);

        // zoom view2d
        glm::vec2 zoom = glm::vec2(z,z);
        glm::vec2 origin = {getWindowWidth()/2, getWindowHeight()/2};
        u_view2d = glm::translate(u_view2d, origin);
        u_view2d = glm::scale(u_view2d, zoom);
        u_view2d = glm::translate(u_view2d, -origin);

        // zoom view3d
        u_eye3d = u_centre3d + (u_eye3d - u_centre3d)*z;
    }
}

void onMouseDrag(float _x, float _y, int _button) {
    if (_button == 1){
        // Left-button drag is used to rotate geometry.
        float dist = glm::length(cam.getPosition());
        lat -= getMouseVelX();
        lon -= getMouseVelY()*0.5;
        cam.orbit(lat,lon,dist);
        cam.lookAt(glm::vec3(0.0));

        // Left-button drag is used to pan u_view2d.
        u_view2d = glm::translate(u_view2d, -getMouseVelocity());

        // Left-button drag is used to rotate eye3d around centre3d.
        // One complete drag across the screen width equals 360 degrees.
        constexpr double tau = 6.283185307179586;
        u_eye3d -= u_centre3d;
        u_up3d -= u_centre3d;
        // Rotate about vertical axis, defined by the 'up' vector.
        float xangle = (getMouseVelX() / getWindowWidth()) * tau;
        u_eye3d = glm::rotate(u_eye3d, -xangle, u_up3d);
        // Rotate about horizontal axis, which is perpendicular to
        // the (centre3d,eye3d,up3d) plane.
        float yangle = (getMouseVelY() / getWindowHeight()) * tau;
        glm::vec3 haxis = glm::cross(u_eye3d-u_centre3d, u_up3d);
        u_eye3d = glm::rotate(u_eye3d, -yangle, haxis);
        u_up3d = glm::rotate(u_up3d, -yangle, haxis);
        //
        u_eye3d += u_centre3d;
        u_up3d += u_centre3d;
    } else {
        // Right-button drag is used to zoom geometry.
        float dist = glm::length(cam.getPosition());
        dist += (-.008f * getMouseVelY());
        if(dist > 0.0f){
            cam.setPosition( -dist * cam.getZAxis() );
            cam.lookAt(glm::vec3(0.0));
        }

        // TODO: rotate view2d.

        // pan view3d.
        float dist3d = glm::length(u_eye3d - u_centre3d);
        glm::vec3 voff = glm::normalize(u_up3d)
            * (getMouseVelY()/getWindowHeight()) * dist3d;
        u_centre3d -= voff;
        u_eye3d -= voff;
        glm::vec3 haxis = glm::cross(u_eye3d-u_centre3d, u_up3d);
        glm::vec3 hoff = glm::normalize(haxis)
            * (getMouseVelX()/getWindowWidth()) * dist3d;
        u_centre3d += hoff;
        u_eye3d += hoff;
    }
}

void onViewportResize(int _newWidth, int _newHeight) {
    cam.setViewport(_newWidth,_newHeight);
    buffer.allocate(_newWidth,_newHeight);
}

void screenshot() {
    if (outputFile != "" && isGL()) {
        unsigned char* pixels = new unsigned char[getWindowWidth()*getWindowHeight()*4];
        glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        Texture::savePixels(outputFile, pixels, getWindowWidth(), getWindowHeight());
    }
}

void onExit() {
    // Take a screenshot if it need 
    screenshot();

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
