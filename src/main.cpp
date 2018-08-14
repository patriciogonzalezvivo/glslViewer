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
#include "ui/cursor.h"
#include "tools/fs.h"
#include "tools/text.h"

// GLOBAL VARIABLES
//============================================================================
//
std::atomic<bool> bRun(true);

//  List of FILES to watch and the variable to communicate that between process
WatchFileList files;
std::mutex filesMutex;
int fileChanged;

std::mutex consoleMutex;
std::string outputFile = "";

std::string version = "1.5.1";

// Here is where all the magic happens
Sandbox sandbox;

//================================================================= Threads
void fileWatcherThread();
void cinWatcherThread();

//================================================================= Functions
void onExit();
void printUsage(char * executableName) {
    std::cerr << "// GlslViewer " << version << " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )" << std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// Swiss army knife of GLSL Shaders. Loads frag/vertex shaders, images and " << std::endl;
    std::cerr << "// geometries. Will reload automatically on changes. Support for multi  "<< std::endl;
    std::cerr << "// buffers, baground and postprocessing passes. Can render headlessly and "<< std::endl;
    std::cerr << "// into a file. Use POSIX STANDARD CONSOLE IN/OUT to comunicate (uniforms,"<< std::endl;
    std::cerr << "// camera position, scene description and  commands) to and with other "<< std::endl;
    std::cerr << "// programs. Compatible with Linux and MacOS, runs from command line with"<< std::endl;
    std::cerr << "// out X11 enviroment on RaspberryPi devices. "<< std::endl;
    std::cerr << "// "<< std::endl;
    std::cerr << "// Usage: " << executableName << " <shader>.frag [<shader>.vert] [<mesh>.(obj/.ply)] [<texture>.(png/jpg)] [-<uniformName> <texture>.(png/jpg/hdr)] [-c <cubemap>.hdr] [-vFlip] [-x <x>] [-y <y>] [-w <width>] [-h <height>] [-l] [--square] [-s/--sec <seconds>] [-o <screenshot_file>.png] [--headless] [--cursor] [-I<include_folder>] [-D<define>] [-v/--version] [--verbose] [--help]\n";
}

void printHelp() {
    std::cout << "// GlslViewer " << version << " by Patricio Gonzalez Vivo ( patriciogonzalezvivo.com )" << std::endl; 
    std::cout << "//" << std::endl;
    std::cout << "// help           return this list of commands." << std::endl;
    std::cout << "//" << std::endl;
    std::cout << "// window_width   return the width of the windows." << std::endl;
    std::cout << "// window_height  return the height of the windows." << std::endl;
    std::cout << "// screen_size    return the width and height of the screen." << std::endl;
    std::cout << "// viewport       return the size of the viewport (u_resolution)." << std::endl;
    std::cout << "// pixel_density  return the pixel density" << std::endl;
    std::cout << "//" << std::endl;
    std::cout << "// date           return u_date as YYYY, M, D and Secs." << std::endl;
    std::cout << "// time           return u_time, the elapsed time." << std::endl;
    std::cout << "// delta          return u_delta, the secs between frames." << std::endl;
    std::cout << "// fps            return u_fps, the number of frames per second." << std::endl;
    std::cout << "// mouse_x        return the position of the mouse in x." << std::endl;
    std::cout << "// mouse_y        return the position of the mouse in x." << std::endl;
    std::cout << "// mouse          return the position of the mouse (u_mouse)." << std::endl;
    std::cout << "//" << std::endl; 
    std::cout << "// files          return a list of files" << std::endl;
    std::cout << "// buffers        return a list of buffers as their uniform name." << std::endl;
    std::cout << "// uniforms       return a list of uniforms and their values." << std::endl;
    std::cout << "// textures       return a list of textures as their uniform name and path." << std::endl;
    std::cout << "//" << std::endl;
    std::cout << "// defines            return a list of active defines" << std::endl;
    std::cout << "// define,<DEFINE>    add a define to the shader" << std::endl;
    std::cout << "// undefine,<DEFINE>  remove a define on the shader" << std::endl;
    std::cout << "//" << std::endl;
    std::cout << "// view3d                         returns the position of the camera," << std::endl;
    std::cout << "//                                the up-vector and the center of model." << std::endl;
    std::cout << "// camera_distance                returns camera to target distance." << std::endl;
    std::cout << "// camera_distance[,distance]     set the camera distance to the target." << std::endl;
    std::cout << "// camera_position                returns the position of the camera." << std::endl;
    std::cout << "// camera_position[,x][,y][,z]    set the position." << std::endl;
    std::cout << "//" << std::endl;
    std::cout << "// screenshot[,filename]      saves a screenshot to a filename." << std::endl;
    std::cout << "// sequence,<A_sec>,<B_sec>   saves a sequence of images from A to B second." << std::endl;
    std::cout << "// frag[,filename]            returns or save the fragment shader source code." << std::endl;
    std::cout << "// frag_dependencies          returns all the fragment shader dependencies." << std::endl;
    std::cout << "// vert[,filename]            return or save the vertex shader source code." << std::endl;
    std::cout << "// vert_dependencies          returns all the vertex shader dependencies." << std::endl;
    std::cout << "//" << std::endl;
    std::cout << "// version        return glslViewer version" << std::endl;
    std::cout << "// exit           close glslViewer" << std::endl;
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
        #ifdef PLATFORM_RPI
        else if (   std::string(argv[i]) == "-l" ||
                    std::string(argv[i]) == "--life-coding" ){
            windowPosAndSize.x = windowPosAndSize.z - 500;
            windowPosAndSize.z = windowPosAndSize.w = 500;
        }
        #endif
    }

    if (displayHelp) {
        printUsage( argv[0] );
        exit(0);
    }

    // Initialize openGL context
    initGL (windowPosAndSize, headless);

    Cursor cursor;  // Cursor
    struct stat st; // for files to watch
    float timeLimit = -1.0f; //  Time limit
    int textureCounter = 0; // Number of textures to load

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
        else if ( argument == "--cursor" ) {
            cursor.init();
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
        else if ( sandbox.iFrag == -1 && (haveExt(argument,"frag") || haveExt(argument,"fs") ) ) {
            if ( stat(argument.c_str(), &st) != 0 ) {
                std::cerr << "Error watching file " << argv[i] << std::endl;
            }
            else {
                WatchFile file;
                file.type = FRAG_SHADER;
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);

                sandbox.iFrag = files.size()-1;
            }
        }
        else if ( sandbox.iVert == -1 && ( haveExt(argument,"vert") || haveExt(argument,"vs") ) ) {
            if ( stat(argument.c_str(), &st) != 0) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                WatchFile file;
                file.type = VERT_SHADER;
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);

                sandbox.iVert = files.size()-1;
            }
        }
        else if ( sandbox.iGeom == -1 && (  haveExt(argument,"ply") || haveExt(argument,"PLY") ||
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

                sandbox.iGeom = files.size()-1;
            }
        }
        else if ( argument == "-vFlip" ) {
            sandbox.vFlip = false;
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

                if ( tex->load(argument, sandbox.vFlip) ) {
                    std::string name = "u_tex"+toString(textureCounter);
                    sandbox.textures[name] = tex;

                    WatchFile file;
                    file.type = IMAGE;
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = sandbox.vFlip;
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
                if ( tex->load(argument, sandbox.vFlip) ) {
                    sandbox.setCubeMap(tex);

                    WatchFile file;
                    file.type = CUBEMAP;
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = sandbox.vFlip;
                    files.push_back(file);

                    std::cout << "// " << argument << " loaded as: " << std::endl;
                    std::cout << "//    uniform samplerCube u_cubeMap;"<< std::endl;
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
                if (tex->load(argument, sandbox.vFlip)) {
                    sandbox.textures[parameterPair] = tex;

                    WatchFile file;
                    file.type = IMAGE;
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = sandbox.vFlip;
                    files.push_back(file);

                    std::cout << "// " << argument << " loaded as: " << std::endl;
                    std::cout << "//     uniform sampler2D " << parameterPair  << ";"<< std::endl;
                    std::cout << "//     uniform vec2 " << parameterPair  << "Resolution;"<< std::endl;
                }
            }
        }
    }

    // If no shader
    if ( sandbox.iFrag == -1 && sandbox.iVert == -1 && sandbox.iGeom == -1 ) {
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

        // Draw
        sandbox.draw();

        // Draw Cursor
        cursor.draw();

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
                    fileChanged = i;
                    files[i].lastChange = date;
                    filesMutex.unlock();
                }
            }
        }
        usleep(500000);
    }
}

void cinWatcherThread() {
    std::string line;
    std::cout << "// > ";
    while (std::getline(std::cin, line)) {
        // GET ONLY
        // 
        if (line == "help") {
            printHelp();
        }
        else if (line == "fps") {
            // Force the output in floats
            printf("%f\n", getFPS());
        }
        else if (line == "delta") {
            // Force the output in floats
            printf("%f\n", getDelta());
        }
        else if (line == "time") {
            // Force the output in floats
            printf("%f\n", getTime());
        }
        else if (line == "date") {
            glm::vec4 date = getDate();
            std::cout << date.x << ',' << date.y << ',' << date.z << ',' << date.w << std::endl;
        }
        else if (line == "view3d") {
            std::cout << sandbox.get3DView() << std::endl;
        }
        else if (beginsWith(line, "frag,")) {
            std::vector<std::string> values = split(line,',');
            if (values.size() == 2) {
                std::ofstream out(values[1]);
                out << sandbox.getFragmentSource();
                out.close();
            }
        }
        else if (line == "frag") {
            std::cout << sandbox.getFragmentSource() << std::endl;
        }
        else if (line == "frag_dependencies") {
            for (unsigned int i = 0; i < sandbox.frag_dependencies.size(); i++) {
                std::cout << sandbox.frag_dependencies[i] << std::endl;
            }
        }
        else if (beginsWith(line, "vert,")) {
            std::vector<std::string> values = split(line,',');
            if (values.size() == 2) {
                std::ofstream out(values[1]);
                out << sandbox.getVertexSource();
                out.close();
            }
        }
        else if (line == "vert") {
            std::cout << sandbox.getVertexSource() << std::endl;
        }
        else if (line == "vert_dependencies") {
            for (unsigned int i = 0; i < sandbox.frag_dependencies.size(); i++) {
                std::cout << sandbox.frag_dependencies[i] << std::endl;
            }
        }
        else if (line == "files") {
            for (unsigned int i = 0; i < files.size(); i++) { 
                std::cout << std::setw(2) << i << "," << std::setw(12) << toString(files[i].type) << "," << files[i].path << std::endl;
            }
        }
        else if (line == "buffers") {
            for (int i = 0; i < sandbox.getTotalBuffers(); i++) {
                std::cout << "u_buffer" << i << std::endl;
            }
        }
        else if (line == "defines") {
            for (unsigned int i = 0; i < sandbox.defines.size(); i++) {
                std::cout << sandbox.defines[i] << std::endl;
            }
        }
        else if (line == "uniforms") {
            for (UniformList::iterator it= sandbox.uniforms.begin(); it != sandbox.uniforms.end(); ++it) {
                std::cout << it->first;
                for (int i = 0; i < it->second.size; i++) {
                    std::cout << ',' << it->second.value[i];
                }
                std::cout << std::endl;
            }
        }
        else if (line == "textures") {
            for (TextureList::iterator it = sandbox.textures.begin(); it != sandbox.textures.end(); ++it) {
                std::cout << it->first << ',' << it->second->getFilePath() << std::endl;
            }
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
        else if (line == "version") {
            std::cout << version << std::endl;
        }
        // GET/SET 
        //
        else if (beginsWith(line, "camera_distance")) {
            std::vector<std::string> values = split(line,',');
            if (values.size() == 2) {
                sandbox.getCamera().setDistance(toFloat(values[1]));
            }
            else {
                std::cout << sandbox.getCamera().getDistance() << std::endl;
            }
        }
        else if (beginsWith(line, "camera_position")) {
            std::vector<std::string> values = split(line,',');
            if (values.size() == 4) {
                sandbox.getCamera().setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            }
            else {
                glm::vec3 pos = sandbox.getCamera().getPosition();
                std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            }
        }
        // ACTIONS
        else if (line == "q" || line == "quit" || line == "exit") {
            bRun.store(false);
        }
        else if (beginsWith(line, "screenshot")) {
            if (line == "screenshot" && outputFile != "") {
                consoleMutex.lock();
                sandbox.screenshotFile = outputFile;
                consoleMutex.unlock();
            }
            else {
                std::vector<std::string> values = split(line,',');
                if (values.size() == 2) {
                    consoleMutex.lock();
                    sandbox.screenshotFile = values[1];
                    consoleMutex.unlock();
                }
            }
        }
        else if (beginsWith(line, "sequence")) {
            std::vector<std::string> values = split(line,',');
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
                    usleep(10000);
                }
            }
        }
        // SET ONLY
        else if (beginsWith(line, "define")) {
            std::vector<std::string> values = split(line,',');
            if (values.size() == 2) {
                consoleMutex.lock();
                sandbox.addDefines( values[1] ); 
                consoleMutex.unlock();
                filesMutex.lock();
                fileChanged = sandbox.iFrag;
                filesMutex.unlock();
            }
        }
        else if (beginsWith(line, "undefine")) {
            std::vector<std::string> values = split(line,',');
            if (values.size() == 2) {
                consoleMutex.lock();
                sandbox.delDefines( values[1] ); 
                consoleMutex.unlock();
                filesMutex.lock();
                fileChanged = sandbox.iFrag;
                filesMutex.unlock();
            }
        }
        else {
            consoleMutex.lock();
            parseUniforms(line, &sandbox.uniforms);
            consoleMutex.unlock();
        }

        std::cout << "// > ";
    }
}

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
