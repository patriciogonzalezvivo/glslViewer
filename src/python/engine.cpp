#include "engine.h"

#include "vera/shaders/defaultShaders.h"

WatchFileList   files;

Engine::Engine() {
    vera::WindowProperties props;
    props.style = vera::EMBEDDED;
    vera::initGL(props);

    verbose = true;

    addDefine("GLSLVIEWER", vera::toString(GLSLVIEWER_VERSION_MAJOR) + vera::toString(GLSLVIEWER_VERSION_MINOR) + vera::toString(GLSLVIEWER_VERSION_PATCH) );

    // LOAD DEFAULT SHADER
    setSource(FRAGMENT, R"(#ifdef GL_ES
precision highp float;
#endif

uniform vec3        u_camera;

uniform vec2        u_resolution;
uniform float       u_time;
uniform int         u_frame;

varying vec4        v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4        v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

void main(void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 pos = v_position.xyz;

    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;

    color.rgb = pos;

    #ifdef MODEL_VERTEX_COLOR
    color = v_color;
    #endif

    #ifdef MODEL_VERTEX_NORMAL
    color.rgb = v_normal * 0.5 + 0.5;
    #endif

    #ifdef MODEL_VERTEX_TEXCOORD
    vec2 uv = v_texcoord;
    #else
    vec2 uv = st;
    #endif
    
    color.rg = uv;
    
    gl_FragColor = color;
})");

    setSource(VERTEX, vera::getDefaultSrc(vera::VERT_DEFAULT_SCENE));
    resetShaders( files );

    help = false;
    cursor = false;
    m_sceneRender.dynamicShadows = true;
};

Engine::~Engine() {

};

void Engine::loadMesh(const std::string& _name, const vera::Mesh& _mesh) {
    uniforms.models[_name] = new vera::Model(_name, _mesh);
    m_sceneRender.loadScene(uniforms);
    m_sceneRender.uniformsInit(uniforms);
    flagChange();
}

void Engine::loadImage(const std::string& _name, const std::string& _path, bool _flip) {
    uniforms.addTexture(_name, _path, _flip);
    flagChange();
}

void Engine::loadShaders() {
    resetShaders(files);
    flagChange();
}

void Engine::setCamera(const vera::Camera& _cam) {
    vera::Camera* cam = uniforms.activeCamera;
    if (cam == nullptr)
        return;
    *cam = _cam;
}

void Engine::setSun(const vera::Light& _light) {
    vera::Light* light = uniforms.lights["default"];
    if (light == nullptr)
        return;

    light->setPosition( _light.getPosition() );
    light->setType(vera::LIGHT_POINT);
    light->color = _light.color;
    light->direction = _light.direction;
    light->intensity = _light.intensity;
    light->falloff = _light.falloff;

}

void Engine::setSunPosition(float _az, float _elev, float _distance) {
    uniforms.setSunPosition(_az, _elev, _distance);
}

void Engine::setMeshTransformMatrix(    const std::string& _name, 
                                        float x1, float y1, float z1, float w1,
                                        float x2, float y2, float z2, float w2,
                                        float x3, float y3, float z3, float w3,
                                        float x4, float y4, float z4, float w4 ) {
    vera::ModelsMap::iterator it = uniforms.models.find(_name);
    if (it != uniforms.models.end())
        it->second->setTransformMatrix(glm::mat4(x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4));
}


void Engine::resize(const size_t& width, const size_t& height) {
    onViewportResize(width, height);
    vera::setViewport(width, height);
}



void Engine::clearModels() {
    uniforms.clearModels();
    // uniforms.clearMaterials();
    // uniforms.clearShaders();
    // m_sceneRender.clearScene();
}

void Engine::draw() {

    // PREP for main render:
    //  - update uniforms
    //  - render buffers, double buffers and pyramid convolutions
    //  - render lighmaps (3D scenes only)
    //  - render normal/possition/extra g buffers (3D scenes only)
    //  - start the recording FBO (when recording)
    renderPrep();

    // Render the main 2D Shader on a billboard or 3D Scene when there is geometry models
    // Note: if the render require multiple views of the render (for quilts or VR) it happens here.
    render();

    // POST render:
    //  - post-processing render pass
    //  - close recording FBO (when recording)
    renderPost();

    renderUI();

    // Finish rendering triggering some events like
    //  - anouncing the first pass have been made
    // renderDone();

    // // RECORD
    // if (isRecording()) {
    //     onScreenshot( vera::toString( getRecordingCount() , 0, 5, '0') + ".png");
    //     recordingFrameAdded();
    // }
    // // SCREENSHOT 
    // else if (screenshotFile != "") {
    //     onScreenshot(screenshotFile);
    //     screenshotFile = "";
    // }

    unflagChange();
    if (!m_initialized) {
        m_initialized = true;
        vera::updateViewport();
        flagChange();
    }
}


bool Engine::haveTexture(const std::string& _name) {
    return uniforms.textures.find(_name) != uniforms.textures.end();
}

bool Engine::addTexture(const std::string& _name, int _width, int _height,const std::vector<float>& _pixels) {
    if (uniforms.textures.find(_name) == uniforms.textures.end()) {
        vera::Texture* tex = new vera::Texture();
        if (tex->load(_width, _height, 4, 32, _pixels.data())) {
            uniforms.textures[_name] = tex;

            if (verbose) {
                std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
                std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;
            }

            return true;
        }
        else
            delete tex;
    }
    return false;
}

bool Engine::haveCubemap(const std::string& _name) {
    return uniforms.cubemaps.find(_name) != uniforms.cubemaps.end();
}

bool Engine::addCubemap(const std::string& _name, int _width, int _height, const std::vector<float>& _pixels) {
    if (uniforms.cubemaps.find(_name) == uniforms.cubemaps.end()) {
        vera::TextureCube* tex = new vera::TextureCube();
        if ( tex->load(_width, _height, 4, _pixels.data(), true) ) {

            if (verbose) {
                std::cout << "// " << _name << " loaded as: " << std::endl;
                std::cout << "uniform samplerCube u_cubeMap;"<< std::endl;
                std::cout << "uniform vec3        u_SH[9];"<< std::endl;
            }

            uniforms.cubemaps[_name] = tex;
            uniforms.activeCubemap = uniforms.cubemaps[_name];

            addDefine("SCENE_SH_ARRAY", "u_SH");
            addDefine("SCENE_CUBEMAP", "u_cubeMap");

            return true;
        }
        else
            delete tex;
    }
    return false;
}