#include "engine.h"

#include "vera/shaders/defaultShaders.h"
#include "vera/ops/draw.h"


Engine::Engine() : m_enableCubemap (true) {
    verbose = true;
    help = false;
    cursor = false;
    m_initialized = false;
    m_sceneRender.dynamicShadows = true;
    uniforms.setSkyFlip(true);
    uniforms.setSkySize(2048/2);
    addDefine("GLSLVIEWER", vera::toString(GLSLVIEWER_VERSION_MAJOR) + vera::toString(GLSLVIEWER_VERSION_MINOR) + vera::toString(GLSLVIEWER_VERSION_PATCH) );
    setSource(FRAGMENT, vera::getDefaultSrc(vera::FRAG_DEFAULT));
    setSource(VERTEX, vera::getDefaultSrc(vera::VERT_DEFAULT_SCENE));
};

Engine::~Engine() {
};

void Engine::init() {
    vera::WindowProperties props;
    props.style = vera::EMBEDDED;
    vera::initGL(props);
    WatchFileList files;
    resetShaders( files );
}

void Engine::draw() {
    vera::textFont("default");

    uniforms.update();

    renderPrep();
    render();
    renderPost();
    renderUI();

    if (screenshotFile != "") {
        onScreenshot(screenshotFile);
        screenshotFile = "";
    }

    unflagChange();

    if (m_plot != PLOT_OFF)
        onPlot();

    m_initialized = true;
}

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
    WatchFileList files;
    resetShaders( files );
}

void Engine::setSkyGround(float _r, float _g, float _b) { 
    glm::vec3 c = uniforms.getGroundAlbedo();
    if (c.r != _r || c.g != _g || c.b != _b)
        uniforms.setGroundAlbedo(glm::vec3(_r, _g, _b));
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
    
    if (uniforms.cubemaps.size() == 1) {
        uniforms.activeCubemap = uniforms.cubemaps["default"];
        uniforms.setSunPosition( _light.getPosition() * glm::vec3(1.0, -1.0, 1.0) );
    }

    light->setPosition( _light.getPosition() );
    light->setType(vera::LIGHT_POINT);
    light->color = _light.color;
    light->direction = _light.direction;
    light->intensity = _light.intensity;
    light->falloff = _light.falloff;
}

void Engine::setFps(int _fps) {
    vera::setFps(_fps);
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
    vera::Camera* cam = uniforms.activeCamera;
    if (cam == nullptr)
        return;
    cam->setViewport(width, height);
}


void Engine::clearModels() {
    uniforms.clearModels();
}

bool Engine::haveTexture(const std::string& _name) {
    return uniforms.textures.find(_name) != uniforms.textures.end();
}

bool Engine::addTexture(const std::string& _name, int _width, int _height, int _id) {
    if (uniforms.textures.find(_name) == uniforms.textures.end()) {
        vera::Texture* tex = new vera::Texture();
        if (tex->load(_width, _height, _id)) {
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

bool Engine::loadTexture(const std::string& _name, int _width, int _height, int _channels, const std::vector<float>& _pixels) {
    if (uniforms.textures.find(_name) == uniforms.textures.end()) {
        vera::Texture* tex = new vera::Texture();
        if (tex->load(_width, _height, _channels, 32, _pixels.data())) {
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

bool Engine::addCubemap(const std::string& _name, int _width, int _height, int _channels, const std::vector<float>& _pixels) {
    if (uniforms.cubemaps.find(_name) == uniforms.cubemaps.end()) {
        vera::TextureCube* tex = new vera::TextureCube();
        if ( tex->load(_width, _height, _channels, _pixels.data(), true) ) {

            if (verbose) {
                std::cout << "// " << _name << " loaded as: " << std::endl;
                std::cout << "uniform samplerCube u_cubeMap;"<< std::endl;
                std::cout << "uniform vec3        u_SH[9];"<< std::endl;
            }

            uniforms.cubemaps[_name] = tex;
            uniforms.activeCubemap = uniforms.cubemaps[_name];

            enableCubemap(true);

            return true;
        }
        else
            delete tex;
    }
    return false;
}

void Engine::setUniform(const std::string& _name, const std::vector<float>& _values) {
    uniforms.set(_name, _values, false);
}

void Engine::enableCubemap(bool _value) {
    m_enableCubemap = _value;
    if (_value) {
        addDefine("SCENE_SH_ARRAY", "u_SH");
        addDefine("SCENE_CUBEMAP", "u_cubeMap");
    }
    else {
        delDefine("SCENE_SH_ARRAY");
        delDefine("SCENE_CUBEMAP");
    }
}

void Engine::showHistogram(bool _value) { 
    if (_value) {
        m_plot = PLOT_RGB;
        m_plot_shader.setSource(vera::getDefaultSrc(vera::FRAG_PLOT), vera::getDefaultSrc(vera::VERT_DYNAMIC_BILLBOARD));
        m_plot_shader.addDefine("PLOT_VALUE", "color += stroke(fract(st.x * 5.0), 0.5, 0.025) * 0.1;");
    }
    else {
        m_plot = PLOT_OFF; 
        m_plot_shader.delDefine("PLOT_VALUE"); 
    }
    m_update_buffers = true;
}

void Engine::setFxaa(bool _value) { 
    fxaa = _value; 
    
    WatchFileList files;
    resetShaders( files );
    flagChange();
}