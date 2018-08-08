#include <sys/stat.h>
#include <unistd.h>

#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

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

std::mutex uniformsMutex;

std::mutex screenshotMutex;

std::string outputFile = "";

Sandbox sandbox;

//================================================================= Threads
void fileWatcherThread();
void cinWatcherThread();

//================================================================= Functions
void onExit();
void printUsage(char * executableName) {
    std::cerr << "Usage: " << executableName << " <shader>.frag [<shader>.vert] [<mesh>.(obj/.ply)] [<texture>.(png/jpg)] [-<uniformName> <texture>.(png/jpg)] [-vFlip] [-x <x>] [-y <y>] [-w <width>] [-h <height>] [-l] [--square] [-s/--sec <seconds>] [-o <screenshot_file>.png] [--headless] [-c/--cursor] [-I<include_folder>] [-D<define>] [-v/--verbose] [--help]\n";
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
        else if (   argument == "-v" || 
                    argument == "--verbose" ) {
            sandbox.verbose = true;
        }
        else if ( argument == "-c" || argument == "--cursor" ) {
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
                file.type = "fragment";
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
                file.type = "vertex";
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
                file.type = "geometry";
                file.path = argument;
                file.lastChange = st.st_mtime;
                files.push_back(file);
                sandbox.iGeom = files.size()-1;
            }
        }
        else if ( argument == "-vFlip" ) {
            sandbox.vFlip = false;
        }
        else if (   haveExt(argument,"png") || haveExt(argument,"PNG") ||
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
                    file.type = "image";
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = sandbox.vFlip;
                    files.push_back(file);

                    std::cout << "// Loading " << argument << " as the following uniform: " << std::endl;
                    std::cout << "//    uniform sampler2D " << name  << "; // loaded"<< std::endl;
                    std::cout << "//    uniform vec2 " << name  << "Resolution;"<< std::endl;
                    textureCounter++;
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
                    file.type = "image";
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = sandbox.vFlip;
                    files.push_back(file);

                    std::cout << "// Loading " << argument << " as the following uniform: " << std::endl;
                    std::cout << "//     uniform sampler2D " << parameterPair  << "; // loaded"<< std::endl;
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
    sandbox.setup(files);

    // Render Loop
    while ( isGL() && bRun.load() ) {
        // Update
        updateGL();

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        // Something change??
        if ( fileChanged != -1 ) {
            sandbox.onFileChange( files, fileChanged );

            filesMutex.lock();
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
            printf("%f\n", getFPS());
        }
        else if (line == "delta") {
            // std::cout << getDelta() << std::endl;
            printf("%f\n", getDelta());
        }
        else if (line == "time") {
            // std::cout << getTime() << std::endl;
            printf("%f\n", getTime());
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
            std::cout << sandbox.get3DView() << std::endl;
        }
        else if (line == "frag") {
            std::cout << sandbox.getFragmentSource() << std::endl;
        }
        else if (line == "vert") {
            std::cout << sandbox.getVertexSource() << std::endl;
        }
        else if (line == "buffers") {
            std::cout << sandbox.getTotalBuffers() << std::endl;
        }
        else if (beginsWith(line, "screenshot")) {
            if (line == "screenshot" && outputFile != "") {
                screenshotMutex.lock();
                sandbox.screenshotFile = outputFile;
                screenshotMutex.unlock();
            }
            else {
                std::vector<std::string> values = split(line,' ');
                if (values.size() == 2) {
                    screenshotMutex.lock();
                    sandbox.screenshotFile = values[1];
                    screenshotMutex.unlock();
                }
            }
        }
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
        else {
            uniformsMutex.lock();
            parseUniforms(line, &sandbox.uniforms);
            uniformsMutex.unlock();
        }
    }
}

void onKeyPress (int _key) {
    if (_key == 's' || _key == 'S') {
        sandbox.onScreenshot(outputFile);
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
