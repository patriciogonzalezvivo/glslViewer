#if defined(__EMSCRIPTEN__)
#define GLFW_INCLUDE_ES3
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <sys/stat.h>

#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#include <stdlib.h>
#endif

#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <fstream>

#include "vera/window.h"
#include "vera/gl/gl.h"
#include "vera/ops/fs.h"
#include "vera/ops/draw.h"
#include "vera/ops/math.h"
#include "vera/ops/time.h"
#include "vera/ops/pixel.h"
#include "vera/ops/string.h"
#include "vera/ops/meshes.h"
#include "vera/shaders/defaultShaders.h"
#include "vera/xr/holoPlay.h"
#include "vera/xr/xr.h"

#include "core/glslViewer.h"
#include "core/tools/files.h"
#include "core/tools/text.h"
#include "core/tools/record.h"
#include "core/tools/console.h"

#if defined(SUPPORT_NCURSES)
#include <ncurses.h>
#include <signal.h>
#endif

#if defined(DEBUG)
#define TRACK_BEGIN(A)      if (sandbox.uniforms.tracker.isRunning()) sandbox.uniforms.tracker.begin(A); 
#define TRACK_END(A)        if (sandbox.uniforms.tracker.isRunning()) sandbox.uniforms.tracker.end(A); 
#else 
#define TRACK_BEGIN(A)
#define TRACK_END(A)
#endif

// Global state variables/functions
std::string                 version = vera::toString(GLSLVIEWER_VERSION_MAJOR) + "." + vera::toString(GLSLVIEWER_VERSION_MINOR) + "." + vera::toString(GLSLVIEWER_VERSION_PATCH);
std::string                 about   = "GlslViewer " + version + " by Patricio Gonzalez Vivo ( http://patriciogonzalezvivo.com )"; 
std::string                 icon_base64 = "iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAYAAACqaXHeAAACzklEQVR4XmMYBaMhMBoCoyEwkkOAkV6e/w8EpNjFCAT0cBtNA4BUT+PyMC0Dg+oBQC1P0yswqBYAxHic2JikplmEshFVAgCfgylNvrQ0GxQ4FAUALsfRKs/Swj6yA4DenoclZWrbS1YAYHMEvaotfAFBjhtIDoDB4HlqBgJJAUBtz8PMoyT1UOomogOAVp6HxeZABQITuc1NajsYV+FGjPsocQtRKQDdcdT2PLInqWk2MWYRDABaeh7mQHrYgSslMTHQCeDzJHpMUZIdSPUO3hRArZgh1hx62wcKLKJTALl5kxRPUSslkOJWmmYBcmKU3tkBZwBQmg8pSc7UDgR8fmGidT0LM5+UAKU08EF2EpsNaJYFsDmAGI9hU0NJ24BQBGMNAGrEAK5YwGc2LT2Py16CKYDS0Cc2JdDC88S4nSpZgFCKIRQI5HieWqmU4gCAOYTcQKDE89QIBIoCAN0B5AQCeiFFKNmSaidZhSAxVSMuz1ISCKR6npwqFt1vNKkGyQkEcj3PQCGg2YAIKYFAqecpqakoSgHUcPhAep6k3iCulEbLpEuNQpVmhSCywbQIBHp4nqgUQMghsICgZiBQy/PEuB1rGUBuoUKNQKBVzONyG9WrQUoCgV7JHjn7MlHS6KFmwUhtzxObdZnI9QShgCMlJdA65vG5hSYtQVIKxoFI9iRnAZAGYpMUesqgpJVGrl5S3Eq3mSFSA5BanidkDk2zACmNJXLVMlAIRidHiQ1A9CRMSd7GlR2oaSaxZpGdBcgtFEEBjs1xtAhQYiJ3dIkMqWUItpinJPYoKcOo4RZGchwwGAKBWm5gJDcGaFGQEeMWatvLSO0kiKuQY6ACoEWgM9LSYdQIDHy1DTXKHqoEAMijxFSLxDqYmmYRimCqBQDMImIcT0mqo3aNQ/UAQPYctQKDltUsTQOAksAYqLYFwygYDYHREBgNgZEUAgDEsgRgHB+UVwAAAABJRU5ErkJggg==";
std::atomic<bool>           bKeepRunnig(true);
bool                        bScreensaverMode = false;
bool                        bRunAtFullFps = false;
bool                        bTerminate = false;
bool                        bStreamsPlaying = true;
#if !defined(__EMSCRIPTEN__)
void                        printUsage(char * executableName);
void                        onExit();
#endif

// The sandbox holds:
//  - the main shader (in 2D)
//  - the scene (when 3D geometry or vertex shaders are loaded)
//  - uniforms (all data pass to the shaders)
GlslViewer                  sandbox;

//  List of FILES to watch (code, images/video or geometry). 
// This will be use by the sandbox to hot reload or keep track of assets
WatchFileList               files;
std::mutex                  filesMutex;
#if !defined(__EMSCRIPTEN__)
void                        fileWatcherThread();
#endif


// Console COMMAND interface. A way to change the state of internal variables. 
// Note: the OSC listener reuse it to process events
CommandList                 commands;
std::mutex                  commandsMutex;
std::vector<std::string>    commandsArgs;    // Execute commands
bool                        commandsExit = false;
#if defined(SUPPORT_NCURSES)
bool                        commands_ncurses = true;
#else
bool                        commands_ncurses = false;
#endif
void                        commandsRun(const std::string &_cmd);
void                        commandsRun(const std::string &_cmd, std::mutex &_mutex);
void                        commandsInit();
#if !defined(__EMSCRIPTEN__)
// Console IN thread
void                        cinWatcherThread();
#else
// In WASM / EMSCRIPTEN projects there is nothreads, so instead of a Console IN rutine
// we expose a command function for the client, and commands are excecute in the main loop
extern "C"  {

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void command(char* c) {
    commandsArgs.push_back( std::string(c) );
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void setFrag(char* c) {
    sandbox.setSource(FRAGMENT, std::string(c) );
    sandbox.resetShaders(files);
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void resize(int width, int height) {
    vera::setWindowSize(width, height);
    // sandbox.onWindowResize(width, height);
}


void setVert(char* c) {
    sandbox.setSource(VERTEX, std::string(c) );
    sandbox.resetShaders(files);
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
#endif
int                         oscPort = 0;
// MAIN LOOP
#if defined(__EMSCRIPTEN__)
EM_BOOL loop (double time, void* userData) {
#else
void loop() {
#endif
    // update time/delta/date and input events
    vera::updateGL();
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


    #ifdef __EMSCRIPTEN__
    // Excecute commands in the main loop in WASM/EMSCRIPTEN project because there is no multiple threads
    if (sandbox.isReady() && commandsArgs.size() > 0) {
        for (size_t i = 0; i < commandsArgs.size(); i++)
            commandsRun(commandsArgs[i]);
        commandsArgs.clear();
    }
    #else
    // If nothing in the scene change skip the frame and try to keep it at 60fps
    if (!bTerminate && !bRunAtFullFps && !sandbox.haveChange()) {
        std::this_thread::sleep_for(std::chrono::milliseconds( vera::getRestMs() ));
        return;
    }
    #endif

    // PREP for main render:
    //  - update uniforms
    //  - render buffers, double buffers and pyramid convolutions
    //  - render lighmaps (3D scenes only)
    //  - render normal/possition/extra g buffers (3D scenes only)
    //  - start the recording FBO (when recording)
    sandbox.renderPrep();

    // Render the main 2D Shader on a billboard or 3D Scene when there is geometry models
    // Note: if the render require multiple views of the render (for quilts or VR) it happens here.
    sandbox.render();

    // POST render:
    //  - post-processing render pass
    //  - close recording FBO (when recording)
    sandbox.renderPost();

    // UI render pass:
    //  - draw textures (debug)
    //  - draw render passes/buffers (debug)
    //  - draw plot widget (debug)
    //  - draw cursor
    //  - draw help prompt
    sandbox.renderUI();

    // Finish rendering triggering some events like
    //  - save image/frame if it's needed
    //  - anouncing the first pass have been made
    sandbox.renderDone();

#ifndef __EMSCRIPTEN__
    if ( bTerminate && sandbox.screenshotFile == "" )
        bKeepRunnig.store(false);
    else
#endif
    
    // Swap GL buffer
    TRACK_BEGIN("render:swap")
    vera::renderGL();
    TRACK_END("render:swap")

    #if defined(__EMSCRIPTEN__)
    return (vera::getXR() == vera::NONE_XR_MODE);
    #endif
}

// Main program
//============================================================================
int main(int argc, char **argv) {

    // FIRST parsing pass through arguments to understand what kind of 
    // WINDOW PROPERTIES and general enviroment set up needs to be created.
    vera::WindowProperties window_properties;

    bool displayHelp = false;
    bool haveVertexShader = false;
    bool haveFragmentShader = false;
    bool haveGeometry = false;
    bool haveTextures = false;

    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);
        if (        argument == "-x" ) {
            if (++i < argc)
                window_properties.screen_x = vera::toInt( std::string(argv[i]) );
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   argument == "-y" ) {
            if(++i < argc)
                window_properties.screen_y = vera::toInt( std::string(argv[i]) );
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   argument == "-s"    || argument == "-size"         || argument == "--size" ) {
            if(++i < argc) {
                window_properties.screen_width = vera::toInt( std::string(argv[i]) );
                window_properties.screen_height = vera::toInt( std::string(argv[i]) );
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   argument == "-w"    || argument == "-width"         || argument == "--width" ) {
            if(++i < argc)
                window_properties.screen_width = vera::toInt( std::string(argv[i]) );
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   argument == "-h"    || argument == "-height"        || argument == "--height" ) {
            if(++i < argc)
                window_properties.screen_height = vera::toInt( std::string(argv[i]) );
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        #if defined(DRIVER_GBM) 
        else if (   argument == "-d"    || argument == "-display"       || argument == "--display") {
            if (++i < argc)
                window_properties.display = std::string(argv[i]);
            else
                std::cout << "Argument '" << argument << "' should be followed by a the display address. Skipping argument." << std::endl;
        }
        #endif
        #if !defined(DRIVER_GLFW)
        else if (   argument == "-mouse"    || argument == "--mouse") {
            if (++i < argc)
                window_properties.mouse = std::string(argv[i]);
            else
                std::cout << "Argument '" << argument << "' should be followed by a the mouse address. Skipping argument." << std::endl;
        }
        #endif
        else if (   argument == "-help"     || argument == "--help" ) {
            displayHelp = true;
        }
        else if (   argument == "-v"        || argument == "-version"       || argument == "--version" ) {
            std::cout << version << std::endl;
        }
        else if (   argument == "-l"        || argument == "-life-coding"   || argument == "--life-coding" ){
            #if defined(DRIVER_BROADCOM) || defined(DRIVER_GBM) 
                window_properties.screen_x = window_properties.screen_width - 512;
                window_properties.screen_width = window_properties.screen_height = 512;
            #else
                if (window_properties.style == vera::UNDECORATED)
                    window_properties.style = vera::UNDECORATED_ALLWAYS_ON_TOP;
                else
                    window_properties.style = vera::ALLWAYS_ON_TOP;
            #endif
        }
        else if (   argument == "-undecorated"  ||  argument == "--undecorated" ) {
            if (window_properties.style == vera::ALLWAYS_ON_TOP)
                window_properties.style = vera::UNDECORATED_ALLWAYS_ON_TOP;
            else 
                window_properties.style = vera::UNDECORATED;
        }
        else if (   argument == "-headless"     || argument == "--headless" )                                   window_properties.style = vera::HEADLESS;
        else if (   argument == "-lenticular"   || argument == "--lenticular")                                  window_properties.style = vera::LENTICULAR;
        else if (   argument == "-f"            || argument == "-fullscreen"    || argument == "--fullscreen")  window_properties.style = vera::FULLSCREEN;
        else if (   argument == "-msaa"         || argument == "--msaa")                                        window_properties.msaa = 4;
        else if (   argument == "-ss"           || argument == "-screensaver"   || argument == "--screensaver") {
            window_properties.style = vera::FULLSCREEN;
            bScreensaverMode = true;
        }
        else if (   argument == "-major"        || argument == "--major" ) {
            if (++i < argc)
                window_properties.major = vera::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a the OPENGL MAJOR version. Skipping argument." << std::endl;
        }
        else if (   argument == "-minor"        || argument == "--minor" ) {
            if (++i < argc)
                window_properties.minor = vera::toInt(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a the OPENGL MINOR version. Skipping argument." << std::endl;
        }
        else if ( vera::haveExt(argument,"vert") || vera::haveExt(argument,"vs") ) {
            haveVertexShader = true;
        }
        else if ( vera::haveExt(argument,"frag") || vera::haveExt(argument,"fs") ) {
            haveFragmentShader = true;
        }
        else if ( ( vera::haveExt(argument,"ply") || vera::haveExt(argument,"PLY") ||
                    vera::haveExt(argument,"obj") || vera::haveExt(argument,"OBJ") ||
                    vera::haveExt(argument,"stl") || vera::haveExt(argument,"STL") ||
                    vera::haveExt(argument,"glb") || vera::haveExt(argument,"GLB") ||
                    vera::haveExt(argument,"gltf") || vera::haveExt(argument,"GLTF") ||
                    vera::haveExt(argument,"splat") || vera::haveExt(argument,"SPLAT") ) ) {
            haveGeometry = true;
        }
        else if (   argument == "-noncurses"|| argument == "--noncurses"    )   commands_ncurses = false;
        else if (   vera::haveExt(argument,"csv") || vera::haveExt(argument,"csv") ) {

        }
        else if (   vera::haveExt(argument,"hdr") || vera::haveExt(argument,"HDR") ||
                    vera::haveExt(argument,"exr") || vera::haveExt(argument,"EXR") ||
                    vera::haveExt(argument,"png") || vera::haveExt(argument,"PNG") ||
                    vera::haveExt(argument,"tga") || vera::haveExt(argument,"TGA") ||
                    vera::haveExt(argument,"psd") || vera::haveExt(argument,"PSD") ||
                    vera::haveExt(argument,"gif") || vera::haveExt(argument,"GIF") ||
                    vera::haveExt(argument,"bmp") || vera::haveExt(argument,"BMP") ||
                    vera::haveExt(argument,"jpg") || vera::haveExt(argument,"JPG") ||
                    vera::haveExt(argument,"jpeg") || vera::haveExt(argument,"JPEG") ||
                    vera::haveExt(argument,"mov") || vera::haveExt(argument,"MOV") ||
                    vera::haveExt(argument,"mp4") || vera::haveExt(argument,"MP4") ||
                    vera::haveExt(argument,"mpeg") || vera::haveExt(argument,"MPEG") ||
                    argument.rfind("/dev/", 0) == 0 ||
                    argument.rfind("http://", 0) == 0 ||
                    argument.rfind("https://", 0) == 0 ||
                    argument.rfind("rtsp://", 0) == 0 ||
                    argument.rfind("rtmp://", 0) == 0 ) {
            haveTextures = true;
        }
    }

    #ifndef __EMSCRIPTEN__
    if (displayHelp || (!haveVertexShader && !haveFragmentShader && !haveGeometry && !haveTextures)) {
        printUsage( argv[0] );
        // exit(0);
    }
    #endif

    // Declare global level commands
    commandsInit();

    // Initialize openGL context
    vera::initGL(window_properties);
    #ifndef __EMSCRIPTEN__
    if (window_properties.style != vera::HEADLESS) {
        vera::setWindowTitle("GlslViewer");

        #if defined(DRIVER_GLFW) && defined(PLATFORM_LINUX)
        int icon_width, icon_height; 
        unsigned char* icon_data = vera::loadPixelsBase64(icon_base64, &icon_width, &icon_height);
        vera::setWindowIcon(icon_data, icon_width, icon_height);
        #endif

    }
    // Start Console IN listener thread
    std::thread cinWatcher( &cinWatcherThread );
    
    #endif

    struct stat st;                         // for files to watch
    int         textureCounter  = 0;        // number of textures to load
    bool        vFlip           = true;     // texture flip state 

    // SECOND parsing pass of arguments focus on the SANDBOX, 
    // by loading assets, setting up properties and excecuting commands
    for (int i = 1; i < argc ; i++){
        std::string argument = std::string(argv[i]);

        // Avoid parsing twice through windows properties arguments with values
        if (        argument == "-x"        || argument == "-y"             || 
                    argument == "-s"        || argument == "-size"          || argument == "--size"     ||
                    argument == "-w"        || argument == "-width"         || argument == "--width"    ||
                    argument == "-h"        || argument == "-height"        || argument == "--height"   ||
                #if defined(DRIVER_GBM) 
                    argument == "-d"        || argument == "-display"       || argument == "--display"  ||
                #endif
                #if !defined(DRIVER_GLFW)
                    argument == "-mouse"    || argument == "--mouse"        ||
                #endif
                    argument == "--major"   || argument == "--major"        || 
                    argument == "--minor"   || argument == "--minor"    ) {
            i++;
        }
        
        // Avoid parsing twice through windows properties arguments without values
        else if (   argument == "-l"        || argument == "-life-coding"   || argument == "--life-coding"  ||
                    argument == "-f"        || argument == "-fullscreen"    || argument == "--fullscreen"   ||
                    argument == "-ss"       || argument == "-screensaver"   || argument == "--screensaver"  ||
                    argument == "-v"        || argument == "-version"       || argument == "--version" || 
                    argument == "-headless" || argument == "--headless"     || 
                    argument == "-help"     || argument == "--help"         ||
                    argument == "-msaa"     || argument == "--msaa"         || 
                    argument == "-undecorated"  || argument == "--undecorated") {
        }

        // Change internal states with no second parameter
        else if (   argument == "-noncurses"|| argument == "--noncurses"    )   commands_ncurses = false;
        else if (   argument == "-nocursor" || argument == "--nocursor"     )   sandbox.cursor = false;
        else if (   argument == "-verbose"  || argument == "--verbose"      )   sandbox.verbose = true;
        else if (   argument == "-fxaa"     || argument == "--fxaa"         )   sandbox.fxaa = true;
        else if (   argument == "-vFlip"    || argument == "--vFlip"        )   vFlip = false;
        else if (   argument == "-fullFps"  || argument == "--fullFps"      ) {
            bRunAtFullFps = true;
            vera::setFps(0);
        }
        // add define GCC style
        else if (   argument != "-D"        && argument.find("-D") == 0 ) {
            std::string define = std::string("define,") + argument.substr(2);
            commandsArgs.push_back(define);
        }
        // add include folder GCC style
        else if ( argument != "-I"          && argument.find("-I") == 0 ) {
            std::string include = argument.substr(2);
            sandbox.include_folders.push_back(include);
        }

        // Change internal states with using a second parameter
        // add define GCC style
        else if (   argument == "-D"        || argument == "-define"    || argument == "--define") {
            if (++i < argc) {
                std::string define = std::string("define,") + std::string(argv[i]);
                commandsArgs.push_back(define);
            }
            else 
                std::cout << "Argument '" << argument << "' should be followed by a define tag" << std::endl;
        }
        // add include folder GCC style
        else if (   argument == "-I"        || argument == "-include"   || argument == "--include") {
            if (++i < argc) {
                std::string include = std::string(argv[i]);
                sandbox.include_folders.push_back(include);
            }
            else 
                std::cout << "Argument '" << argument << "' should be followed by a include path" << std::endl;
        }
        else if (   argument == "-r"        || argument == "-fps"  || argument == "--fps" ) {
            if(++i < argc)
                vera::setFps( vera::toInt(std::string(argv[i])) );
            else
                std::cout << "Argument '" << argument << "' should be followed by a <pixels>. Skipping argument." << std::endl;
        }
        else if (   argument == "-quilt"    || argument == "--quilt" ) {
            if (++i < argc)
                sandbox.quilt_resolution = vera::toInt(argv[i]);
            else
                std::cout << "Argument '" << argument << "' should be followed by a <quilt index type>" << std::endl;
        }
        else if (   argument == "-quilt_tile"    || argument == "--quilt_tile" ) {
            if (++i < argc)
                sandbox.quilt_tile = vera::toInt(argv[i]);
            else
                std::cout << "Argument '" << argument << "' should be followed by a <tile index type>" << std::endl;
        }
        else if (   argument == "-lenticular"   || argument == "--lenticular" ) {
            std::string calibration_file = "default";
            if (i+1 < argc) {
                calibration_file = std::string(argv[i+1]);
                if (vera::urlExists(calibration_file) && vera::haveExt(calibration_file,"json")) 
                    i++;
                else 
                    calibration_file = "default";
            } 
            else
                std::cout << "Argument '" << argument << "' should be followed by a path to calibration JSON file" << std::endl;

            sandbox.quilt_tile = -1;
            sandbox.lenticular = calibration_file;
            if (sandbox.quilt_resolution == -1) 
                sandbox.quilt_resolution = 0;
        }
        
        else if (   argument == "-p"        || argument == "-port"  || argument == "--port" ) {
            if(++i < argc)
                oscPort = vera::toInt(std::string(argv[i]));
            else
        #if defined(SUPPORT_OSC)
                std::cout << "Argument '" << argument << "' should be followed by an <osc_port>. Skipping argument." << std::endl;
        #else
                std::cout << "This version of GlslViewer wasn't compiled with OSC support" << std::endl;
        #endif
        }

        // Excecute COMMANDS
        else if (   argument == "-e" ) {
            if (++i < argc)         
                commandsArgs.push_back(std::string(argv[i]));
            else
                std::cout << "Argument '" << argument << "' should be followed by a <command>. Skipping argument." << std::endl;
        }
        else if (   argument == "-E" ) {
            if(++i < argc) {
                commandsArgs.push_back(std::string(argv[i]));
                commandsExit = true;
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <command>. Skipping argument." << std::endl;
        }

        // LOAD ASSETS
        //

        //  load fragment shader 
        else if ( sandbox.frag_index == -1 && (vera::haveExt(argument,"frag") || vera::haveExt(argument,"fs") ) ) {
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cout << "File " << argv[i] << " not founded. Creating a default fragment shader with that name"<< std::endl;

                std::ofstream out(argv[i]);
                if (haveGeometry)
                    out << vera::getDefaultSrc(vera::FRAG_DEFAULT_SCENE);
                else if (haveTextures)
                    out << vera::getDefaultSrc(vera::FRAG_DEFAULT_TEXTURE);
                else 
                    out << vera::getDefaultSrc(vera::FRAG_DEFAULT);
                out.close();
            }

            WatchFile file;
            file.type = FRAG_SHADER;
            file.path = argument;
            file.lastChange = st.st_mtime;
            files.push_back(file);

            sandbox.frag_index = files.size()-1;

        }

        // load vertex shader
        else if ( sandbox.vert_index == -1 && ( vera::haveExt(argument,"vert") || vera::haveExt(argument,"vs") ) ) {
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cout << "File " << argv[i] << " not founded. Creating a default vertex shader with that name"<< std::endl;

                std::ofstream out(argv[i]);
                out << vera::getDefaultSrc(vera::VERT_DEFAULT_SCENE);
                out.close();
            }

            WatchFile file;
            file.type = VERT_SHADER;
            file.path = argument;
            file.lastChange = st.st_mtime;
            files.push_back(file);

            sandbox.vert_index = files.size()-1;
        }

        // load geometry
        else if ( sandbox.geom_index == -1 && ( vera::haveExt(argument,"ply") || vera::haveExt(argument,"PLY") ||
                                                vera::haveExt(argument,"obj") || vera::haveExt(argument,"OBJ") ||
                                                vera::haveExt(argument,"stl") || vera::haveExt(argument,"STL") ||
                                                vera::haveExt(argument,"glb") || vera::haveExt(argument,"GLB") ||
                                                vera::haveExt(argument,"gltf") || vera::haveExt(argument,"GLTF") ||
                                                vera::haveExt(argument,"splat") || vera::haveExt(argument,"SPLAT") ) ) {
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

        // load image 
        else if (   vera::haveExt(argument,"hdr") || vera::haveExt(argument,"HDR") ||
                    vera::haveExt(argument,"exr") || vera::haveExt(argument,"EXR") ||
                    vera::haveExt(argument,"png") || vera::haveExt(argument,"PNG") ||
                    vera::haveExt(argument,"tga") || vera::haveExt(argument,"TGA") ||
                    vera::haveExt(argument,"psd") || vera::haveExt(argument,"PSD") ||
                    vera::haveExt(argument,"gif") || vera::haveExt(argument,"GIF") ||
                    vera::haveExt(argument,"bmp") || vera::haveExt(argument,"BMP") ||
                    vera::haveExt(argument,"jpg") || vera::haveExt(argument,"JPG") ||
                    vera::haveExt(argument,"jpeg") || vera::haveExt(argument,"JPEG")) {

            if ( vera::haveWildcard(argument) ) {
                if ( sandbox.uniforms.addStreamingTexture("u_tex" + vera::toString(textureCounter), argument, vFlip, false) )
                    textureCounter++;
            }
            else if ( sandbox.uniforms.addTexture("u_tex" + vera::toString(textureCounter), argument, vFlip) )
                textureCounter++;
        } 
        // load cubemap image as enviroment lighting map but not display it
        else if ( argument == "-c" || argument == "-sh" ) {
            if(++i < argc) {
                argument = std::string(argv[i]);
                sandbox.uniforms.addCubemap("enviroment", argument);
                sandbox.uniforms.activeCubemap = sandbox.uniforms.cubemaps["enviroment"];
                commandsArgs.push_back("cubemap,on");
                commandsArgs.push_back("cubemap,off");
                sandbox.getSceneRender().showCubebox = false;
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <environmental_map>. Skipping argument." << std::endl;
        }
        // load cubemap image and display it
        else if ( argument == "-C" ) {
            if(++i < argc)
            {
                argument = std::string(argv[i]);
                sandbox.uniforms.addCubemap("enviroment", argument);
                sandbox.uniforms.activeCubemap = sandbox.uniforms.cubemaps["enviroment"];
                commandsArgs.push_back("cubemap,on");
                sandbox.getSceneRender().showCubebox = true;
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <environmental_map>. Skipping argument." << std::endl;
        }

        // load video device
        else if ( argument == "-video"  || argument == "--video" ) {
            if (++i < argc) {
                argument = std::string(argv[i]);
                if ( sandbox.uniforms.addStreamingTexture("u_tex" + vera::toString(textureCounter), argument, vFlip, true) )
                    textureCounter++;
            }
        }
        else if (   argument.rfind("/dev/", 0) == 0) {
            if ( sandbox.uniforms.addStreamingTexture("u_tex" + vera::toString(textureCounter), argument, vFlip, true) )
                textureCounter++;
        }

        // load video file
        else if (   vera::haveExt(argument,"mov") || vera::haveExt(argument,"MOV") ||
                    vera::haveExt(argument,"mp4") || vera::haveExt(argument,"MP4") ||
                    vera::haveExt(argument,"mkv") || vera::haveExt(argument,"MKV") ||
                    vera::haveExt(argument,"mpg") || vera::haveExt(argument,"MPG") ||
                    vera::haveExt(argument,"mpeg") || vera::haveExt(argument,"MPEG") ||
                    vera::haveExt(argument,"h264") ||
                    argument.rfind("http://", 0) == 0 ||
                    argument.rfind("https://", 0) == 0 ||
                    argument.rfind("rtsp://", 0) == 0 ||
                    argument.rfind("rtmp://", 0) == 0) {
            if ( sandbox.uniforms.addStreamingTexture("u_tex" + vera::toString(textureCounter), argument, vFlip, false) )
                textureCounter++;
        }
        
        else if ( argument == "-a"  || argument == "-audio"     || argument == "--audio") {
            std::string device_id = "-1"; //default device id
            // device_id is optional argument, not iterate yet
            if ((i + 1) < argc) {
                argument = std::string(argv[i + 1]);
                if (vera::isInt(argument)) {
                    device_id = argument;
                    i++;
                }
            }
            if ( sandbox.uniforms.addStreamingAudioTexture("u_tex" + vera::toString(textureCounter), device_id, vFlip, true) )
                textureCounter++;
        }

        // load CSV data for camera path 
        else if ( vera::haveExt(argument,"csv") || vera::haveExt(argument,"CSV") ) {
            sandbox.uniforms.addCameras(argument);
        }
        
        // load specific textures image/video but with a custom name
        else if ( argument.find("-") == 0 ) {
            std::string parameterPair = argument.substr( argument.find_last_of('-') + 1 );
            if(++i < argc) {
                argument = std::string(argv[i]);

                // If it's a video file, capture device, streaming url or Image sequence
                if (vera::haveExt(argument,"mov") || vera::haveExt(argument,"MOV") ||
                    vera::haveExt(argument,"mp4") || vera::haveExt(argument,"MP4") ||
                    vera::haveExt(argument,"mpeg") || vera::haveExt(argument,"MPEG") ||
                    argument.rfind("/dev/", 0) == 0 ||
                    argument.rfind("http://", 0) == 0 ||
                    argument.rfind("https://", 0) == 0 ||
                    argument.rfind("rtsp://", 0) == 0 ||
                    argument.rfind("rtmp://", 0) == 0 ||
                    vera::haveWildcard(argument) ) {
                    sandbox.uniforms.addStreamingTexture(parameterPair, argument, vFlip, false);
                }

                // if points to a folder
                else if ( vera::isFolder(argument) ) 
                    sandbox.uniforms.addStreamingTexture(parameterPair, argument, vFlip, false);

                // Load a sequence of uniform data
                else if ( vera::haveExt(argument,"csv") || vera::haveExt(argument,"CSV") )
                    sandbox.uniforms.addSequence(parameterPair, argument);
                
                // Else load it as a single texture
                else 
                    sandbox.uniforms.addTexture(parameterPair, argument, vFlip);
            }
            else
                std::cout << "Argument '" << argument << "' should be followed by a <texture>. Skipping argument." << std::endl;
        }
    }

    // Verbose GL context creation
    if (sandbox.verbose) {
        std::cout << "Specs:\n" << std::endl;
        std::cout << "  - Vendor: " <<  vera::getVendor() << std::endl;
        std::cout << "  - Renderer: " <<  vera::getRenderer() << std::endl;
        std::cout << "  - Version: " <<  vera::getGLVersion() << std::endl;
        std::cout << "  - GLSL version: " <<  vera::getGLSLVersion() << std::endl;
        std::cout << "  - Extensions: " <<  vera::getExtensions() << std::endl;

        std::cout << "  - Implementation limits: " << std::endl;
        int param;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &param);
        std::cout << "      + GL_MAX_TEXTURE_SIZE = " << param << std::endl;
    }

    #ifndef __EMSCRIPTEN__
    // If there is no shader nor geometry, exit
    if ( sandbox.frag_index == -1 && sandbox.vert_index == -1 && sandbox.geom_index == -1 ) {
        printUsage(argv[0]);
        // onExit();
        // exit(EXIT_FAILURE);
    }
    #endif

    // Add GLSLVIEWER <VERSION> define
    commandsArgs.push_back( "define,GLSLVIEWER," + vera::toString(GLSLVIEWER_VERSION_MAJOR) + vera::toString(GLSLVIEWER_VERSION_MINOR) + vera::toString(GLSLVIEWER_VERSION_PATCH) );

    // let sandbox load commands
    sandbox.commandsInit(commands);

    // Load files to sandbox
    sandbox.loadAssets(files);
    if (sandbox.uniforms.models.size() > 0 ) {
        float area = sandbox.getSceneRender().getArea();
        sandbox.uniforms.setSunPosition( glm::vec3(0.0,area*10.0,area*10.0) );
    }

    // EVENTs callbacks
    //
    vera::setMousePressCallback(        [&](float _x, float _y, int _button) {  sandbox.onMousePress(_x, _y, _button); } );
    vera::setMouseMoveCallback(         [&](float _x, float _y) {  sandbox.onMouseMove(_x, _y); } );
    vera::setMouseDragCallback(         [&](float _x, float _y, int _button) {  sandbox.onMouseDrag(_x, _y, _button); } );
    vera::setScrollCallback(            [&](float _yOffset) {                   sandbox.onScroll(_yOffset); } );
    vera::setWindowResizeCallback(    [&](int _newWidth, int _newHeight) { 
        sandbox.onWindowResize(_newWidth, _newHeight);
    } );

    vera::setMouseMoveCallback(         [&](float _x, float _y) { 
        if (bScreensaverMode) {
            if (sandbox.isReady()) {
                bKeepRunnig = false;
                bKeepRunnig.store(false);
            }
        }
    } );

    vera::setKeyPressCallback(          [&](int _key) { 
        if (bScreensaverMode) {
            bKeepRunnig = false;
            bKeepRunnig.store(false);
        }
        else {
            if (_key == 'q' || _key == 'Q') {
                bKeepRunnig = false;
                bKeepRunnig.store(false);
            }
            else if (_key == 'a' || _key == 'A') {
                commandsRun("axis,toggle");
                commandsRun("update");
            }
            else if (_key == 'b' || _key == 'B') {
                commandsRun("bboxes,toggle");
                commandsRun("update");
            }
            else if (_key == 'c' || _key == 'C') {
                sandbox.cursor = !sandbox.cursor;
                vera::setMouseVisibility(sandbox.cursor);
                commandsRun("update");
            }
            else if (_key == 'd' || _key == 'D') {
                sandbox.getSceneRender().dynamicShadows = !sandbox.getSceneRender().dynamicShadows;
                commandsRun("update");
            }
            else if (_key == 'f') {
                commandsRun("floor,toggle");
                commandsRun("update");
            }
            else if (_key == 'F') {
                vera::setFullscreen(!vera::isFullscreen());
                commandsRun("update");
            }
            else if (_key == 'g' || _key == 'G') {
                commandsRun("grid,toggle");
                commandsRun("update");
            }
            else if (_key == 'h' || _key == 'H') {
                sandbox.help = !sandbox.help;
                commandsRun("update");
            }
            else if (_key == 'i' || _key == 'I') {
                commandsRun("plot,toggle");
                commandsRun("update");
            }
            #if !defined(PLATFORM_WINDOWS) || !defined(DRIVER_GLFW)
            else if (_key == 'o' || _key == 'O') {
                if (sandbox.frag_index != -1) {
                    std::string cmd = "open " + files[sandbox.frag_index].path;
                    system( cmd.c_str() );
                }

                if (sandbox.vert_index != -1) {
                    std::string cmd = "open " + files[sandbox.vert_index].path;
                    system( cmd.c_str() );
                }
            }
            #endif
            else if (_key == 'p' || _key == 'P') {
                commandsRun("buffers,toggle");
                commandsRun("update");
            }
            else if (_key == 'r' || _key == 'R') {
                commandsRun("streams,restart");
            }
            else if (_key == 's' || _key == 'S') {
                commandsRun("sky,toggle");
                commandsRun("update");
            }
            else if (_key == 't' || _key == 'T') {
                commandsRun("textures,toggle");
                commandsRun("update");
            }
            else if (_key == 'v' || _key == 'V') {
                sandbox.verbose = !sandbox.verbose;
                commandsRun("update");
            }
            else if (_key == 32) {
                commandsRun(std::string("streams,") + std::string( bStreamsPlaying ? "stop" : "play" ));
                bStreamsPlaying = !bStreamsPlaying;
            }
            else if (_key == 48) {
                commandsRun("camera,default");
                commandsRun("update");
            }
            else if (_key >= 49 && _key <= 57) {
                commandsRun(std::string("camera,") + vera::toString(_key - 49));
                commandsRun("update");
            }
            // Basic camera controls 
            else if (_key == VERA_KEY_LEFT) {
                commandsRun("camera_move,-0.1, 0, 0");
            } else if (_key == VERA_KEY_RIGHT) {
                commandsRun("camera_move,0.1, 0, 0");
            } else if (_key == VERA_KEY_UP) {
                commandsRun("camera_move,0, 0, 0.1");   
            } else if (_key == VERA_KEY_DOWN) {
                commandsRun("camera_move,0, 0, -0.1");
            }
        }
    } );
    vera::setDropCallback(    [&](int _count, const char** _paths) {    
        for (int i = 0;  i < _count;  i++) {
            std::string path = std::string( _paths[i] ); 

            if ( vera::haveExt(path,"frag") || vera::haveExt(path,"fs")  ) {
                if (sandbox.frag_index == -1) {
                    WatchFile file;
                    file.type = FRAG_SHADER;
                    file.path = path;
                    file.lastChange = 0;
                    files.push_back(file);
                    sandbox.frag_index = files.size()-1;
                }
                else {
                    files[sandbox.frag_index].path = path;
                    files[sandbox.frag_index].lastChange = 0;
                }
            }
            // load vertex shader
            else if (   vera::haveExt(path,"vert") || vera::haveExt(path,"vs") ) {
                if (sandbox.vert_index == -1) {
                    WatchFile file;
                    file.type = VERT_SHADER;
                    file.path = path;
                    file.lastChange = 0;
                    files.push_back(file);
                    sandbox.vert_index = files.size()-1;
                }
                else {
                    files[sandbox.vert_index].path = path;
                    files[sandbox.vert_index].lastChange = 0;
                }
            }
            // load geometry
            else if (   vera::haveExt(path,"ply") || vera::haveExt(path,"PLY") ||
                        vera::haveExt(path,"obj") || vera::haveExt(path,"OBJ") ||
                        vera::haveExt(path,"stl") || vera::haveExt(path,"STL") ||
                        vera::haveExt(path,"glb") || vera::haveExt(path,"GLB") ||
                        vera::haveExt(path,"gltf") || vera::haveExt(path,"GLTF") ) {

                bool init_lights = sandbox.uniforms.models.size() == 0;

                if (sandbox.geom_index == -1) {
                    WatchFile file;
                    file.type = GEOMETRY;
                    file.path = path;
                    file.lastChange = 0;
                    files.push_back(file); 
                    sandbox.geom_index = files.size()-1;
                }
                else {
                    commandsRun("models,clear");
                    files[sandbox.geom_index].path = path;
                    files[sandbox.geom_index].lastChange = 0;
                }
                sandbox.loadAssets(files);

                // if (init_lights) {
                //     float area = sandbox.getSceneRender().getArea();
                //     sandbox.uniforms.setSunPosition( glm::vec3(0.0,area*10.0,area*10.0) );
                // }

                commandsRun("update");
            }
            // load cubemap
            else if (   vera::haveExt(path,"hdr") || vera::haveExt(path,"HDR") ) {
                sandbox.uniforms.addCubemap("enviroment", path);
                sandbox.uniforms.activeCubemap = sandbox.uniforms.cubemaps["enviroment"];
                sandbox.getSceneRender().showCubebox = true;
                commandsRun("cubemap,on");
                commandsRun("update");
            }
            // load image 
            else if (   vera::haveExt(path,"png") || vera::haveExt(path,"PNG") ||
                        vera::haveExt(path,"tga") || vera::haveExt(path,"TGA") ||
                        vera::haveExt(path,"psd") || vera::haveExt(path,"PSD") ||
                        vera::haveExt(path,"gif") || vera::haveExt(path,"GIF") ||
                        vera::haveExt(path,"bmp") || vera::haveExt(path,"BMP") ||
                        vera::haveExt(path,"jpg") || vera::haveExt(path,"JPG") ||
                        vera::haveExt(path,"jpeg") || vera::haveExt(path,"JPEG")) {

                if ( sandbox.uniforms.addTexture("u_tex" + vera::toString(textureCounter), path, vFlip) )
                    textureCounter++;

                commandsRun("update");
            }
            // load video file
            else if (   vera::haveExt(path,"mov") || vera::haveExt(path,"MOV") ||
                        vera::haveExt(path,"mp4") || vera::haveExt(path,"MP4") ||
                        vera::haveExt(path,"mkv") || vera::haveExt(path,"MKV") ||
                        vera::haveExt(path,"mpg") || vera::haveExt(path,"MPG") ||
                        vera::haveExt(path,"mpeg") || vera::haveExt(path,"MPEG") ||
                        vera::haveExt(path,"h264") ) {
                if ( sandbox.uniforms.addStreamingTexture("u_tex" + vera::toString(textureCounter), path, vFlip, false) )
                    textureCounter++;

                commandsRun("update");
            }
        }
    } );

#if defined(__EMSCRIPTEN__)
    // On the browser (WASM/EMSCRIPTEN)

    //  - Request animation loop on browser
    emscripten_request_animation_frame_loop(loop, 0);

    //  - Make sure it's at the right canvas size
    double width,  height;
    emscripten_get_element_css_size("#canvas", &width, &height);
    vera::setWindowSize(width, height);

    //  - init WebXR
    webxr_init(
        /* Frame callback */
        [](void* _userData, int, WebXRRigidTransform* _headPose, WebXRView* _views, int _viewCount) {
            GlslViewer* sbox = (GlslViewer*)_userData;

            vera::updateGL();

            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

            if (sbox->isReady() && commandsArgs.size() > 0) {
                for (size_t i = 0; i < commandsArgs.size(); i++)
                    commandsRun(commandsArgs[i]);
                
                commandsArgs.clear();
            }

            vera::Camera* cam = sbox->uniforms.activeCamera;
            if (cam == nullptr)
                return;

            webxr_set_projection_params(cam->getNearClip(), cam->getFarClip());
            sbox->renderPrep();

            glm::vec3 cam_pos = cam->getPosition();
            glm::vec3 head_pos = glm::make_vec3(_headPose->position);

            for(int viewIndex = 0; viewIndex < _viewCount; viewIndex++) {

                WebXRView view = _views[ viewIndex];
                glViewport( view.viewport[0], view.viewport[1], view.viewport[2], view.viewport[3] );
                cam->setViewport(view.viewport[2], view.viewport[3]);
                glm::mat4 t = glm::translate(glm::mat4(1.), glm::make_vec3(view.viewPose.position) + head_pos );
                glm::mat4 r = glm::toMat4( glm::quat(view.viewPose.orientation[3], view.viewPose.orientation[0], view.viewPose.orientation[1], view.viewPose.orientation[2]) );
                // cam->setTransformMatrix( glm::translate( glm::inverse(t * r), cam_pos) );
                cam->setTransformMatrix( glm::inverse(t * r) );
                cam->setProjection( glm::make_mat4(view.projectionMatrix) );

                sbox->render();
                // sbox->renderUI();
            } 

            sbox->renderPost();
            sbox->renderDone();
            vera::renderGL();

            cam->setPosition(cam_pos);
        },

        /* Session start callback */
        [](void* _userData, int _mode) {
            std::cout << "Session START callback" << std::endl;
            vera::setXR((vera::XrMode)_mode);

            GlslViewer* sbox = (GlslViewer*)_userData;
            sbox->addDefine("PLATFORM_WEBXR", vera::toString(_mode));

            // // TODO: select START/END callbacks
            // webxr_set_select_start_callback([](WebXRInputSource *_inputSource, void *_userData) { 
            //     printf("select_start_callback\n"); 
            // }, _userData);

            // webxr_set_select_end_callback([](WebXRInputSource *_inputSource, void *_userData) { 
            //     printf("select_end_callback\n");

            // }, _userData);
        },

        /* Session end callback */
        [](void* _userData, int _mode) {
            std::cout << "Session END callback" << std::endl;
            vera::setXR(vera::NONE_XR_MODE);
            emscripten_request_animation_frame_loop(loop, _userData);    
        },

        /* Error callback */
        [](void* _userData, int _error) {
            switch (_error){
                case WEBXR_ERR_API_UNSUPPORTED:
                    std::cout << "WebXR unsupported in this browser." << std::endl;
                    break;
                case WEBXR_ERR_GL_INCAPABLE:
                    std::cout << "GL context cannot be used to render to WebXR" << std::endl;
                    break;
                case WEBXR_ERR_SESSION_UNSUPPORTED:
                    std::cout << "VR not supported on this device" << std::endl;
                    break;
                default:
                    std::cout << "Unknown WebXR error with code" << std::endl;
                }
        },
        /* userData */
        &sandbox);

        vera::requestXR(vera::VR_MODE);

#else

    #if defined(PLATFORM_RPI)
    vera::setWindowVSync(false);
    #else
    vera::setWindowVSync(true);
    #endif

    // Start watchers
    std::thread fileWatcher( &fileWatcherThread );

    // OSC
    #if defined(SUPPORT_OSC)
    lo::ServerThread oscServer(oscPort);
    oscServer.set_callbacks( [&st](){
        std::cout << "// Listening for OSC commands on port:" << oscPort << std::endl;
    }, [](){});
    oscServer.add_method(0, 0, [](const char *path, lo::Message m) {
        std::string line;
        std::vector<std::string> address = vera::split(std::string(path), '/');
        for (size_t i = 0; i < address.size(); i++)
            line +=  ((i != 0) ? "," : "") + address[i];

        std::string types = m.types();
        lo_arg** argv = m.argv(); 
        lo_message msg = m;
        for (size_t i = 0; i < types.size(); i++) {
            if ( types[i] == 's')
                line += "," + std::string( (const char*)argv[i] );
            else if (types[i] == 'i')
                line += "," + vera::toString(argv[i]->i);
            else
                line += "," + vera::toString(argv[i]->f);
        }

        if (sandbox.verbose)
            std::cout << line << std::endl;
            
        commandsRun(line, oscMutex);
    });

    if (oscPort > 0) {
        oscServer.start();
    }
    #endif

    if (sandbox.verbose) {
        std::cout << "\nRunning on:\n" << std::endl;
        std::cout << "  - Vendor:       " << vera::getVendor() << std::endl;
        std::cout << "  - Version:      " << vera::getGLVersion() << std::endl;
        std::cout << "  - Renderer:     " << vera::getRenderer() << std::endl;
        std::cout << "  - GLSL version: " << vera::getGLSLVersion() << std::endl;
        std::cout << "  - Extensions:   " << vera::getExtensions() << std::endl;

        std::cout << "\nImplementation limits:\n" << std::endl;
        int param;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &param);
        std::cout << "  - GL_MAX_TEXTURE_SIZE = " << param << std::endl;
    }

    // Render Loop
    while (vera::isGL() && bKeepRunnig.load())
        loop();

    // If is terminated by the windows manager, turn bKeepRunnig off so the fileWatcher can stop
    if ( !vera::isGL() )
        bKeepRunnig.store(false);

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
void commandsRun(const std::string &_cmd) { commandsRun(_cmd, commandsMutex); }
void commandsRun(const std::string &_cmd, std::mutex &_mutex) {
    bool resolve = false;

    // Check if _cmd is present in the list of commands
    for (size_t i = 0; i < commands.size(); i++) {
        if (vera::beginsWith(_cmd, commands[i].trigger)) {
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
    
    // Scene commands
    // 
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
            std::vector<std::string> values = vera::split(_line,',');
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
    "about", "about glslViewer", true));

    commands.push_back(Command("window_width", [&](const std::string& _line){ 
        if (_line == "window_width") {
            std::cout << vera::getWindowWidth() << std::endl;
            return true;
        }
        return false;
    },
    "window_width", "return the width of the windows", false));

    commands.push_back(Command("window_height", [&](const std::string& _line){ 
        if (_line == "window_height") {
            std::cout << vera::getWindowHeight() << std::endl;
            return true;
        }
        return false;
    },
    "window_height", "return the height of the windows", false));

    commands.push_back(Command("screen_size", [&](const std::string& _line){ 
        if (_line == "screen_size") {
            std::cout << vera::getDisplayWidth() << ',' << vera::getDisplayHeight()<< std::endl;
            return true;
        }
        return false;
    },
    "screen_size", "return the screen size", false));

    commands.push_back(Command("viewport", [&](const std::string& _line){ 
        if (_line == "viewport") {
            glm::ivec4 viewport = vera::getViewport();
            std::cout << viewport.x << ',' << viewport.y << ',' << viewport.z << ',' << viewport.w << std::endl;
            return true;
        }
        return false;
    },
    "viewport", "return the viewport size", false));

    commands.push_back(Command("mouse", [&](const std::string& _line) { 
        std::vector<std::string> values = vera::split(_line,',');
        if (_line == "mouse") {
            std::cout << vera::getMouseX() << "," << vera::getMouseY() << std::endl;
            return true;
        }
        else if (values[1] == "capture") {
            captureMouse(true);
            return true;
        }
        return false;
    },
    "mouse", "return the mouse position", false));

    commands.push_back(Command("delta", [&](const std::string& _line){ 
        if (_line == "delta") {
            // Force the output in floats
            std::cout << std::setprecision(6) << vera::getDelta() << std::endl;
            return true;
        }
        return false;
    },
    "delta", "return u_delta, the secs between frames", false));

    commands.push_back(Command("date", [&](const std::string& _line){ 
        if (_line == "date") {
            // Force the output in floats
            glm::vec4 date = vera::getDate();
            std::cout << date.x << ',' << date.y << ',' << date.z << ',' << date.w << std::endl;
            return true;
        }
        return false;
    },
    "date", "return u_date as YYYY, M, D and Secs", false));

    commands.push_back(Command("files", [&](const std::string& _line){ 
        if (_line == "files") {
            for (size_t i = 0; i < files.size(); i++) { 
                std::cout << std::setw(2) << i << "," << std::setw(12) << vera::toString(files[i].type) << "," << files[i].path << std::endl;
            }
            return true;
        }
        return false;
    },
    "files", "return a list of files", false));
    
    commands.push_back(Command("screenshot", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
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
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() >= 3) {
            float from = vera::toFloat(values[1]);
            float to = vera::toFloat(values[2]);
            float fps = 24.0;

            if (values.size() == 4)
                fps = vera::toFloat(values[3]);

            if (from >= to) {
                from = 0.0;
            }

            commandsMutex.lock();
            recordingStartSecs(from, to, fps);
            commandsMutex.unlock();

            float pct = 0.0f;
            while (pct < 1.0f) {
                // get progres.
                commandsMutex.lock();
                pct = getRecordingPercentage();
                commandsMutex.unlock();

                console_draw_pct(pct);

                std::this_thread::sleep_for(std::chrono::milliseconds( vera::getRestMs() ));
            }
            return true;
        }
        return false;
    },
    "sequence,<from_sec>,<to_sec>[,<fps>]","save a PNG sequence <from_sec> <to_sec> at <fps> (default: 24)",false));

    commands.push_back(Command("secs", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() >= 3) {
            float from = vera::toFloat(values[1]);
            float to = vera::toFloat(values[2]);
            float fps = 24.0;

            if (values.size() == 4)
                fps = vera::toFloat(values[3]);

            if (from >= to) {
                from = 0.0;
            }

            commandsMutex.lock();
            recordingStartSecs(from, to, fps);
            commandsMutex.unlock();

            float pct = 0.0f;
            while (pct < 1.0f) {
                // get progres.
                commandsMutex.lock();
                pct = getRecordingPercentage();
                commandsMutex.unlock();

                console_draw_pct(pct);

                std::this_thread::sleep_for(std::chrono::milliseconds( vera::getRestMs() ));
            }
            return true;
        }
        return false;
    },
    "secs,<A>,<B>[,<fps>]","saves a sequence of images from second A to second B at <fps> (default: 24)", false));

    commands.push_back(Command("frames", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() >= 3) {
            int from = vera::toInt(values[1]);
            int to = vera::toInt(values[2]);
            float fps = 24.0;

            if (values.size() == 4)
                fps = vera::toFloat(values[3]);

            if (from >= to)
                from = 0.0;

            commandsMutex.lock();
            recordingStartFrames(from, to, fps);
            commandsMutex.unlock();

            float pct = 0.0f;
            while (pct < 1.0f) {

                // Check progres.
                commandsMutex.lock();
                pct = getRecordingPercentage();
                commandsMutex.unlock();
                
                console_draw_pct(pct);

                std::this_thread::sleep_for(std::chrono::milliseconds( vera::getRestMs() ));
            }
            return true;
        }
        return false;
    },
    "frames,<A>,<B>[,<fps>]","saves a sequence of images from frame <A> to <B> at <fps> (default: 24)", false));

    #if defined(SUPPORT_LIBAV) && !defined(PLATFORM_RPI)
    commands.push_back(Command("record", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() >= 3) {
            RecordingSettings settings;
            settings.src_width = vera::getWindowWidth();
            settings.src_height = vera::getWindowHeight();
            settings.src_fps = vera::getFps();
            float pd = vera::getDisplayPixelRatio();

            settings.trg_path = values[1];
            float from = 0;
            float to = 0.0;
            
            if (values.size() > 2)
                from = vera::toFloat(values[2]);

            if (values.size() > 3)
                to = vera::toFloat(values[3]);
            else
                to = vera::toFloat(values[2]);

            if (from >= to)
                from = 0.0;

            if (from == 0.0)
                sandbox.uniforms.setStreamsRestart();

            settings.trg_fps = 24.0f;
            if (values.size() > 4)
                settings.trg_fps = vera::toFloat(values[4]);

            settings.trg_width = (int)(settings.src_width);
            settings.trg_height = (int)(settings.src_height);

            bool valid = false;
            if (vera::haveExt(values[1], "mp4") || vera::haveExt(values[1], "MP4") ) {
                settings.trg_width = vera::roundTo( settings.trg_width, 2);
                settings.trg_height = vera::roundTo( settings.trg_height , 2);

                valid = true;
                settings.trg_args = "-r " + vera::toString( settings.trg_fps );
                settings.trg_args += " -c:v libx264";
                // settings.trg_args += " -b:v 20000k";
                settings.trg_args += " -vf \"vflip,fps=" + vera::toString(settings.trg_fps);
                if (pd > 1)
                    settings.trg_args += ",scale=" + vera::toString(settings.trg_width,0) + ":" + vera::toString(settings.trg_height,0) + ":flags=lanczos";
                settings.trg_args += "\"";
                settings.trg_args += " -crf 10";
                settings.trg_args += " -pix_fmt yuv420p";
                settings.trg_args += " -vsync 1";
                settings.trg_args += " -g 1";
            }
            else if (vera::haveExt(values[1], "gif") || vera::haveExt(values[1], "GIF") ) {;
                settings.trg_width = vera::roundTo( (int)((settings.trg_width/pd)/2), 2);
                settings.trg_height = vera::roundTo( (int)((settings.trg_height/pd)/2), 2);

                valid = true;
                settings.trg_args = " -vf \"vflip,fps=" + vera::toString(settings.trg_fps);
                if (pd > 1)
                    settings.trg_args += ",scale=" + vera::toString((float)settings.trg_width,0) + ":" + vera::toString((float)settings.trg_height,0) + ":flags=lanczos";
                settings.trg_args += ",split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse\"";
                settings.trg_args += " -loop 0";
            }

            if (valid) {
                commandsMutex.lock();
                recordingPipeOpen(settings, from, to);
                commandsMutex.unlock();

                float pct = 0.0f;
                while (pct < 1.0f) {
                    commandsMutex.lock();
                    pct = getRecordingPercentage();
                    commandsMutex.unlock();

                    console_draw_pct(pct);
                    std::this_thread::sleep_for(std::chrono::milliseconds( vera::getRestMs() ));
                }
            }

            return true;
        }
        return false;
    },
    "record,<file>,<A>,<B>[,<fps>]","record a video from second <A> to second <B> at <fps> (default: 24.0f)", false));
    #endif

    // General environment commands
    //
    commands.push_back(Command("fullFps", [&](const std::string& _line){
        if (_line == "fullFps") {
            std::string rta = bRunAtFullFps ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                commandsMutex.lock();
                bRunAtFullFps = (values[1] == "on");
                vera::setFps(0);
                commandsMutex.unlock();
            }
        }
        return false;
    },
    "fullFps[,on|off]", "go to full FPS or not", false));

    commands.push_back(Command("fps", [&](const std::string& _line){
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            commandsMutex.lock();
            vera::setFps( vera::toInt(values[1]) );
            commandsMutex.unlock();
            return true;
        }
        else {
            // Force the output in floats
            std::cout << std::setprecision(6) << vera::getFps() << std::endl;
            return true;
        }
        return false;
    },
    "fps", "return or set the amount of frames per second", false));

    commands.push_back(Command("vsync", [&](const std::string& _line){
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            commandsMutex.lock();
            vera::setWindowVSync(values[1] == "on");
            commandsMutex.unlock();
        }
        return false;
    },
    "vsync[,on|off]", "set VSync on/off. It's on by default.", false));

    commands.push_back(Command("cursor", [&](const std::string& _line){
        if (_line == "cursor") {
            std::string rta = sandbox.cursor ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                commandsMutex.lock();
                sandbox.cursor = (values[1] == "on");
                commandsMutex.unlock();
            }
        }
        return false;
    },
    "cursor[,on|off]", "show/hide cursor", false));

    commands.push_back(Command("pixel_density", [&](const std::string& _line){ 
        if (_line == "pixel_density") {
            std::cout << vera::getDisplayPixelRatio() << std::endl;
            return true;
        }
        // else {
        //     std::vector<std::string> values = vera::split(_line,',');
        //     if (values.size() == 2) {
        //         commandsMutex.lock();
        //         vera::setPixelDensity( vera::toFloat(values[1]) );
        //         commandsMutex.unlock();
        //         return true;
        //     }
        // }
        return false;
    },
    // "pixel_density[,<pixel_density>]", "return or set pixel density", false));
    "pixel_density", "return pixel density", false));

    // Shader commands
    //

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

    commands.push_back(Command("frag", [&](const std::string& _line){ 
        if (_line == "frag") {
            std::cout << sandbox.getSource(FRAGMENT) << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                if (vera::isDigit(values[1])) {
                    // Line number
                    size_t lineNumber = vera::toInt(values[1]) - 1;
                    std::vector<std::string> lines = vera::split(sandbox.getSource(FRAGMENT),'\n', true);
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
                std::vector<std::string> lines = vera::split(sandbox.getSource(FRAGMENT),'\n', true);
                for (size_t i = 1; i < values.size(); i++) {
                    size_t lineNumber = vera::toInt(values[i]) - 1;
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
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                if (vera::isDigit(values[1])) {
                    // Line number
                    size_t lineNumber = vera::toInt(values[1]) - 1;
                    std::vector<std::string> lines = vera::split(sandbox.getSource(VERTEX),'\n', true);
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
                std::vector<std::string> lines = vera::split(sandbox.getSource(VERTEX),'\n', true);
                for (size_t i = 1; i < values.size(); i++) {
                    size_t lineNumber = vera::toInt(values[i]) - 1;
                    if (lineNumber < lines.size()) {
                        std::cout << lineNumber + 1 << " " << lines[lineNumber] << std::endl; 
                    }
                }
            }
        }
        return false;
    },
    "vert[,<filename>]", "returns or save the vertex shader source code", false));

    commands.push_back( Command("define", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            std::vector<std::string> v = vera::split(values[1],' ');
            if (v.size() > 1)
                sandbox.addDefine( v[0], v[1] );
            else
                sandbox.addDefine( v[0] );
            return true;
        }
        else if (values.size() == 3) {
            sandbox.addDefine( values[1], values[2] );
            return true;
        }
        return false;
    },
    "define,<KEYWORD>[,<VALUE>]", "add a define to the shader", false));

    commands.push_back( Command("undefine", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            sandbox.delDefine( values[1] );
            return true;
        }
        return false;
    },
    "undefine,<KEYWORD>", "remove a define on the shader", false));


    // Add 3D objects
    //
    commands.push_back(Command("plane", [&](const std::string & line) {
        std::vector<std::string> values = vera::split(line,',');
        int resolution = 512;

        if (values.size() > 1)
            resolution  = vera::toInt(values[1]);

        float size_f = resolution;
        vera::Mesh plane = vera::planeMesh(1.0f, 1.0f, resolution, resolution);
            
        // Add Model with pcl mesh
        sandbox.loadModel( new vera::Model("plane", plane) );

        #if defined(__EMSCRIPTEN__)
        // Commands are parse in the main GL loop in EMSCRIPTEN,
        // there is no risk to reload shaders outside main GL thread
        sandbox.resetShaders(files);
        #else
        // Reloading shaders can't be done directly on multi-thread (event thread)
        // to solve that, we trigger reloading by flagging changes on all files
        // witch signal the main GL thread to reload assets
        for (size_t i = 0; i < files.size(); i++)
            files[i].lastChange = 0;
        #endif

        return true;
    }, "plane[,<RESOLUTION>]", "add a plane"));

    commands.push_back(Command("pcl_plane", [&](const std::string & line) {
        std::vector<std::string> values = vera::split(line,',');
        int resolution = 512;

        if (values.size() > 1)
            resolution  = vera::toInt(values[1]);

        float size_f = resolution;
        vera::Mesh pcl;
        pcl.setDrawMode(vera::POINTS);
        for (int y = 0; y < resolution; y++) 
            for (int x = 0; x < resolution; x++)
                pcl.addVertex(glm::vec3(x/size_f, y/size_f, 0.0f));
            
        // Add Model with pcl mesh
        sandbox.loadModel( new vera::Model("point_cloud_plane", pcl) );

        #if defined(__EMSCRIPTEN__)
        // Commands are parse in the main GL loop in EMSCRIPTEN,
        // there is no risk to reload shaders outside main GL thread
        sandbox.resetShaders(files);
        #else
        // Reloading shaders can't be done directly on multi-thread (event thread)
        // to solve that, we trigger reloading by flagging changes on all files
        // witch signal the main GL thread to reload assets
        for (size_t i = 0; i < files.size(); i++)
            files[i].lastChange = 0;
        #endif

        return true;
    }, "pcl_plane[,<RESOLUTION>]", "add a pointcloud plane"));

    commands.push_back(Command("sphere", [&](const std::string & line) {
        std::vector<std::string> values = vera::split(line,',');
        int resolution = 24;

        if (values.size() > 1)
            resolution  = vera::toInt(values[1]);
            
        sandbox.loadModel( new vera::Model("sphere", vera::sphereMesh(resolution) ) );

        #if defined(__EMSCRIPTEN__)
        // Commands are parse in the main GL loop in EMSCRIPTEN,
        // there is no risk to reload shaders outside main GL thread
        sandbox.resetShaders(files);
        #else
        // Reloading shaders can't be done directly on multi-thread (event thread)
        // to solve that, we trigger reloading by flagging changes on all files
        // witch signal the main GL thread to reload assets
        for (size_t i = 0; i < files.size(); i++)
            files[i].lastChange = 0;
        #endif

        return true;
    }, "sphere[,<RESOLUTION>]", "add sphere mesh"));

    commands.push_back(Command("pcl_sphere", [&](const std::string & line) {
        std::vector<std::string> values = vera::split(line,',');
        int resolution = 24;

        if (values.size() > 1)
            resolution  = vera::toInt(values[1]);
        vera::Mesh mesh = vera::sphereMesh(resolution);
        mesh.setDrawMode(vera::POINTS);

        sandbox.loadModel( new vera::Model("pcl_sphere", mesh) );

        #if defined(__EMSCRIPTEN__)
        // Commands are parse in the main GL loop in EMSCRIPTEN,
        // there is no risk to reload shaders outside main GL thread
        sandbox.resetShaders(files);
        #else
        // Reloading shaders can't be done directly on multi-thread (event thread)
        // to solve that, we trigger reloading by flagging changes on all files
        // witch signal the main GL thread to reload assets
        for (size_t i = 0; i < files.size(); i++)
            files[i].lastChange = 0;
        #endif

        return true;
    }, "pcl_sphere[,<RESOLUTION>]", "add sphere mesh"));

    commands.push_back(Command("icosphere", [&](const std::string & line) {
        std::vector<std::string> values = vera::split(line,',');
        int resolution = 3;

        if (values.size() > 1)
            resolution  = vera::toInt(values[1]);
            
        sandbox.loadModel( new vera::Model("icosphere", vera::icosphereMesh(1.0, resolution) ) );

        #if defined(__EMSCRIPTEN__)
        // Commands are parse in the main GL loop in EMSCRIPTEN,
        // there is no risk to reload shaders outside main GL thread
        sandbox.resetShaders(files);
        #else
        // Reloading shaders can't be done directly on multi-thread (event thread)
        // to solve that, we trigger reloading by flagging changes on all files
        // witch signal the main GL thread to reload assets
        for (size_t i = 0; i < files.size(); i++)
            files[i].lastChange = 0;
        #endif

        return true;
    }, "icosphere[,<RESOLUTION>]", "add icosphere mesh"));


    commands.push_back(Command("cylinder", [&](const std::string & line) {
        std::vector<std::string> values = vera::split(line,',');
        int resolutionR = 16;
        int resolutionH = 3;
        int resolutionC = 1;

        if (values.size() > 1) {
            resolutionR  = vera::toInt(values[1]);
        }
        if (values.size() > 2) {
            resolutionH  = vera::toInt(values[2]);
        }
        if (values.size() > 3) {
            resolutionC  = vera::toInt(values[3]);
        }
            
        sandbox.loadModel( new vera::Model("c", vera::cylinderMesh(1.0, 1.0, resolutionR, resolutionH, resolutionC, resolutionC != 0) ) );

        #if defined(__EMSCRIPTEN__)
        // Commands are parse in the main GL loop in EMSCRIPTEN,
        // there is no risk to reload shaders outside main GL thread
        sandbox.resetShaders(files);
        #else
        // Reloading shaders can't be done directly on multi-thread (event thread)
        // to solve that, we trigger reloading by flagging changes on all files
        // witch signal the main GL thread to reload assets
        for (size_t i = 0; i < files.size(); i++)
            files[i].lastChange = 0;
        #endif

        return true;
    }, "cylinder[,<RESOLUTION_RADIUS>,<RESOLUTION_HEIGHT>]]", "add cylinder mesh"));

    commands.push_back(Command("wait", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            if (values[0] == "wait_sec")
                std::this_thread::sleep_for(std::chrono::seconds( vera::toInt(values[1])) );
            else if (values[0] == "wait_ms")
                std::this_thread::sleep_for(std::chrono::milliseconds( vera::toInt(values[1])) );
            else if (values[0] == "wait_us")
                std::this_thread::sleep_for(std::chrono::microseconds( vera::toInt(values[1])) );
            else
                std::this_thread::sleep_for(std::chrono::microseconds( (int)(vera::toFloat(values[1]) * 1000000) ));
            return true;
        }
        return false;
    },
    "wait,<seconds>", "wait for X <seconds> before excecuting another command", false));

    // ACTIONS commands
    //
    commands.push_back(Command("reload", [&](const std::string& _line){ 
        if (_line == "reload" || _line == "reload,all") {
            for (size_t i = 0; i < files.size(); i++) 
                sandbox.onFileChange( files, i );
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2 && values[0] == "reload") {
                for (size_t i = 0; i < files.size(); i++) {
                    if (files[i].path == values[1]) {
                        sandbox.onFileChange( files, i );
                        return true;
                    } 
                }
            }
        }
        return false;
    },
    "reload[,<filename>]", "reload one or all files", false));

    commands.push_back(Command("update", [&](const std::string& _line){ 
        if (_line == "update") {
            vera::flagChange();
            return true;
        }
        return false;
    },
    "update", "force all uniforms to be updated", false));

    commands.push_back(Command("exit", [&](const std::string& _line){ 
        if (_line == "exit") {
            bTerminate = true;
            // bKeepRunnig.store(false);
            return true;
        }
        return false;
    },
    "exit", "close glslViewer", false));

    commands.push_back(Command("quit", [&](const std::string& _line){ 
        if (_line == "quit") {
            bTerminate = true;
            // bKeepRunnig.store(false);
            return true;
        }
        return false;
    },
    "quit", "close glslViewer", false));

    commands.push_back(Command("q", [&](const std::string& _line){ 
        if (_line == "q") {
            bTerminate = true;
            // bKeepRunnig.store(false);
            return true;
        }
        return false;
    },
    "q", "close glslViewer", false));
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
    std::cerr << "      <texture>.(png/tga/jpg/bmp/psd/gif/exr/hdr/mov/mp4/rtsp/rtmp/etc)   # load and assign texture to uniform u_tex<N>" << std::endl;
    std::cerr << "      -<uniform_name> <texture>.(png/tga/jpg/bmp/psd/gif/exr/hdr)         # load a textures with a custom name" << std::endl;
    std::cerr << "      --video <video_device_number>   # open video device allocated wit that particular id" << std::endl;
    std::cerr << "      --audio [<capture_device_id>]   # open audio capture device as sampler2D texture " << std::endl;
    std::cerr << "      -C <enviromental_map>.(png/tga/jpg/bmp/psd/gif/hdr)     # load a env. map as cubemap" << std::endl;
    std::cerr << "      -c <enviromental_map>.(png/tga/jpg/bmp/psd/gif/hdr)     # load a env. map as cubemap but hided" << std::endl;
    std::cerr << "      -sh <enviromental_map>.(png/tga/jpg/bmp/psd/gif/hdr)    # load a env. map as spherical harmonics array" << std::endl;
    std::cerr << "      -vFlip                      # all textures after will be flipped vertically" << std::endl;
    std::cerr << "      -x <pixels>                 # set the X position of the window on the screen" << std::endl;
    std::cerr << "      -y <pixels>                 # set the Y position of the window on the screen" << std::endl;
    std::cerr << "      -s  or --size <pixels>      # set width and height of the window" << std::endl;
    std::cerr << "      -w  or --width <pixels>     # set the width of the window" << std::endl;
    std::cerr << "      -h  or --height <pixels>    # set the height of the window" << std::endl;
#if defined(DRIVER_GBM) 
    std::cerr << "      -d  or --display <display>  # open specific display port. Ex: -d /dev/dri/card1" << std::endl;
#endif
#if !defined(DRIVER_GLFW)
    std::cerr << "      -m  or --mouse <mouse>      # open specific mouse port. Ex: -d /dev/input/mice" << std::endl;
#endif
    std::cerr << "      -f  or --fullscreen         # load the window in fullscreen" << std::endl;
    std::cerr << "      -l  or --life-coding        # live code mode, where the billboard is allways visible" << std::endl;
    std::cerr << "      -ss or --screensaver        # screensaver mode, any pressed key will exit" << std::endl;
    std::cerr << "      --headless                  # headless rendering" << std::endl;
    std::cerr << "      --nocursor                  # hide cursor" << std::endl;
    std::cerr << "      --nofloor                   # hide cursor" << std::endl;
    std::cerr << "      --noncurses                 # disable ncurses command interface" << std::endl;
    std::cerr << "      --fps <fps>                 # fix the max FPS" << std::endl;
    std::cerr << "      --fxaa                      # set FXAA as postprocess filter" << std::endl;
    std::cerr << "      --quilt <0-15>              # quilt render (HoloPlay)" << std::endl;
    std::cerr << "      --quilt_tile <N>            # render a particular tile of a quilt (HoloPlay)" << std::endl;
    std::cerr << "      --lenticular <visual.json>  # lenticular calibration file, Looking Glass Model (HoloPlay)" << std::endl;
    std::cerr << "      -I<include_folder>          # add an include folder to default for #include files" << std::endl;
    std::cerr << "      -D<define>                  # add system #defines directly from the console argument" << std::endl;
    std::cerr << "      -p <OSC_port>               # open OSC listening port" << std::endl;
    std::cerr << "      -e  or -E <command>         # execute command when start. Multiple -e commands can be stack" << std::endl;
    std::cerr << "      -v  or --version            # return glslViewer version" << std::endl;
    std::cerr << "      --verbose                   # turn verbose outputs on" << std::endl;
    std::cerr << "      --help                      # print help for one or all command" << std::endl;
}

void onExit() {

    #if defined(SUPPORT_LIBAV) && !defined(PLATFORM_RPI)
    recordingPipeClose();
    #endif

    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // Delete the dynamic resources
    sandbox.uniforms.clear();

    // close openGL instance
    vera::closeGL();
}

//  Watching Thread
//============================================================================
void fileWatcherThread() {
    struct stat st;
    while ( bKeepRunnig.load() ) {
        for (size_t i = 0; i < files.size(); i++) {
            stat( files[i].path.c_str(), &st );
            int date = st.st_mtime;
            if ( date != files[i].lastChange ) {
                filesMutex.lock();
                files[i].lastChange = date;
                sandbox.onFileChange( files, i );
                filesMutex.unlock();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds( 500 ));
    }
}

//  Command line Thread
//============================================================================
void cinWatcherThread() {

    #if defined(SUPPORT_NCURSES)
    if (commands_ncurses) {
        console_init(oscPort);
        signal(SIGWINCH, console_sigwinch_handler);
        console_refresh();
    }
    #endif

    while (!sandbox.isReady()) {
        vera::sleep_ms( vera::getRestSec() * 1000000 );
        std::this_thread::sleep_for(std::chrono::milliseconds( vera::getRestMs() ));
    }

    // Argument commands to execute comming from -e or -E
    if (commandsArgs.size() > 0) {
        for (size_t i = 0; i < commandsArgs.size(); i++) {
            commandsRun(commandsArgs[i]);
            #if defined(SUPPORT_NCURSES)
            console_refresh();
            #endif
        }
        commandsArgs.clear();

        // If it's using -E exit after executing all commands
        if (commandsExit) {
            bTerminate = true;
            // bKeepRunnig.store(false);
        }
    }

    #if defined(SUPPORT_NCURSES)
    if (commands_ncurses) {
        while ( bKeepRunnig.load() ) {
            std::string cmd;
            if (console_getline(cmd, commands, sandbox))
                if (cmd.size() > 0)
                    commandsRun(cmd);
        }
        console_end();
    } else
    #endif
    {
        // Commands coming from the console IN
        std::string cmd;
        std::cout << "// > ";
        while (std::getline(std::cin, cmd)) {
            commandsRun(cmd);
            std::cout << "// > ";
        }
    }
}

#endif
