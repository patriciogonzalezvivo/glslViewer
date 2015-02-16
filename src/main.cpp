#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <map>

#include <FreeImage.h>
#include "inotify-cxx.h"

#include "gl.h"
#include "utils.h"
#include "shader.h"
#include "vertexLayout.h"
#include "vboMesh.h"
#include "texture.h"

#include <GLES2/gl2ext.h>

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

VboMesh* canvas;

//  Time
struct timeval tv;
unsigned long long timeStart;
unsigned long long timeNow;
float timeSec = 0.0f;
float timeLimit = 0.0f;

//============================================================================

VboMesh* rect(float _x, float _y, float _w, float _h){
    
    std::vector<VertexLayout::VertexAttrib> attribs;
    attribs.push_back({"a_position", 3, GL_FLOAT, false, 0});
    attribs.push_back({"a_texcoord", 2, GL_FLOAT, false, 0});
    VertexLayout* vertexLayout = new VertexLayout(attribs);

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

    VboMesh* mesh = new VboMesh(vertexLayout);
    mesh->addVertices((GLbyte*)vertices.data(), vertices.size());
    mesh->addIndices(indices.data(), indices.size());

    return mesh;
}

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

    canvas = rect(0.0,0.0,1.0,1.0);
}

static void draw(float _w, float _h){

    // GLenum error = glGetError();
    // if(error != GL_NO_ERROR){
    //     std::cerr << "Error: " << error << std::endl;
    // }

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

	timeNow = 	(unsigned long long)(tv.tv_sec) * 1000 +
				(unsigned long long)(tv.tv_usec) / 1000;

 	timeSec = (timeNow - timeStart)*0.001;

    shader.use();
    shader.sendUniform("u_time", timeSec);
    shader.sendUniform("u_mouse", mouse.x, mouse.y);
    shader.sendUniform("u_resolution",_w, _h);
    
    // ShaderToy Specs
    shader.sendUniform("iGlobalTime", timeSec);
    shader.sendUniform("iMouse", mouse.x, mouse.y, (mouse.button==1)?1.0:0.0, (mouse.button==2)?1.0:0.0);
    shader.sendUniform("iResolution",_w, _h, 0.0f);

    unsigned int index = 0;
    for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
        shader.sendUniform(it->first,it->second,index);
        shader.sendUniform(it->first+"Resolution",it->second->getWidth(),it->second->getHeight());
        index++;
    }

    canvas->draw(&shader);
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

   delete canvas;
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

    fragFile = "none";

    for (int i = 1; i < argc ; i++){
        if ( std::string(argv[i]).find(".frag") != std::string::npos) {
            fragFile = std::string(argv[i]);
            break;
        }
    }

    if(argc < 2 || fragFile == "none"){
		std::cerr << "GLSL render that updates changes instantly.\n";
		std::cerr << "Usage: " << argv[0] << " shader.frag [-textureNameA texture.png] [-l] [-s] [-x x] [-y y] [-w width] [-height]\n";

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

            // Start OpenGL ES
            bcm_host_init();

            // Clear application state
            memset( state, 0, sizeof( *state ) );

	        int32_t success = 0;
            EGLBoolean result;
            EGLint num_config;
    
            static EGL_DISPMANX_WINDOW_T nativewindow;
    
            DISPMANX_ELEMENT_HANDLE_T dispman_element;
            DISPMANX_DISPLAY_HANDLE_T dispman_display;
            DISPMANX_UPDATE_HANDLE_T dispman_update;
            VC_RECT_T dst_rect;
            VC_RECT_T src_rect;
    
            static const EGLint attribute_list[] = {
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_NONE
            };
    
            static const EGLint context_attributes[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE
            };

            EGLConfig config;
    
            // get an EGL display connection
            state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            assert(state->display!=EGL_NO_DISPLAY);
    
            // initialize the EGL display connection
            result = eglInitialize(state->display, NULL, NULL);
            assert(EGL_FALSE != result);
    
            // get an appropriate EGL frame buffer configuration
            result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
            assert(EGL_FALSE != result);
    
            // get an appropriate EGL frame buffer configuration
            result = eglBindAPI(EGL_OPENGL_ES_API);
            assert(EGL_FALSE != result);
    
            // create an EGL rendering context
            state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
            assert(state->context!=EGL_NO_CONTEXT);
    
            // create an EGL window surface
            success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
            assert( success >= 0 );

            uint32_t x = 0;
            uint32_t y = 0;
            uint32_t w = state->screen_width;
            uint32_t h = state->screen_height;

            std::string outputFile = "";
            bool bDither = false;

            //Setup
            for (int i = 1; i < argc ; i++){

                if ( std::string(argv[i]) == "-u" ) {
                    fragAutoHeader = true;
                } else if ( std::string(argv[i]) == "-d" || 
                            std::string(argv[i]) == "--dither" ) {
                    bDither = true;
                } else if ( std::string(argv[i]) == "-s" ||
                            std::string(argv[i]) == "--sec") {
                    i++;
                    timeLimit = getFloat(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "-o" ){
                    i++;
                    outputFile = std::string(argv[i]);
                } else if ( std::string(argv[i]) == "-x" ) {
                    i++;
                    x = getInt(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "-y" ) {
                    i++;
                    y = getInt(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "-w" || 
                            std::string(argv[i]) == "--width" ) {
                    i++;
                    w = getInt(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "-h" || 
                            std::string(argv[i]) == "--height") {
                    i++;
                    h = getInt(std::string(argv[i]));
                } else if ( std::string(argv[i]) == "--square") {
                    if (state->screen_width > state->screen_height) {
                        x = state->screen_width/2-state->screen_height/2;
                    } else {
                        y = state->screen_height/2-state->screen_width/2;
                    }
                    w = h = MIN(state->screen_width,state->screen_height);
                } else if ( std::string(argv[i]) == "-l" || 
                            std::string(argv[i]) == "--life-coding" ){
                    x = w-500;
                    w = h = 500;
                }
            }

            dst_rect.x = x;
            dst_rect.y = y;
            dst_rect.width = w;
            dst_rect.height = h;
    
            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = w << 16;
            src_rect.height = h << 16;
    
            dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
            dispman_update = vc_dispmanx_update_start( 0 );

            dispman_element = vc_dispmanx_element_add( dispman_update, dispman_display,
                                               0/*layer*/, &dst_rect, 0/*src*/,
                                               &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);
    
            nativewindow.element = dispman_element;
            nativewindow.width = w;
            nativewindow.height = h;
            vc_dispmanx_update_submit_sync( dispman_update );
    
            state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
            assert(state->surface != EGL_NO_SURFACE);
     
            // connect the context to the surface
            result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
            assert(EGL_FALSE != result);
                
            // Set background color and clear buffers
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

            glClear(GL_COLOR_BUFFER_BIT );

	        // Prepare viewport
            glViewport(0, 0, w, h );

            //Load the the resources (textures)
            for (int i = 1; i < argc ; i++){
                if (std::string(argv[i]) == "-x" || 
                    std::string(argv[i]) == "-y" || 
                    std::string(argv[i]) == "-w" || 
                    std::string(argv[i]) == "--width" || 
                    std::string(argv[i]) == "-h" || 
                    std::string(argv[i]) == "--height" ||
                    std::string(argv[i]) == "-o" ||
                    std::string(argv[i]) == "-s" || 
                    std::string(argv[i]) == "--sec") {
                    i++;
                } else if ( std::string(argv[i]) == "-u" ||
                            std::string(argv[i]) == "-d" || 
                            std::string(argv[i]) == "--dither" || 
                            std::string(argv[i]) == "--square" ||
                            std::string(argv[i]) == "-l" || 
                            std::string(argv[i]) == "--life-coding"  ) {

                } else if (std::string(argv[i]).find("-") == 0) {
                    std::string parameterPair = std::string(argv[i]).substr(std::string(argv[i]).find_last_of('-')+1);
                    i++;
                    Texture* tex = new Texture();
                    if( tex->load(std::string(argv[i])) ){
                        textures[parameterPair] = tex;
                    }
                }
            }

            // Setup
            setup();
           
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
             
            // Clear the background
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            bool bPlay = true;
            // Render Loop
            while (bPlay) {
                // Update
                updateMouse();
                
                // Draw
                draw(w,h);
                
                eglSwapBuffers(state->display, state->surface); 

                if (timeLimit > 0.0 && timeSec > timeLimit){

                    if(outputFile != ""){
                        FreeImage_Initialise();
                        FREE_IMAGE_FORMAT fif = FIF_PNG;//FreeImage_GetFileType(outputFile.c_str());

                        if (fif != FIF_UNKNOWN){
                            BYTE* pixels = new BYTE[w*h*4];
                            glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                            
                            int bpp = 32;
                            int pitch = ((((bpp * w) + 31) / 32) * 4);
                            
                            // FIBITMAP* bmp = FreeImage_AllocateT(FIT_RGBA16,w,h,bpp); 
                            // BYTE* bmpBits = FreeImage_GetBits(bmp);
                            // if (bmpBits != NULL) {
                            //     int srcStride = pitch;
                            //     int dstStride = FreeImage_GetPitch(bmp);
                            //     BYTE* src = pixels;
                            //     BYTE* dst = bmpBits;
                            //     for(int i = 0; i < h; i++){
                            //         memcpy(dst, src, srcStride);
                            //         src += srcStride;
                            //         dst += dstStride;
                            //     }
                            // }

                            FIBITMAP* bmp = FreeImage_ConvertFromRawBits(pixels, w, h, pitch, bpp, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK, false);
                            
                            if(bDither){
                                FIBITMAP* dither = FreeImage_Dither(bmp,FID_FS);
                                FreeImage_Save(fif,dither,outputFile.c_str());
                                if (dither != NULL){
                                    FreeImage_Unload(dither);
                                }
                            } else {
                                FreeImage_Save(fif,bmp,outputFile.c_str());
                            }
                            
                            if (bmp != NULL){
                                FreeImage_Unload(bmp);
                            }
                        }
                    }
                    
                    bPlay = false;
                }

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
