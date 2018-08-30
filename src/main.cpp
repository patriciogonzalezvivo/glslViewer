#include <sys/stat.h>
#include <unistd.h>

#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <fstream>

#include "gl/gl.h"
#include "window.h"
#include "sandbox.h"
#include "tools/fs.h"
#include "tools/text.h"
#include "tools/command.h"

// GLOBAL VARIABLES
//============================================================================
//
std::atomic<bool> bRun(true);

#ifndef REST_FPS
#define REST_FPS 33333
#endif

//  List of FILES to watch and the variable to communicate that between process
WatchFileList files;
std::mutex filesMutex;
int fileChanged;

// Console elements
CommandList commands;
std::mutex  consoleMutex;
std::string outputFile      = "";
std::string execute_cmd     = "";       // Execute commands
bool        execute_exit    = false;

std::string version = "1.5.5";
std::string name = "GlslViewer";
std::string header = name + " " + version + " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )"; 

// Here is where all the magic happens
Sandbox sandbox;

//================================================================= Threads
void fileWatcherThread();
void cinWatcherThread();

//================================================================= Functions
void onExit();
void printUsage(char * executableName) {
    std::cerr << "// " << header << std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// Swiss army knife of GLSL Shaders. Loads frag/vertex shaders, images and " << std::endl;
    std::cerr << "// geometries. Will reload automatically on changes. Support for multi  "<< std::endl;
    std::cerr << "// buffers, baground and postprocessing passes. Can render headlessly and "<< std::endl;
    std::cerr << "// into a file. Use POSIX STANDARD CONSOLE IN/OUT to comunicate (uniforms,"<< std::endl;
    std::cerr << "// camera position, scene description and  commands) to and with other "<< std::endl;
    std::cerr << "// programs. Compatible with Linux and MacOS, runs from command line with"<< std::endl;
    std::cerr << "// out X11 enviroment on RaspberryPi devices. "<< std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// Usage: " << executableName << " <shader>.frag [<shader>.vert] [<mesh>.(obj/.ply)] [<texture>.(png/jpg/hdr)] [-<uniformName> <texture>.(png/jpg/hdr)] [-c <cubemap>.(png/jpg/hdr)] [-vFlip] [-x <x>] [-y <y>] [-w <width>] [-h <height>] [-l] [--square] [-s/--sec <seconds>] [-o <screenshot_file>.png] [--headless] [--cursor] [-I<include_folder>] [-D<define>] [-e/-E <command>][-v/--version] [--verbose] [--help]\n";
}

void declareCommands() {
    commands.push_back(Command("help", [&](const std::string& _line){
        if (_line == "help") {
            std::cout << "// " << header << std::endl;
            std::cout << "// " << std::endl;
            for (unsigned int i = 0; i < commands.size(); i++) {
                std::cout << "// " << commands[i].description << std::endl;
            }
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                for (unsigned int i = 0; i < commands.size(); i++) {
                    if (commands[i].begins_with == values[1]) {
                        std::cout << "// " << commands[i].description << std::endl;
                    }
                }
            }
        }
        return false;
    },
    "help[,<command>]       print help for one or all command"));

    commands.push_back(Command("version", [&](const std::string& _line){ 
        if (_line == "version") {
            std::cout << version << std::endl;
            return true;
        }
        return false;
    },
    "version                return glslViewer version."));

    commands.push_back(Command("debug", [&](const std::string& _line){
        if (_line == "debug") {
            std::string rta = sandbox.debug ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                consoleMutex.lock();
                sandbox.debug = (values[1] == "on");
                consoleMutex.unlock();
            }
        }
        return false;
    },
    "debug[,on|off]       show/hide debug elements"));

    commands.push_back(Command("cursor", [&](const std::string& _line){
        if (_line == "cursor") {
            std::string rta = sandbox.cursor ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                consoleMutex.lock();
                sandbox.cursor = (values[1] == "on");
                consoleMutex.unlock();
            }
        }
        return false;
    },
    "cursor[,on|off]       show/hide cursor"));

    commands.push_back(Command("window_height", [&](const std::string& _line){ 
        if (_line == "window_height") {
            std::cout << getWindowHeight() << std::endl;
            return true;
        }
        return false;
    },
    "window_height          return the height of the windows."));

    commands.push_back(Command("pixel_density", [&](const std::string& _line){ 
        if (_line == "pixel_density") {
            std::cout << getPixelDensity() << std::endl;
            return true;
        }
        return false;
    },
    "pixel_density          return the pixel density."));

    commands.push_back(Command("screen_size", [&](const std::string& _line){ 
        if (_line == "screen_size") {
            glm::ivec2 screen_size = getScreenSize();
            std::cout << screen_size.x << ',' << screen_size.y << std::endl;
            return true;
        }
        return false;
    },
    "screen_size            return the screen size."));

    commands.push_back(Command("viewport", [&](const std::string& _line){ 
        if (_line == "viewport") {
            glm::ivec4 viewport = getViewport();
            std::cout << viewport.x << ',' << viewport.y << ',' << viewport.z << ',' << viewport.w << std::endl;
            return true;
        }
        return false;
    },
    "viewport               return the viewport size."));

    commands.push_back(Command("mouse", [&](const std::string& _line){ 
        if (_line == "mouse") {
            glm::vec2 pos = getMousePosition();
            std::cout << pos.x << "," << pos.y << std::endl;
            return true;
        }
        return false;
    },
    "mouse                  return the mouse position."));
    
    commands.push_back(Command("fps", [&](const std::string& _line){ 
        if (_line == "fps") {
            // Force the output in floats
            printf("%f\n", getFPS());
            return true;
        }
        return false;
    },
    "fps                    return u_fps, the number of frames per second."));

    commands.push_back(Command("delta", [&](const std::string& _line){ 
        if (_line == "delta") {
            // Force the output in floats
            printf("%f\n", getDelta());
            return true;
        }
        return false;
    },
    "delta                  return u_delta, the secs between frames."));

    commands.push_back(Command("time", [&](const std::string& _line){ 
        if (_line == "time") {
            // Force the output in floats
            printf("%f\n", getTime());
            return true;
        }
        return false;
    },
    "time                   return u_time, the elapsed time."));

    commands.push_back(Command("date", [&](const std::string& _line){ 
        if (_line == "date") {
            // Force the output in floats
            glm::vec4 date = getDate();
            std::cout << date.x << ',' << date.y << ',' << date.z << ',' << date.w << std::endl;
            return true;
        }
        return false;
    },
    "date                   return u_date as YYYY, M, D and Secs."));

    commands.push_back(Command("culling", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 1) {
            if (sandbox.getCulling() == NONE) {
                std::cout << "none" << std::endl;
            }
            else if (sandbox.getCulling() == FRONT) {
                std::cout << "front" << std::endl;
            }
            else if (sandbox.getCulling() == BACK) {
                std::cout << "back" << std::endl;
            }
            else if (sandbox.getCulling() == BOTH) {
                std::cout << "both" << std::endl;
            }
            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "none") {
                sandbox.setCulling(NONE);
            }
            else if (values[1] == "front") {
                sandbox.setCulling(FRONT);
            }
            else if (values[1] == "back") {
                sandbox.setCulling(BACK);
            }
            else if (values[1] == "both") {
                sandbox.setCulling(BOTH);
            }
            return true;
        }

        return false;
    },
    "culling[,<none|front|back|both>]   get or set the culling modes"));

    commands.push_back(Command("frag", [&](const std::string& _line){ 
        if (_line == "frag") {
            std::cout << sandbox.getSource(FRAGMENT) << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                if (isDigit(values[1])) {
                    // Line number
                    unsigned int lineNumber = toInt(values[1]) - 1;
                    std::vector<std::string> lines = split(sandbox.getSource(FRAGMENT),'\n', true);
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
                std::vector<std::string> lines = split(sandbox.getSource(FRAGMENT),'\n', true);
                for (unsigned int i = 1; i < values.size(); i++) {
                    unsigned int lineNumber = toInt(values[i]) - 1;
                    if (lineNumber < lines.size()) {
                        std::cout << lineNumber + 1 << " " << lines[lineNumber] << std::endl; 
                    }
                }
            }
        }
        return false;
    },
    "frag[,<filename>]      returns or save the fragment shader source code."));

    commands.push_back(Command("vert", [&](const std::string& _line){ 
        if (_line == "vert") {
            std::cout << sandbox.getSource(VERTEX) << std::endl;
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                if (isDigit(values[1])) {
                    // Line number
                    unsigned int lineNumber = toInt(values[1]) - 1;
                    std::vector<std::string> lines = split(sandbox.getSource(VERTEX),'\n', true);
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
                std::vector<std::string> lines = split(sandbox.getSource(VERTEX),'\n', true);
                for (unsigned int i = 1; i < values.size(); i++) {
                    unsigned int lineNumber = toInt(values[i]) - 1;
                    if (lineNumber < lines.size()) {
                        std::cout << lineNumber + 1 << " " << lines[lineNumber] << std::endl; 
                    }
                }
            }
        }
        return false;
    },
    "vert[,<filename>]      returns or save the vertex shader source code."));

    commands.push_back( Command("dependencies", [&](const std::string& _line){ 
        if (_line == "dependencies") {
            for (unsigned int i = 0; i < files.size(); i++) { 
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
    "dependencies[,vert|frag]   returns all the dependencies of the vertex o fragment shader or both."));

    commands.push_back(Command("files", [&](const std::string& _line){ 
        if (_line == "files") {
            for (unsigned int i = 0; i < files.size(); i++) { 
                std::cout << std::setw(2) << i << "," << std::setw(12) << toString(files[i].type) << "," << files[i].path << std::endl;
            }
            return true;
        }
        return false;
    },
    "files                  return a list of files."));

    commands.push_back(Command("buffers", [&](const std::string& _line){ 
        if (_line == "buffers") {
            for (int i = 0; i < sandbox.getTotalBuffers(); i++) {
                std::cout << "u_buffer" << i << std::endl;
            }
            return true;
        }
        return false;
    },
    "buffers                return a list of buffers as their uniform name."));

    commands.push_back(Command("defines", [&](const std::string& _line){ 
        if (_line == "defines") {
            for (unsigned int i = 0; i < sandbox.defines.size(); i++) {
                std::cout << sandbox.defines[i] << std::endl;
            }
            return true;
        }
        return false;
    },
    "defines                return a list of active defines"));

    commands.push_back( Command("define,", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            consoleMutex.lock();
            sandbox.addDefines( values[1] ); 
            consoleMutex.unlock();
            filesMutex.lock();
            fileChanged = sandbox.frag_index;
            filesMutex.unlock();
            return true;
        }
        return false;
    },
    "define,<KEYWORD>       add a define to the shader"));

    commands.push_back( Command("undefine,", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            consoleMutex.lock();
            sandbox.delDefines( values[1] ); 
            consoleMutex.unlock();
            filesMutex.lock();
            fileChanged = sandbox.frag_index;
            filesMutex.unlock();
            return true;
        }
        return false;
    },
    "undefine,<KEYWORD>     remove a define on the shader"));

    commands.push_back(Command("uniforms", [&](const std::string& _line){ 
        if (_line == "uniforms,all") {
            // Print all Native Uniforms (they carry functions)
            for (UniformFunctionsList::iterator it= sandbox.uniforms_functions.begin(); it != sandbox.uniforms_functions.end(); ++it) {                
                std::cout << it->second.type << ',' << it->first;
                if (it->second.print) {
                    std::cout << "," << it->second.print();
                }
                std::cout << std::endl;
            }
        }
        else {
            // Print Native Uniforms (they carry functions) that are present on the shader
            for (UniformFunctionsList::iterator it= sandbox.uniforms_functions.begin(); it != sandbox.uniforms_functions.end(); ++it) {                
                if (it->second.present) {
                    std::cout << it->second.type << ',' << it->first;
                    if (it->second.print) {
                        std::cout << "," << it->second.print();
                    }
                    std::cout << std::endl;
                }
            }
        }
        
        // Print user defined uniform data
        for (UniformDataList::iterator it= sandbox.uniforms_data.begin(); it != sandbox.uniforms_data.end(); ++it) {
            std::cout << it->second.getType() << "," << it->first;
            for (int i = 0; i < it->second.size; i++) {
                std::cout << ',' << it->second.value[i];
            }
            std::cout << std::endl;
        }

        for (int i = 0; i < sandbox.getTotalBuffers(); i++) {
            std::cout << "sampler2D," << "u_buffer" << i << std::endl;
        }

        for (TextureList::iterator it = sandbox.textures.begin(); it != sandbox.textures.end(); ++it) {
            std::cout << "sampler2D," << it->first << ',' << it->second->getFilePath() << std::endl;
        }

        // TODO
        //      - Cubemap

        return true;
    },
    "uniforms[,all|active]  return a list of all uniforms and their values or just the one active (default)."));

    commands.push_back(Command("textures", [&](const std::string& _line){ 
        if (_line == "textures") {
            for (TextureList::iterator it = sandbox.textures.begin(); it != sandbox.textures.end(); ++it) {
                std::cout << it->first << ',' << it->second->getFilePath() << std::endl;
            }
            return true;
        }
        return false;
    },
    "textures               return a list of textures as their uniform name and path."));

    commands.push_back(Command("window_width", [&](const std::string& _line){ 
        if (_line == "window_width") {
            std::cout << getWindowWidth() << std::endl;
            return true;
        }
        return false;
    },
    "window_width           return the width of the windows."));

    commands.push_back(Command("camera_distance", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            consoleMutex.lock();
            sandbox.getCamera().setDistance(toFloat(values[1]));
            sandbox.flagChange();
            consoleMutex.unlock();
            return true;
        }
        else {
            std::cout << sandbox.getCamera().getDistance() << std::endl;
            return true;
        }
        return false;
    },
    "camera_distance[,<dist>]       get or set the camera distance to the target."));

    commands.push_back(Command("camera_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            consoleMutex.lock();
            sandbox.getCamera().setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            sandbox.flagChange();
            consoleMutex.unlock();
            return true;
        }
        else {
            glm::vec3 pos = sandbox.getCamera().getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "camera_position[,<x>,<y>,<z>]  get or set the camera position."));

    commands.push_back(Command("camera_exposure", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            consoleMutex.lock();
            sandbox.getCamera().setExposure(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            sandbox.flagChange();
            consoleMutex.unlock();
            return true;
        }
        else {
            std::cout << sandbox.getCamera().getExposure() << std::endl;
            return true;
        }
        return false;
    },
    "camera_exposure[,<aperture>,<shutterSpeed>,<sensitivity>]  get or set the camera exposure. Defaults: 16, 1/125s, 100 ISO"));

    commands.push_back(Command("light_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            consoleMutex.lock();
            sandbox.getLight().setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            sandbox.flagChange();
            consoleMutex.unlock();
            return true;
        }
        else {
            glm::vec3 pos = sandbox.getLight().getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "light_position[,<x>,<y>,<z>]  get or set the light position."));

    commands.push_back(Command("light_color", [&](const std::string& _line){ 
         std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            consoleMutex.lock();
            sandbox.getLight().color = glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            sandbox.flagChange();
            consoleMutex.unlock();
            return true;
        }
        else {
            glm::vec3 color = sandbox.getLight().color;
            std::cout << color.x << ',' << color.y << ',' << color.z << std::endl;
            return true;
        }
        return false;
    },
    "light_color[,<r>,<g>,<b>]      get or set the light color."));

    commands.push_back(Command("screenshot", [&](const std::string& _line){ 
        if (_line == "screenshot" && outputFile != "") {
            consoleMutex.lock();
            sandbox.screenshotFile = outputFile;
            consoleMutex.unlock();
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                consoleMutex.lock();
                sandbox.screenshotFile = values[1];
                consoleMutex.unlock();
                return true;
            }
        }
        return false;
    },
    "screenshot[,<filename>]        saves a screenshot to a filename."));

     commands.push_back(Command("sequence", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 3) {
            float from = toFloat(values[1]);
            float to = toFloat(values[2]);

            if (from >= to) {
                from = 0.0;
            }

            consoleMutex.lock();
            sandbox.record(from, to);
            consoleMutex.unlock();

            std::cout << "// " << std::endl;

            int pct = 0;
            while (pct < 100) {
                // Delete previous line
                const std::string deleteLine = "\e[2K\r\e[1A";
                std::cout << deleteLine;

                // Check progres.
                consoleMutex.lock();
                pct = sandbox.getRecordedPorcentage();
                consoleMutex.unlock();
                
                std::cout << "// [ ";
                for (int i = 0; i < 50; i++) {
                    if (i < pct/2) {
                        std::cout << "#";
                    }
                    else {
                        std::cout << ".";
                    }
                }
                std::cout << " ] " << pct << "%" << std::endl;
                usleep(REST_FPS);
            }
            return true;
        }
        return false;
    },
    "sequence,<A_sec>,<B_sec>   saves a sequence of images from A to B second."));

    commands.push_back(Command("q", [&](const std::string& _line){ 
        if (_line == "q") {
            bRun.store(false);
            return true;
        }
        return false;
    },
    "q                          close glslViewer"));

    commands.push_back(Command("quit", [&](const std::string& _line){ 
        if (_line == "quit") {
            bRun.store(false);
            return true;
        }
        return false;
    },
    "quit                       close glslViewer"));

    commands.push_back(Command("exit", [&](const std::string& _line){ 
        if (_line == "exit") {
            bRun.store(false);
            return true;
        }
        return false;
    },
    "exit                       close glslViewer"));
}

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
    bool displayHelp = false;
    bool alwaysOnTop = false;
    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);

        if (        std::string(argv[i]) == "-x" ) {
            i++;
            windowPosAndSize.x = toInt(std::string(argv[i]));
        }
        else if (   std::string(argv[i]) == "-y" ) {
            i++;
            windowPosAndSize.y = toInt(std::string(argv[i]));
        }
        else if (   std::string(argv[i]) == "-w" ||
                    std::string(argv[i]) == "--width" ) {
            i++;
            windowPosAndSize.z = toInt(std::string(argv[i]));
        }
        else if (   std::string(argv[i]) == "-h" ||
                    std::string(argv[i]) == "--height" ) {
            i++;
            windowPosAndSize.w = toInt(std::string(argv[i]));
        }
        else if (   std::string(argv[i]) == "--headless" ) {
            headless = true;
        }
        else if (   std::string(argv[i]) == "--help" ) {
            displayHelp = true;
        }
        
        else if (   std::string(argv[i]) == "-l" ||
                    std::string(argv[i]) == "--life-coding" ){
        #ifdef PLATFORM_RPI
            windowPosAndSize.x = windowPosAndSize.z - 500;
            windowPosAndSize.z = windowPosAndSize.w = 500;
        #else
            alwaysOnTop = true;
        #endif
        }
        
    }

    if (displayHelp) {
        printUsage( argv[0] );
        exit(0);
    }

    // Declare commands
    declareCommands();

    // Initialize openGL context
    initGL (windowPosAndSize, headless, alwaysOnTop);

    struct stat st;                         // for files to watch
    float       timeLimit       = -1.0f;    // Time limit
    int         textureCounter  = 0;        // Number of textures to load
    bool        vFlip           = true;     // Flip state
    bool        fullFps         = false;

    //Load the the resources (textures)
    for (int i = 1; i < argc ; i++){
        std::string argument = std::string(argv[i]);

        if (    argument == "-x" || argument == "-y" ||
                argument == "-w" || argument == "--width" ||
                argument == "-h" || argument == "--height" ) {
            i++;
        }
        else if (   argument == "-l" ||
                    argument == "--headless") {
        }
        else if (   argument == "--verbose" ) {
            sandbox.verbose = true;
        }
        else if (   argument == "--debug" ) {
            sandbox.debug = true;
        }
        else if ( argument == "--cursor" ) {
            sandbox.cursor = true;
        }
        else if ( argument == "-s" || argument == "--sec" ) {
            i++;
            argument = std::string(argv[i]);
            timeLimit = toFloat(argument);
            std::cout << "// Will exit in " << timeLimit << " seconds." << std::endl;
        }
        else if ( argument == "-o" ) {
            i++;
            argument = std::string(argv[i]);
            if ( haveExt(argument, "png" )) {
                outputFile = argument;
                std::cout << "// Will save screenshot to " << outputFile  << " on exit." << std::endl;
            }
            else {
                std::cerr << "At the moment screenshots only support PNG formats" << std::endl;
            }
        }
        else if ( argument == "-e" ) {
            i++;
            execute_cmd = std::string(argv[i]);
        }
        else if ( argument == "-E" ) {
            i++;
            execute_cmd = std::string(argv[i]);
            execute_exit = true;
        }
        else if (argument == "-F" ) {
            fullFps = true;
        }
        else if ( sandbox.frag_index == -1 && (haveExt(argument,"frag") || haveExt(argument,"fs") ) ) {
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cerr << "Error watching file " << argv[i] << std::endl;
            }
            else {
                WatchFile file;
                file.type = FRAG_SHADER;
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);

                sandbox.frag_index = files.size()-1;
            }
        }
        else if ( sandbox.vert_index == -1 && ( haveExt(argument,"vert") || haveExt(argument,"vs") ) ) {
            if ( stat(argument.c_str(), &st) != 0) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                WatchFile file;
                file.type = VERT_SHADER;
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);

                sandbox.vert_index = files.size()-1;
            }
        }
        else if ( sandbox.geom_index == -1 && (  haveExt(argument,"ply") || haveExt(argument,"PLY") ||
                                            haveExt(argument,"obj") || haveExt(argument,"OBJ") ) ) {
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
        else if ( argument == "-vFlip" ) {
            vFlip = false;
        }
        else if (   haveExt(argument,"hdr") || haveExt(argument,"HDR") ||
                    haveExt(argument,"png") || haveExt(argument,"PNG") ||
                    haveExt(argument,"jpg") || haveExt(argument,"JPG") ||
                    haveExt(argument,"jpeg") || haveExt(argument,"JPEG")) {
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                Texture* tex = new Texture();

                if ( tex->load(argument, vFlip) ) {
                    std::string name = "u_tex"+toString(textureCounter);
                    sandbox.textures[name] = tex;

                    WatchFile file;
                    file.type = IMAGE;
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = vFlip;
                    files.push_back(file);

                    std::cout << "// " << argument << " loaded as: " << std::endl;
                    std::cout << "//    uniform sampler2D " << name  << ";"<< std::endl;
                    std::cout << "//    uniform vec2 " << name  << "Resolution;"<< std::endl;
                    textureCounter++;
                }
            }
        }
        else if ( argument == "-c" ) {
            i++;
            argument = std::string(argv[i]);
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cerr << "Error watching cubefile: " << argument << std::endl;
            }
            else {
                TextureCube* tex = new TextureCube();
                if ( tex->load(argument, vFlip) ) {
                    sandbox.setCubeMap(tex);

                    WatchFile file;
                    file.type = CUBEMAP;
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = vFlip;
                    files.push_back(file);

                    sandbox.addDefines("CUBE_MAP u_cubeMap");
                    sandbox.addDefines("SH_ARRAY u_SH");

                    std::cout << "// " << argument << " loaded as: " << std::endl;
                    std::cout << "//    uniform samplerCube u_cubeMap;"<< std::endl;
                    std::cout << "//    uniform vec3        u_SH[9];"<< std::endl;
                }
            }
        }
        else if ( argument.find("-D") == 0 ) {
            std::string define = argument.substr(2);
            sandbox.defines.push_back(define);
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
            i++;
            argument = std::string(argv[i]);
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                Texture* tex = new Texture();
                if (tex->load(argument, vFlip)) {
                    sandbox.textures[parameterPair] = tex;

                    WatchFile file;
                    file.type = IMAGE;
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = vFlip;
                    files.push_back(file);

                    std::cout << "// " << argument << " loaded as: " << std::endl;
                    std::cout << "//     uniform sampler2D " << parameterPair  << ";"<< std::endl;
                    std::cout << "//     uniform vec2 " << parameterPair  << "Resolution;"<< std::endl;
                }
            }
        }
    }

    // If no shader
    if ( sandbox.frag_index == -1 && sandbox.vert_index == -1 && sandbox.geom_index == -1 ) {
        printUsage(argv[0]);
        onExit();
        exit(EXIT_FAILURE);
    }

    // Start watchers
    fileChanged = -1;
    std::thread fileWatcher( &fileWatcherThread );
    std::thread cinWatcher( &cinWatcherThread );

    // Start working on the GL context
    filesMutex.lock();
    sandbox.setup(files);
    filesMutex.unlock();

    // Render Loop
    while ( isGL() && bRun.load() ) {
        // Update
        updateGL();

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        // Something change??
        if ( fileChanged != -1 ) {
            filesMutex.lock();
            sandbox.onFileChange( files, fileChanged );
            fileChanged = -1;
            filesMutex.unlock();
        }

        // If nothing in the scene change skip the frame and try to keep it at 30fps
        if (!fullFps && !sandbox.haveChange()) {
            usleep(REST_FPS);
            continue;
        }

        // Draw
        sandbox.draw();

        // Draw Cursor and Debug elements
        sandbox.drawUI();

        sandbox.drawDone();

        // Swap the buffers
        renderGL();

        if ( timeLimit >= 0.0 && getTime() >= timeLimit ) {
            bRun.store(false);
        }
    }

    // If is terminated by the windows manager, turn bRun off so the fileWatcher can stop
    if ( !isGL() ) {
        bRun.store(false);
    }

    onExit();

    // Wait for watchers to end
    fileWatcher.join();

    // Force cinWatcher to finish (because is waiting for input)
    pthread_t handler = cinWatcher.native_handle();
    pthread_cancel( handler );

    exit(0);
}

// Events
//============================================================================
void onKeyPress (int _key) {
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
    sandbox.onScroll(_yoffset);
}

void onMouseDrag(float _x, float _y, int _button) {
    sandbox.onMouseDrag(_x, _y, _button);
}

void onViewportResize(int _newWidth, int _newHeight) {
    sandbox.onViewportResize(_newWidth, _newHeight);
}

void onExit() {
    // Take a screenshot if it need
    sandbox.onScreenshot(outputFile);

    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // close openGL instance
    closeGL();

    // Delete the resources of Sandbox
    sandbox.clean();
}


//  Watching Thread
//============================================================================
void fileWatcherThread() {
    struct stat st;
    while ( bRun.load() ) {
        for ( uint i = 0; i < files.size(); i++ ) {
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
        usleep(500000);
    }
}

//  Command line Thread
//============================================================================
void cinWatcherThread() {
    while (!sandbox.isReady()) {
        usleep(REST_FPS);
    }

    if (execute_cmd.size() > 0) {
        bool resolve = false;
        for (unsigned int i = 0; i < commands.size(); i++) {
            if (beginsWith(execute_cmd, commands[i].begins_with)) {
                if (commands[i].exec(execute_cmd)) {
                    resolve = true;
                    break;
                }
            }
        }

        if (execute_exit && resolve) {
            bRun.store(false);
        }
    }

    std::string line;
    std::cout << "// > ";
    while (std::getline(std::cin, line)) {

        bool resolve = false;
        for (unsigned int i = 0; i < commands.size(); i++) {
            if (beginsWith(line, commands[i].begins_with)) {
                if (commands[i].exec(line)) {
                    resolve = true;
                    break;
                }
            }
        }

        // If nothing match maybe the user is trying to define the content of a uniform
        if (!resolve) {
            consoleMutex.lock();
            parseUniformData(line, &sandbox.uniforms_data);
            sandbox.flagChange();
            consoleMutex.unlock();
        }

        std::cout << "// > ";
    }
}