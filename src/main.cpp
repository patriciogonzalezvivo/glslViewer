#ifdef __EMSCRIPTEN__
#define GLFW_INCLUDE_ES3
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <sys/stat.h>

#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <fstream>

#include "ada/window.h"
#include "ada/gl/gl.h"
#include "ada/tools/fs.h"
#include "ada/tools/time.h"
#include "ada/tools/text.h"
#include "ada/tools/holoplay.h"
#include "ada/shaders/defaultShaders.h"

#include "sandbox.h"
#include "types/files.h"
#include "tools/text.h"
#include "tools/console.h"

#ifdef SUPPORT_NCURSES
#include <ncurses.h>
#include <signal.h>
#endif

#define TRACK_BEGIN(A)      if (sandbox.uniforms.tracker.isRunning()) sandbox.uniforms.tracker.begin(A); 
#define TRACK_END(A)        if (sandbox.uniforms.tracker.isRunning()) sandbox.uniforms.tracker.end(A); 

std::string                 version = "2.0.5";
std::string                 about   = "GlslViewer " + version + " by Patricio Gonzalez Vivo ( http://patriciogonzalezvivo.com )"; 

// Here is where all the magic happens
Sandbox                     sandbox;

//  List of FILES to watch and the variable to communicate that between process
WatchFileList               files;
std::mutex                  filesMutex;
int                         fileChanged;

// Commands variables
CommandList                 commands;
std::mutex                  commandsMutex;
std::vector<std::string>    commandsArgs;    // Execute commands
bool                        commandsExit = false;

std::atomic<bool>           keepRunnig(true);
bool                        screensaver = false;
bool                        bTerminate = false;
bool                        fullFps = false;

void                        commandsRun(const std::string &_cmd);
void                        commandsRun(const std::string &_cmd, std::mutex &_mutex);
void                        commandsInit();

#if !defined(__EMSCRIPTEN__)
void                        printUsage(char * executableName);
void                        fileWatcherThread();
void                        cinWatcherThread();
void                        onExit();

#else

extern "C"  {
    
void command(char* c) {
    commandsArgs.push_back( std::string(c) );
}

void setFrag(char* c) {
    sandbox.setSource(FRAGMENT, std::string(c) );
    sandbox.reloadShaders(files);
}

void setVert(char* c) {
    sandbox.setSource(VERTEX, std::string(c) );
    sandbox.reloadShaders(files);
}

char* getFrag() {
    return (char*)sandbox.getSource(FRAGMENT).c_str();
}

char* getVert() {
    return (char*)sandbox.getSource(VERTEX).c_str();
}

}

#endif

// Open Sound Control
#if defined(SUPPORT_OSC)
#include <lo/lo_cpp.h>
std::mutex                  oscMutex;
int                         oscPort = 0;
#endif

#if defined(__EMSCRIPTEN__)
EM_BOOL loop (double time, void* userData) {
#else
void loop() {
#endif
    
    ada::updateGL();

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    #ifndef __EMSCRIPTEN__
    if (!bTerminate && !fullFps && !sandbox.haveChange()) {
    // If nothing in the scene change skip the frame and try to keep it at 60fps
        std::this_thread::sleep_for(std::chrono::milliseconds( ada::getRestMs() ));
        return;
    }
    #else

    if (sandbox.isReady() && commandsArgs.size() > 0) {
        for (size_t i = 0; i < commandsArgs.size(); i++) {
            commandsRun(commandsArgs[i]);
        }
        commandsArgs.clear();
    }

    #endif

    // Draw Scene
    sandbox.render();

    // Draw Cursor and 2D Debug elements
    sandbox.renderUI();

    // Finish drawing
    sandbox.renderDone();

#ifndef __EMSCRIPTEN__
    if ( bTerminate && sandbox.screenshotFile == "" )
        keepRunnig.store(false);
    else
#endif
    
    // TRACK_BEGIN("renderSwap")
    ada::renderGL();
    // TRACK_END("renderSwap")

    #if defined(__EMSCRIPTEN__)
    return true;
    #endif
}

// Main program
//============================================================================
int main(int argc, char **argv) {

    ada::WindowProperties window_properties;

    #if defined(DRIVER_GBM) 
    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);
        if ( std::string(argv[i]) == "-d" || std::string(argv[i]) == "--display") {
            if (++i < argc)
                window_properties.display = std::string(argv[i]);
            else
                std::cout << "Argument '" << argument << "' should be followed by a the display address. Skipping argument." << std::endl;
        }
    }
    #endif

    // Set the size
    glm::ivec4 window_viewport = glm::ivec4(0);
    window_viewport.z = 512;
    window_viewport.w = 512;
    #if defined(DRIVER_BROADCOM) || defined(DRIVER_GBM) 
    glm::ivec2 screen = ada::getScreenSize();
    window_viewport.z = screen.x;
    window_viewport.w = screen.y;
    #endif

    bool displayHelp = false;
    bool willLoadGeometry = false;
    bool willLoadTextures = false;
    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);
        if (        std::string(argv[i]) == "-x" ) {
            if(++i < argc)
                window_viewport.x = ada::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   std::string(argv[i]) == "-y" ) {
            if(++i < argc)
                window_viewport.y = ada::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   std::string(argv[i]) == "-w" ||
                    std::string(argv[i]) == "--width" ) {
            if(++i < argc)
                window_viewport.z = ada::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   std::string(argv[i]) == "-h" ||
                    std::string(argv[i]) == "--height" ) {
            if(++i < argc)
                window_viewport.w = ada::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   std::string(argv[i]) == "--fps" ) {
            if(++i < argc)
                ada::setFps( ada::toInt(std::string(argv[i])) );
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   std::string(argv[i]) == "--help" ) {
            displayHelp = true;
        }
        #if !defined(DRIVER_GLFW)
        else if (   std::string(argv[i]) == "--mouse") {
            if (++i < argc)
                window_properties.mouse = std::string(argv[i]);
            else
                std::cout << "Argument '" << argument << "' should be followed by a the mouse address. Skipping argument." << std::endl;
        }
        #endif
        else if (   std::string(argv[i]) == "--nocursor" ) {
            commandsArgs.push_back("cursor,off");
        }
        else if (   std::string(argv[i]) == "--headless" ) {
            window_properties.style = ada::HEADLESS;
        }
        else if (   std::string(argv[i]) == "-f" ||
                    std::string(argv[i]) == "--fullscreen" ) {
            window_properties.style = ada::FULLSCREEN;
        }
        else if (   std::string(argv[i]) == "--lenticular") {
            window_properties.style = ada::LENTICULAR;
        }
        else if (   std::string(argv[i]) == "-l" ||
                    std::string(argv[i]) == "--life-coding" ){
            #if defined(DRIVER_BROADCOM) || defined(DRIVER_GBM) 
                window_viewport.x = window_viewport.z - 512;
                window_viewport.z = window_viewport.w = 512;
            #else
                window_properties.style = ada::ALLWAYS_ON_TOP;
            #endif
        }
        else if (   std::string(argv[i]) == "-ss" ||
                    std::string(argv[i]) == "--screensaver") {
            window_properties.style = ada::FULLSCREEN;
            screensaver = true;
        }
        else if (   std::string(argv[i]) == "--msaa") {
            window_properties.msaa = 4;
        }
        else if (   std::string(argv[i]) == "--major") {
            if (++i < argc)
                window_properties.major = ada::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a the OPENGL MAJOR version. Skipping argument." << std::endl;
        }
        else if (   std::string(argv[i]) == "--minor") {
            if (++i < argc)
                window_properties.minor = ada::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a the OPENGL MINOR version. Skipping argument." << std::endl;
        }
        else if ( ( ada::haveExt(argument,"ply") || ada::haveExt(argument,"PLY") ||
                    ada::haveExt(argument,"obj") || ada::haveExt(argument,"OBJ") ||
                    ada::haveExt(argument,"stl") || ada::haveExt(argument,"STL") ||
                    ada::haveExt(argument,"glb") || ada::haveExt(argument,"GLB") ||
                    ada::haveExt(argument,"gltf") || ada::haveExt(argument,"GLTF") ) ) {
            willLoadGeometry = true;
        }
        else if (   ada::haveExt(argument,"hdr") || ada::haveExt(argument,"HDR") ||
                    ada::haveExt(argument,"png") || ada::haveExt(argument,"PNG") ||
                    ada::haveExt(argument,"tga") || ada::haveExt(argument,"TGA") ||
                    ada::haveExt(argument,"psd") || ada::haveExt(argument,"PSD") ||
                    ada::haveExt(argument,"gif") || ada::haveExt(argument,"GIF") ||
                    ada::haveExt(argument,"bmp") || ada::haveExt(argument,"BMP") ||
                    ada::haveExt(argument,"jpg") || ada::haveExt(argument,"JPG") ||
                    ada::haveExt(argument,"jpeg") || ada::haveExt(argument,"JPEG") ||
                    ada::haveExt(argument,"mov") || ada::haveExt(argument,"MOV") ||
                    ada::haveExt(argument,"mp4") || ada::haveExt(argument,"MP4") ||
                    ada::haveExt(argument,"mpeg") || ada::haveExt(argument,"MPEG") ||
                    argument.rfind("/dev/", 0) == 0 ||
                    argument.rfind("http://", 0) == 0 ||
                    argument.rfind("https://", 0) == 0 ||
                    argument.rfind("rtsp://", 0) == 0 ||
                    argument.rfind("rtmp://", 0) == 0 ) {
            willLoadTextures = true;
        }   
    }

    #ifndef __EMSCRIPTEN__
    if (displayHelp) {
        printUsage( argv[0] );
        exit(0);
    }
    #endif

    // Declare commands
    commandsInit();

    // Initialize openGL context
    ada::initGL (window_viewport, window_properties);
    #ifndef __EMSCRIPTEN__
    ada::setWindowTitle("GlslViewer");
    #endif

    struct stat st;                         // for files to watch
    int         textureCounter  = 0;        // Number of textures to load
    bool        vFlip           = true;     // Flip state

    //Load the the resources (textures)
    for (int i = 1; i < argc ; i++){
        std::string argument = std::string(argv[i]);

        if (    argument == "-x" || argument == "-y" ||
                argument == "-w" || argument == "--width" ||
                argument == "-h" || argument == "--height" ||
                argument == "-d" || argument == "--display" ||
                argument == "--major" || argument == "--minor" ||
                argument == "--mouse" || argument == "--fps" ) {
            i++;
        }
        else if (   argument == "-l" || argument == "--headless" ||
                    argument == "--msaa" ) {
        }
        else if (   std::string(argv[i]) == "-f" ||
                    std::string(argv[i]) == "--fullscreen" ) {
        }
        else if (   argument == "--verbose" ) {
            sandbox.verbose = true;
        }
        else if ( argument == "--nocursor" ) {
            sandbox.cursor = false;
        }
        else if ( argument == "--fxaa" ) {
            sandbox.fxaa = true;
        }
        #if defined(SUPPORT_OSC)
        else if ( argument== "-p" || argument == "--port" ) {
            if(++i < argc)
                oscPort = ada::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by an <osc_port>. Skipping argument." << std::endl;
        }
        #endif
        else if ( argument == "-e" ) {
            if(++i < argc)         
                commandsArgs.push_back(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a <command>. Skipping argument." << std::endl;
        }
        else if ( argument == "-E" ) {
            if(++i < argc) {
                commandsArgs.push_back(std::string(argv[i]));
                commandsExit = true;
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <command>. Skipping argument." << std::endl;
        }
        else if (argument == "--fullFps" ) {
            fullFps = true;
            ada::setFps(0);
        }
        else if ( sandbox.frag_index == -1 && (ada::haveExt(argument,"frag") || ada::haveExt(argument,"fs") ) ) {
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cout << "File " << argv[i] << " not founded. Creating a default fragment shader with that name"<< std::endl;

                std::ofstream out(argv[i]);
                if (willLoadGeometry)
                    out << ada::getDefaultSrc(ada::FRAG_DEFAULT_SCENE);
                else if (willLoadTextures)
                    out << ada::getDefaultSrc(ada::FRAG_DEFAULT_TEXTURE);
                else 
                    out << ada::getDefaultSrc(ada::FRAG_DEFAULT);
                out.close();
            }

            WatchFile file;
            file.type = FRAG_SHADER;
            file.path = argument;
            file.lastChange = st.st_mtime;
            files.push_back(file);

            sandbox.frag_index = files.size()-1;

        }
        else if ( sandbox.vert_index == -1 && ( ada::haveExt(argument,"vert") || ada::haveExt(argument,"vs") ) ) {
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cout << "File " << argv[i] << " not founded. Creating a default vertex shader with that name"<< std::endl;

                std::ofstream out(argv[i]);
                out << ada::getDefaultSrc(ada::VERT_DEFAULT_SCENE);
                out.close();
            }

            WatchFile file;
            file.type = VERT_SHADER;
            file.path = argument;
            file.lastChange = st.st_mtime;
            files.push_back(file);

            sandbox.vert_index = files.size()-1;
        }
        else if ( sandbox.geom_index == -1 && ( ada::haveExt(argument,"ply") || ada::haveExt(argument,"PLY") ||
                                                ada::haveExt(argument,"obj") || ada::haveExt(argument,"OBJ") ||
                                                ada::haveExt(argument,"stl") || ada::haveExt(argument,"STL") ||
                                                ada::haveExt(argument,"glb") || ada::haveExt(argument,"GLB") ||
                                                ada::haveExt(argument,"gltf") || ada::haveExt(argument,"GLTF") ) ) {
            if ( stat(argument.c_str(), &st) != 0) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                WatchFile file;
                file.type = GEOMETRY;
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file); 
                sandbox.geom_index = files.size()-1;
            }
        }
        else if (   argument == "-vFlip" ||
                    argument == "--vFlip" ) {
            vFlip = false;
        }
        else if (   ada::haveExt(argument,"hdr") || ada::haveExt(argument,"HDR") ||
                    ada::haveExt(argument,"png") || ada::haveExt(argument,"PNG") ||
                    ada::haveExt(argument,"tga") || ada::haveExt(argument,"TGA") ||
                    ada::haveExt(argument,"psd") || ada::haveExt(argument,"PSD") ||
                    ada::haveExt(argument,"gif") || ada::haveExt(argument,"GIF") ||
                    ada::haveExt(argument,"bmp") || ada::haveExt(argument,"BMP") ||
                    ada::haveExt(argument,"jpg") || ada::haveExt(argument,"JPG") ||
                    ada::haveExt(argument,"jpeg") || ada::haveExt(argument,"JPEG")) {

            if ( checkPattern(argument) ) {
                if ( sandbox.uniforms.addStreamingTexture("u_tex" + ada::toString(textureCounter), argument, vFlip, false) )
                    textureCounter++;
            }
            else if ( sandbox.uniforms.addTexture("u_tex" + ada::toString(textureCounter), argument, files, vFlip) )
                textureCounter++;
        } 
        else if ( argument == "--video" ) {
            if (++i < argc) {
                argument = std::string(argv[i]);
                if ( sandbox.uniforms.addStreamingTexture("u_tex" + ada::toString(textureCounter), argument, vFlip, true) )
                    textureCounter++;
            }
        }
        else if (   ada::haveExt(argument,"mov") || ada::haveExt(argument,"MOV") ||
                    ada::haveExt(argument,"mp4") || ada::haveExt(argument,"MP4") ||
                    ada::haveExt(argument,"mkv") || ada::haveExt(argument,"MKV") ||
                    ada::haveExt(argument,"mpg") || ada::haveExt(argument,"MPG") ||
                    ada::haveExt(argument,"mpeg") || ada::haveExt(argument,"MPEG") ||
                    ada::haveExt(argument,"h264") ||
                    argument.rfind("http://", 0) == 0 ||
                    argument.rfind("https://", 0) == 0 ||
                    argument.rfind("rtsp://", 0) == 0 ||
                    argument.rfind("rtmp://", 0) == 0) {
            if ( sandbox.uniforms.addStreamingTexture("u_tex" + ada::toString(textureCounter), argument, vFlip, false) )
                textureCounter++;
        }
        else if (   argument.rfind("/dev/", 0) == 0) {
            if ( sandbox.uniforms.addStreamingTexture("u_tex" + ada::toString(textureCounter), argument, vFlip, true) )
                textureCounter++;
        }
        else if ( ada::haveExt(argument,"csv") || ada::haveExt(argument,"CSV") ) {
            sandbox.uniforms.addCameraTrack(argument);
        }
        else if ( argument == "--audio" || argument == "-a" ) {
            std::string device_id = "-1"; //default device id
            // device_id is optional argument, not iterate yet
            if ((i + 1) < argc) {
                argument = std::string(argv[i + 1]);
                if (ada::isInt(argument)) {
                    device_id = argument;
                    i++;
                }
            }
            if ( sandbox.uniforms.addAudioTexture("u_tex" + ada::toString(textureCounter), device_id, vFlip, true) )
                textureCounter++;
        }
        else if ( argument == "--quilt" ) {
            if (++i < argc)
                sandbox.quilt = ada::toInt(argv[i]);
        }
        else if ( argument == "--lenticular" ) {
            if (++i < argc)
                sandbox.lenticular = std::string( argv[i] );
            else 
                sandbox.lenticular = "default";

            if (sandbox.quilt == -1) 
                sandbox.quilt = 0;
        }
        else if ( argument == "-c" || argument == "-sh" ) {
            if(++i < argc) {
                argument = std::string(argv[i]);
                sandbox.uniforms.setCubeMap(argument, files);

                if (ada::haveExt(argument,"hdr") || ada::haveExt(argument,"HDR"))
                    sandbox.addDefine("SCENE_CUBEMAP_HDR","1");

                sandbox.getScene().showCubebox = false;
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <environmental_map>. Skipping argument." << std::endl;
        }
        else if ( argument == "-C" ) {
            if(++i < argc)
            {
                argument = std::string(argv[i]);
                sandbox.uniforms.setCubeMap(argument, files);
                sandbox.getScene().showCubebox = true;
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <environmental_map>. Skipping argument." << std::endl;
        }
        else if ( argument.find("-D") == 0 ) {
            // Defines are added/remove once existing shaders
            // On multiple meshes files like OBJ, there can be multiple 
            // variations of meshes, that only get created after loading the sece
            // to work around that defines are add post-loading as argument commands
            std::string define = std::string("define,") + argument.substr(2);
            commandsArgs.push_back(define);
        }
        else if ( argument.find("-I") == 0 ) {
            std::string include = argument.substr(2);
            sandbox.include_folders.push_back(include);
        }
        else if (   argument == "-v" || 
                    argument == "--version") {
            std::cout << version << std::endl;
        }
        else if ( argument.find("-") == 0 ) {
            std::string parameterPair = argument.substr( argument.find_last_of('-') + 1 );
            if(++i < argc) {
                argument = std::string(argv[i]);

                // If it's a video file, capture device, streaming url or Image sequence
                if (ada::haveExt(argument,"mov") || ada::haveExt(argument,"MOV") ||
                    ada::haveExt(argument,"mp4") || ada::haveExt(argument,"MP4") ||
                    ada::haveExt(argument,"mpeg") || ada::haveExt(argument,"MPEG") ||
                    argument.rfind("/dev/", 0) == 0 ||
                    argument.rfind("http://", 0) == 0 ||
                    argument.rfind("https://", 0) == 0 ||
                    argument.rfind("rtsp://", 0) == 0 ||
                    argument.rfind("rtmp://", 0) == 0 ||
                    checkPattern(argument) ) {
                    sandbox.uniforms.addStreamingTexture(parameterPair, argument, vFlip, false);
                }
                // Else load it as a single texture
                else 
                    sandbox.uniforms.addTexture(parameterPair, argument, files, vFlip);
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <texture>. Skipping argument." << std::endl;
        }
    }

    if (sandbox.verbose) {
        printf("//Specs: \n");
        printf("//  - Vendor: %s\n", ada::getVendor().c_str() );
        printf("//  - Renderer: %s\n", ada::getRenderer().c_str() );
        printf("//  - Version: %s\n", ada::getGLVersion().c_str() );
        printf("//  - GLSL version: %s\n", ada::getGLSLVersion().c_str() );
        printf("//  - Extensions: %s\n", ada::getExtensions().c_str() );

        printf("//  - Implementation limits:\n");
        int param;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &param);
        std::cout << "//      + GL_MAX_TEXTURE_SIZE = " << param << std::endl;

    }

    // If no shader
    #ifndef __EMSCRIPTEN__
    if ( sandbox.frag_index == -1 && sandbox.vert_index == -1 && sandbox.geom_index == -1 ) {
        printUsage(argv[0]);
        onExit();
        exit(EXIT_FAILURE);
    }
    #endif

    sandbox.setup(files, commands);

#ifdef __EMSCRIPTEN__
    emscripten_request_animation_frame_loop(loop, 0);

    double width,  height;
    emscripten_get_element_css_size("#canvas", &width, &height);
    ada::setWindowSize(width, height);

#else

    ada::setWindowVSync(true);

    // Start watchers
    fileChanged = -1;
    std::thread fileWatcher( &fileWatcherThread );
    std::thread cinWatcher( &cinWatcherThread );

    // OSC
    #if defined(SUPPORT_OSC)
    lo::ServerThread oscServer(oscPort);
    oscServer.set_callbacks( [&st](){printf("// Listening for OSC commands on port: %i\n", oscPort);}, [](){});
    oscServer.add_method(0, 0, [](const char *path, lo::Message m) {
        std::string line;
        std::vector<std::string> address = ada::split(std::string(path), '/');
        for (size_t i = 0; i < address.size(); i++)
            line +=  ((i != 0) ? "," : "") + address[i];

        std::string types = m.types();
        lo_arg** argv = m.argv(); 
        lo_message msg = m;
        for (size_t i = 0; i < types.size(); i++) {
            if ( types[i] == 's')
                line += "," + std::string( (const char*)argv[i] );
            else if (types[i] == 'i')
                line += "," + ada::toString(argv[i]->i);
            else
                line += "," + ada::toString(argv[i]->f);
        }

        if (sandbox.verbose)
            std::cout << line << std::endl;
            
        commandsRun(line, oscMutex);
    });

    if (oscPort > 0) {
        oscServer.start();
    }
    #endif
    
    // Render Loop
    while ( ada::isGL() && keepRunnig.load() ){
        // Something change??
        if ( fileChanged != -1 ) {
            filesMutex.lock();
            sandbox.onFileChange( files, fileChanged );
            fileChanged = -1;
            filesMutex.unlock();
        }

        loop();
    }

    
    // If is terminated by the windows manager, turn keepRunnig off so the fileWatcher can stop
    if ( !ada::isGL() )
        keepRunnig.store(false);

    onExit();
    
    // Wait for watchers to end
    fileWatcher.join();

    // Force cinWatcher to finish (because is waiting for input)
    #ifndef PLATFORM_WINDOWS
    pthread_t cinHandler = cinWatcher.native_handle();
    pthread_cancel( cinHandler );
    #endif

    #if defined(SUPPORT_NCURSES)
    endwin();
    #endif

    exit(0);
#endif

    return 1;
}

// Events
//============================================================================
void ada::onKeyPress (int _key) {
    if (screensaver) {
        keepRunnig = false;
        keepRunnig.store(false);
    }
    else {
        if (_key == 'q' || _key == 'Q') {
            keepRunnig = false;
            keepRunnig.store(false);
        }
    }
}

void ada::onMouseMove(float _x, float _y) {
    if (screensaver) {
        if (sandbox.isReady()) {
            keepRunnig = false;
            keepRunnig.store(false);
        }
    }
}

void ada::onMouseClick(float _x, float _y, int _button) { }
void ada::onScroll(float _yoffset) { sandbox.onScroll(_yoffset); }
void ada::onMouseDrag(float _x, float _y, int _button) { sandbox.onMouseDrag(_x, _y, _button); }
void ada::onViewportResize(int _newWidth, int _newHeight) { sandbox.onViewportResize(_newWidth, _newHeight); }

void commandsRun(const std::string &_cmd) { commandsRun(_cmd, commandsMutex); }
void commandsRun(const std::string &_cmd, std::mutex &_mutex) {
    bool resolve = false;

    // Check if _cmd is present in the list of commands
    for (size_t i = 0; i < commands.size(); i++) {
        if (ada::beginsWith(_cmd, commands[i].trigger)) {
            // Do require mutex the thread?
            if (commands[i].mutex) _mutex.lock();

            // Execute de command
            resolve = commands[i].exec(_cmd);

            if (commands[i].mutex) _mutex.unlock();

            // If got resolved stop searching
            if (resolve) break;
        }
    }

    // If nothing match maybe the user is trying to define the content of a uniform
    if (!resolve) {
        _mutex.lock();
        sandbox.uniforms.parseLine(_cmd);
        _mutex.unlock();
    }
}

void commandsInit() {
    commands.push_back(Command("help", [&](const std::string& _line){
        if (_line == "help") {
            std::cout << "Use:\n        help,<one_of_the_following_commands>" << std::endl;
            for (size_t i = 0; i < commands.size(); i++) {
                if (i%4 == 0)
                    std::cout << std::endl;
                if (commands[i].trigger != "help")
                    std::cout << std::left << std::setw(16) << commands[i].trigger << " ";
            } 
            std::cout << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                for (size_t i = 0; i < commands.size(); i++) {
                    if (commands[i].trigger == values[1]) {
                        std::cout << commands[i].trigger << std::left << std::setw(16) << "   " << commands[i].description << std::endl;
                    }
                }
                return true;
            }
        }
        return false;
    },
    "help,<command>", "print help for one or all command", false));

    commands.push_back(Command("version", [&](const std::string& _line){ 
        if (_line == "version") {
            std::cout << version << std::endl;
            return true;
        }
        return false;
    },
    "version", "return glslViewer version", false));

    commands.push_back(Command("about", [&](const std::string& _line){ 
        if (_line == "about") {
            std::cout << about << std::endl;
            return true;
        }
        return false;
    },
    "about", "about glslViewer", false));

    commands.push_back(Command("window_width", [&](const std::string& _line){ 
        if (_line == "window_width") {
            std::cout << ada::getWindowWidth() << std::endl;
            return true;
        }
        return false;
    },
    "window_width", "return the width of the windows", false));

    commands.push_back(Command("window_height", [&](const std::string& _line){ 
        if (_line == "window_height") {
            std::cout << ada::getWindowHeight() << std::endl;
            return true;
        }
        return false;
    },
    "window_height", "return the height of the windows", false));

    commands.push_back(Command("pixel_density", [&](const std::string& _line){ 
        if (_line == "pixel_density") {
            std::cout << ada::getPixelDensity() << std::endl;
            return true;
        }
        return false;
    },
    "pixel_density", "return the pixel density", false));

    commands.push_back(Command("screen_size", [&](const std::string& _line){ 
        if (_line == "screen_size") {
            glm::ivec2 screen_size = ada::getScreenSize();
            std::cout << screen_size.x << ',' << screen_size.y << std::endl;
            return true;
        }
        return false;
    },
    "screen_size", "return the screen size", false));

    commands.push_back(Command("viewport", [&](const std::string& _line){ 
        if (_line == "viewport") {
            glm::ivec4 viewport = ada::getViewport();
            std::cout << viewport.x << ',' << viewport.y << ',' << viewport.z << ',' << viewport.w << std::endl;
            return true;
        }
        return false;
    },
    "viewport", "return the viewport size", false));

    commands.push_back(Command("mouse", [&](const std::string& _line){ 
        if (_line == "mouse") {
            glm::vec2 pos = ada::getMousePosition();
            std::cout << pos.x << "," << pos.y << std::endl;
            return true;
        }
        return false;
    },
    "mouse", "return the mouse position", false));
    
    commands.push_back(Command("fps", [&](const std::string& _line){
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 2) {
            commandsMutex.lock();
            ada::setFps( ada::toInt(values[1]) );
            commandsMutex.unlock();
            return true;
        }
        else {
            // Force the output in floats
            printf("%f\n", ada::getFps());
            return true;
        }
        return false;
    },
    "fps", "return or set the amount of frames per second", false));

    commands.push_back(Command("delta", [&](const std::string& _line){ 
        if (_line == "delta") {
            // Force the output in floats
            printf("%f\n", ada::getDelta());
            return true;
        }
        return false;
    },
    "delta", "return u_delta, the secs between frames", false));

    commands.push_back(Command("date", [&](const std::string& _line){ 
        if (_line == "date") {
            // Force the output in floats
            glm::vec4 date = ada::getDate();
            std::cout << date.x << ',' << date.y << ',' << date.z << ',' << date.w << std::endl;
            return true;
        }
        return false;
    },
    "date", "return u_date as YYYY, M, D and Secs", false));

    commands.push_back(Command("files", [&](const std::string& _line){ 
        if (_line == "files") {
            for (size_t i = 0; i < files.size(); i++) { 
                std::cout << std::setw(2) << i << "," << std::setw(12) << ada::toString(files[i].type) << "," << files[i].path << std::endl;
            }
            return true;
        }
        return false;
    },
    "files", "return a list of files", false));

    commands.push_back( Command("define", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        bool change = false;
        if (values.size() == 2) {
            std::vector<std::string> v = ada::split(values[1],' ');
            if (v.size() > 1)
                sandbox.addDefine( v[0], v[1] );
            else
                sandbox.addDefine( v[0] );
            change = true;
        }
        else if (values.size() == 3) {
            sandbox.addDefine( values[1], values[2] );
            change = true;
        }

        if (change) {
            fullFps = true;
            for (size_t i = 0; i < files.size(); i++) {
                if (files[i].type == FRAG_SHADER ||
                    files[i].type == VERT_SHADER ) {
                        filesMutex.lock();
                        fileChanged = i;
                        filesMutex.unlock();
                        std::this_thread::sleep_for(std::chrono::milliseconds( ada::getRestMs() ));
                }
            }
            fullFps = false;
        }
        return change;
    },
    "define,<KEYWORD>[,<VALUE>]", "add a define to the shader", false));

    commands.push_back( Command("undefine", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 2) {
            sandbox.delDefine( values[1] );
            fullFps = true;
            for (size_t i = 0; i < files.size(); i++) {
                if (files[i].type == FRAG_SHADER ||
                    files[i].type == VERT_SHADER ) {
                        filesMutex.lock();
                        fileChanged = i;
                        filesMutex.unlock();
                        std::this_thread::sleep_for(std::chrono::milliseconds( ada::getRestMs() ));
                }
            }
            fullFps = false;
            return true;
        }
        return false;
    },
    "undefine,<KEYWORD>", "remove a define on the shader", false));

    commands.push_back(Command("reload", [&](const std::string& _line){ 
        if (_line == "reload" || _line == "reload,all") {
            fullFps = true;
            for (size_t i = 0; i < files.size(); i++) {
                filesMutex.lock();
                fileChanged = i;
                filesMutex.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds( ada::getRestMs() ));
            }
            fullFps = false;
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2 && values[0] == "reload") {
                for (size_t i = 0; i < files.size(); i++) {
                    if (files[i].path == values[1]) {
                        filesMutex.lock();
                        fileChanged = i;
                        filesMutex.unlock();
                        return true;
                    } 
                }
            }
        }
        return false;
    },
    "reload[,<filename>]", "reload one or all files", false));

    commands.push_back(Command("frag", [&](const std::string& _line){ 
        if (_line == "frag") {
            std::cout << sandbox.getSource(FRAGMENT) << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                if (ada::isDigit(values[1])) {
                    // Line number
                    size_t lineNumber = ada::toInt(values[1]) - 1;
                    std::vector<std::string> lines = ada::split(sandbox.getSource(FRAGMENT),'\n', true);
                    if (lineNumber < lines.size()) {
                        std::cout << lineNumber + 1 << " " << lines[lineNumber] << std::endl; 
                    }
                    
                }
                else {
                    // Write shader into a file
                    std::ofstream out(values[1]);
                    out << sandbox.getSource(FRAGMENT);
                    out.close();
                }
                return true;
            }
            else if (values.size() > 2) {
                std::vector<std::string> lines = ada::split(sandbox.getSource(FRAGMENT),'\n', true);
                for (size_t i = 1; i < values.size(); i++) {
                    size_t lineNumber = ada::toInt(values[i]) - 1;
                    if (lineNumber < lines.size()) {
                        std::cout << lineNumber + 1 << " " << lines[lineNumber] << std::endl; 
                    }
                }
            }
        }
        return false;
    },
    "frag[,<filename>]", "returns or save the fragment shader source code", false));

    commands.push_back(Command("vert", [&](const std::string& _line){ 
        if (_line == "vert") {
            std::cout << sandbox.getSource(VERTEX) << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                if (ada::isDigit(values[1])) {
                    // Line number
                    size_t lineNumber = ada::toInt(values[1]) - 1;
                    std::vector<std::string> lines = ada::split(sandbox.getSource(VERTEX),'\n', true);
                    if (lineNumber < lines.size()) {
                        std::cout << lineNumber + 1 << " " << lines[lineNumber] << std::endl; 
                    }
                    
                }
                else {
                    // Write shader into a file
                    std::ofstream out(values[1]);
                    out << sandbox.getSource(VERTEX);
                    out.close();
                }
                return true;
            }
            else if (values.size() > 2) {
                std::vector<std::string> lines = ada::split(sandbox.getSource(VERTEX),'\n', true);
                for (size_t i = 1; i < values.size(); i++) {
                    size_t lineNumber = ada::toInt(values[i]) - 1;
                    if (lineNumber < lines.size()) {
                        std::cout << lineNumber + 1 << " " << lines[lineNumber] << std::endl; 
                    }
                }
            }
        }
        return false;
    },
    "vert[,<filename>]", "returns or save the vertex shader source code", false));

    commands.push_back( Command("dependencies", [&](const std::string& _line){ 
        if (_line == "dependencies") {
            for (size_t i = 0; i < files.size(); i++) { 
                if (files[i].type == GLSL_DEPENDENCY) {
                    std::cout << files[i].path << std::endl;
                }   
            }
            return true;
        }
        else if (_line == "dependencies,frag") {
            sandbox.printDependencies(FRAGMENT);
            return true;
        }
        else if (_line == "dependencies,vert") {
            sandbox.printDependencies(VERTEX);
            return true;
        }
        return false;
    },
    "dependencies[,<vert|frag>]", "returns all the dependencies of the vertex o fragment shader or both", false));

    commands.push_back(Command("update", [&](const std::string& _line){ 
        if (_line == "update") {
            sandbox.flagChange();
        }
        return false;
    },
    "update", "force all uniforms to be updated", false));

    commands.push_back(Command("wait", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 2) {
            if (values[0] == "wait_sec")
                std::this_thread::sleep_for(std::chrono::seconds( ada::toInt(values[1])) );
            else if (values[0] == "wait_ms")
                std::this_thread::sleep_for(std::chrono::milliseconds( ada::toInt(values[1])) );
            else if (values[0] == "wait_us")
                std::this_thread::sleep_for(std::chrono::microseconds( ada::toInt(values[1])) );
            else
                std::this_thread::sleep_for(std::chrono::microseconds( (int)(ada::toFloat(values[1]) * 1000000) ));
        }
        return false;
    },
    "wait,<seconds>", "wait for X <seconds> before excecuting another command", false));

    commands.push_back(Command("fullFps", [&](const std::string& _line){
        if (_line == "fullFps") {
            std::string rta = fullFps ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                commandsMutex.lock();
                fullFps = (values[1] == "on");
                ada::setFps(0);
                commandsMutex.unlock();
            }
        }
        return false;
    },
    "fullFps[,on|off]", "go to full FPS or not", false));

    commands.push_back(Command("cursor", [&](const std::string& _line){
        if (_line == "cursor") {
            std::string rta = sandbox.cursor ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = ada::split(_line,',');
            if (values.size() == 2) {
                commandsMutex.lock();
                sandbox.cursor = (values[1] == "on");
                commandsMutex.unlock();
            }
        }
        return false;
    },
    "cursor[,on|off]", "show/hide cursor", false));

    commands.push_back(Command("screenshot", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() == 2) {
            commandsMutex.lock();
            sandbox.screenshotFile = values[1];
            commandsMutex.unlock();
            return true;
        }
        return false;
    },
    "screenshot[,<filename>]", "saves a screenshot to a filename", false));

    commands.push_back(Command("sequence", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() >= 3) {
            float from = ada::toFloat(values[1]);
            float to = ada::toFloat(values[2]);
            float fps = 24.0;

            if (values.size() == 4)
                fps = ada::toFloat(values[3]);

            if (from >= to) {
                from = 0.0;
            }

            commandsMutex.lock();
            sandbox.recordSecs(from, to, fps);
            commandsMutex.unlock();

            float pct = 0.0f;
            while (pct < 1.0f) {
                // get progres.
                commandsMutex.lock();
                pct = sandbox.getRecordedPercentage();
                commandsMutex.unlock();

                console_draw_pct(pct);

                std::this_thread::sleep_for(std::chrono::milliseconds( ada::getRestMs() ));
            }
            return true;
        }
        return false;
    },
    "sequence,<from_sec>,<to_sec>[,<fps>]","save a PNG sequence <from_sec> <to_sec> at <fps> (default: 24)",false));

    commands.push_back(Command("secs", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() >= 3) {
            int from = ada::toInt(values[1]);
            int to = ada::toInt(values[2]);
            float fps = 24.0;

            if (values.size() == 4)
                fps = ada::toFloat(values[3]);

            if (from >= to) {
                from = 0;
            }

            commandsMutex.lock();
            sandbox.recordSecs(from, to, fps);
            commandsMutex.unlock();

            float pct = 0.0f;
            while (pct < 1.0f) {
                // get progres.
                commandsMutex.lock();
                pct = sandbox.getRecordedPercentage();
                commandsMutex.unlock();

                console_draw_pct(pct);

                std::this_thread::sleep_for(std::chrono::milliseconds( ada::getRestMs() ));
            }
            return true;
        }
        return false;
    },
    "secs,<A>,<B>[,<fps>]","saves a sequence of images from second A to second B at <fps> (default: 24)", false));

    commands.push_back(Command("frames", [&](const std::string& _line){ 
        std::vector<std::string> values = ada::split(_line,',');
        if (values.size() >= 3) {
            float from = ada::toFloat(values[1]);
            float to = ada::toFloat(values[2]);
            float fps = 24.0;

            if (values.size() == 4)
                fps = ada::toFloat(values[3]);

            if (from >= to)
                from = 0.0;

            commandsMutex.lock();
            sandbox.recordFrames(from, to, fps);
            commandsMutex.unlock();

            float pct = 0.0f;
            while (pct < 1.0f) {

                // Check progres.
                commandsMutex.lock();
                pct = sandbox.getRecordedPercentage();
                commandsMutex.unlock();
                
                console_draw_pct(pct);

                std::this_thread::sleep_for(std::chrono::milliseconds( ada::getRestMs() ));
            }
            return true;
        }
        return false;
    },
    "frames,<A>,<B>[,<fps>]","saves a sequence of images from frame <A> to <B> at <fps> (default: 24)", false));

    commands.push_back(Command("q", [&](const std::string& _line){ 
        if (_line == "q") {
            keepRunnig.store(false);
            return true;
        }
        return false;
    },
    "q", "close glslViewer", false));

    commands.push_back(Command("quit", [&](const std::string& _line){ 
        if (_line == "quit") {
            bTerminate = true;
            // keepRunnig.store(false);
            return true;
        }
        return false;
    },
    "quit", "close glslViewer", false));

    commands.push_back(Command("exit", [&](const std::string& _line){ 
        if (_line == "exit") {
            bTerminate = true;
            // keepRunnig.store(false);
            return true;
        }
        return false;
    },
    "exit", "close glslViewer", false));
}

#ifndef __EMSCRIPTEN__

void printUsage(char * executableName) {
    std::cerr << about << "\n" << std::endl;
    std::cerr << "This is a flexible console-base OpenGL Sandbox to display 2D/3D GLSL shaders without the need of an UI. You can definitely make your own IDE or UI that communicates back/forth with glslViewer thought the standard POSIX console In/Out or OSC.\n" << std::endl;
    std::cerr << "For more information:"<< std::endl;
    std::cerr << "  - refer to repository wiki https://github.com/patriciogonzalezvivo/glslViewer/wiki"<< std::endl;
    std::cerr << "  - joing the #glslViewer channel in https://shader.zone/\n"<< std::endl;
    std::cerr << "Usage:"<< std::endl;
    std::cerr << "          " << executableName << " <frag_shader>.frag [<vert_shader>.vert <geometry>.obj|ply|stl|glb|gltf]\n" << std::endl;
    std::cerr << "Optional arguments:\n"<< std::endl;
    std::cerr << "      <texture>.(png/tga/jpg/bmp/psd/gif/hdr/mov/mp4/rtsp/rtmp/etc)   # load and assign texture to uniform u_tex<N>" << std::endl;
    std::cerr << "      -<uniform_name> <texture>.(png/tga/jpg/bmp/psd/gif/hdr)         # load a textures with a custom name" << std::endl;
    std::cerr << "      --video <video_device_number>   # open video device allocated wit that particular id" << std::endl;
    std::cerr << "      --audio [<capture_device_id>]   # open audio capture device as sampler2D texture " << std::endl;
    std::cerr << "      -C <enviromental_map>.(png/tga/jpg/bmp/psd/gif/hdr)     # load a env. map as cubemap" << std::endl;
    std::cerr << "      -c <enviromental_map>.(png/tga/jpg/bmp/psd/gif/hdr)     # load a env. map as cubemap but hided" << std::endl;
    std::cerr << "      -sh <enviromental_map>.(png/tga/jpg/bmp/psd/gif/hdr)    # load a env. map as spherical harmonics array" << std::endl;
    std::cerr << "      -vFlip                      # all textures after will be flipped vertically" << std::endl;
    std::cerr << "      -x <pixels>                 # set the X position of the billboard on the screen" << std::endl;
    std::cerr << "      -y <pixels>                 # set the Y position of the billboard on the screen" << std::endl;
    std::cerr << "      -w <pixels>                 # set the width of the window" << std::endl;
    std::cerr << "      -h <pixels>                 # set the height of the billboard" << std::endl;
    std::cerr << "      -d  or --display <display>  # open specific display port. Ex: -d /dev/dri/card1" << std::endl;
    std::cerr << "      -f  or --fullscreen         # load the window in fullscreen" << std::endl;
    std::cerr << "      -l  or --life-coding        # live code mode, where the billboard is allways visible" << std::endl;
    std::cerr << "      -ss or --screensaver        # screensaver mode, any pressed key will exit" << std::endl;
    std::cerr << "      --headless                  # headless rendering" << std::endl;
    std::cerr << "      --nocursor                  # hide cursor" << std::endl;
    std::cerr << "      --fps <fps>                 # fix the max FPS" << std::endl;
    std::cerr << "      --fxaa                      # set FXAA as postprocess filter" << std::endl;
    std::cerr << "      --quilt <0-7>               # quilt render (HoloPlay)" << std::endl;
    std::cerr << "      --lenticular [visual.json]  # lenticular calubration file, Looking Glass Model (HoloPlay)" << std::endl;
    std::cerr << "      -I<include_folder>          # add an include folder to default for #include files" << std::endl;
    std::cerr << "      -D<define>                  # add system #defines directly from the console argument" << std::endl;
    std::cerr << "      -p <OSC_port>               # open OSC listening port" << std::endl;
    std::cerr << "      -e  or -E <command>         # execute command when start. Multiple -e commands can be stack" << std::endl;
    std::cerr << "      -v  or --version            # return glslViewer version" << std::endl;
    std::cerr << "      --verbose                   # turn verbose outputs on" << std::endl;
    std::cerr << "      --help                      # print help for one or all command" << std::endl;
}

void onExit() {
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // Delete the resources of Sandbox
    sandbox.clear();

    // close openGL instance
    ada::closeGL();
}

//  Watching Thread
//============================================================================
void fileWatcherThread() {
    struct stat st;
    while ( keepRunnig.load() ) {
        for ( uint32_t i = 0; i < files.size(); i++ ) {
            if ( fileChanged == -1 ) {
                stat( files[i].path.c_str(), &st );
                int date = st.st_mtime;
                if ( date != files[i].lastChange ) {
                    filesMutex.lock();
                    files[i].lastChange = date;
                    fileChanged = i;
                    filesMutex.unlock();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds( 500 ));
    }
}

//  Command line Thread
//============================================================================
void cinWatcherThread() {

    while (!sandbox.isReady()) {
        ada::sleep_ms( ada::getRestSec() * 1000000 );
        std::this_thread::sleep_for(std::chrono::milliseconds( ada::getRestMs() ));
    }

    // Argument commands to execute comming from -e or -E
    if (commandsArgs.size() > 0) {
        for (size_t i = 0; i < commandsArgs.size(); i++) {
            commandsRun(commandsArgs[i], commandsMutex);
        }
        commandsArgs.clear();

        // If it's using -E exit after executing all commands
        if (commandsExit) {
            bTerminate = true;
            // keepRunnig.store(false);
        }
    }

    #if defined(SUPPORT_NCURSES)
    console_init();
    signal(SIGWINCH, console_sigwinch_handler);
    while ( keepRunnig.load() ) {
        std::string cmd;
        if (console_getline(cmd, commands, sandbox))
            if (cmd.size())
                commandsRun(cmd, commandsMutex);
    }
    console_end();
    #else
    // Commands coming from the console IN
    std::string cmd;
    std::cout << "// > ";
    while (std::getline(std::cin, cmd)) {
        commandsRun(cmd, commandsMutex);
        std::cout << "// > ";
    }
    #endif

}

#endif
