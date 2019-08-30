#include "sandbox.h"

#include <sys/stat.h>   // stat
#include <algorithm>    // std::find
#include <math.h>

#include "window.h"

#include "tools/text.h"
#include "tools/image.h"
#include "tools/shapes.h"

#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "default_shaders.h"

#define FRAME_DELTA 0.03333333333

// ------------------------------------------------------------------------- CONTRUCTOR
Sandbox::Sandbox(): 
    frag_index(-1), vert_index(-1), geom_index(-1),
    verbose(false), cursor(true), debug(false),
    // Main Vert/Frag/Geom
    m_frag_source(""), m_vert_source(""), m_vbo(nullptr),
    // Background
    m_background_enabled(false),
    // Buffers
    m_buffers_total(0),
    // PostProcessing
    m_postprocessing_enabled(false),
    // Geometry helpers
    m_billboard_vbo(nullptr), m_cross_vbo(nullptr),
    // Record
    m_record_start(0.0f), m_record_head(0.0f), m_record_end(0.0f), m_record_counter(0), m_record(false),
    // Scene
    m_view2d(1.0), m_frame(0), m_change(true) {

    // Adding default deines
    addDefine("GLSLVIEWER 1");
    // Define PLATFORM
    #ifdef PLATFORM_OSX
    addDefine("PLATFORM_OSX");
    #endif

    #ifdef PLATFORM_LINUX
    addDefine("PLATFORM_LINUX");
    #endif

    #ifdef PLATFORM_RPI
    addDefine("PLATFORM_RPI");
    #endif

    // TIME UNIFORMS
    //
    uniforms.functions["u_time"] = UniformFunction( "float", 
    [this](Shader& _shader) {
        if (m_record) _shader.setUniform("u_time", m_record_head);
        else _shader.setUniform("u_time", float(getTime()));
    }, []() { return toString(getTime()); } );

    uniforms.functions["u_delta"] = UniformFunction("float", 
    [this](Shader& _shader) {
        if (m_record) _shader.setUniform("u_delta", float(FRAME_DELTA));
        else _shader.setUniform("u_delta", float(getDelta()));
    },
    []() { return toString(getDelta()); });

    uniforms.functions["u_date"] = UniformFunction("vec4", [](Shader& _shader) {
        _shader.setUniform("u_date", getDate());
    },
    []() { return toString(getDate(), ','); });

    // MOUSE
    uniforms.functions["u_mouse"] = UniformFunction("vec2", [](Shader& _shader) {
        _shader.setUniform("u_mouse", getMouseX(), getMouseY());
    },
    []() { return toString(getMouseX()) + "," + toString(getMouseY()); } );

    // VIEWPORT
    uniforms.functions["u_resolution"]= UniformFunction("vec2", [](Shader& _shader) {
        _shader.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
    },
    []() { return toString(getWindowWidth()) + "," + toString(getWindowHeight()); });

    // SCENE
    uniforms.functions["u_scene"] = UniformFunction("sampler2D", [this](Shader& _shader) {
        if (m_postprocessing_enabled) {
            _shader.setUniformTexture("u_scene", &m_scene_fbo, _shader.textureIndex++ );
        }
    });

    uniforms.functions["u_sceneDepth"] = UniformFunction("sampler2D", [this](Shader& _shader) {
        if (m_postprocessing_enabled && m_scene_fbo.getTextureId()) {
            _shader.setUniformDepthTexture("u_sceneDepth", &m_scene_fbo, _shader.textureIndex++ );
        }
    });

    uniforms.functions["u_view2d"] = UniformFunction("mat3", [this](Shader& _shader) {
        _shader.setUniform("u_view2d", m_view2d);
    });

    uniforms.functions["u_modelViewProjectionMatrix"] = UniformFunction("mat4");
}

// ------------------------------------------------------------------------- SET

void Sandbox::setup( WatchFileList &_files, CommandList &_commands ) {

    // Add Sandbox Commands
    //
    _commands.push_back(Command("defines", [&](const std::string& _line){ 
        if (_line == "defines") {
            for (unsigned int i = 0; i < defines.size(); i++) {
                std::cout << defines[i] << std::endl;
            }
            return true;
        }
        return false;
    },
    "defines                return a list of active defines", false));

    _commands.push_back(Command("uniforms", [&](const std::string& _line){ 
        uniforms.print(_line == "uniforms,all");

        // TODO
        //      - Cubemap

        return true;
    },
    "uniforms[,all|active]  return a list of all uniforms and their values or just the one active (default).", false));

    _commands.push_back(Command("textures", [&](const std::string& _line){ 
        if (_line == "textures") {
            uniforms.printTextures();
            return true;
        }
        return false;
    },
    "textures               return a list of textures as their uniform name and path.", false));

    _commands.push_back(Command("buffers", [&](const std::string& _line){ 
        if (_line == "buffers") {
            uniforms.printBuffers();
            return true;
        }
        return false;
    },
    "buffers                return a list of buffers as their uniform name.", false));

    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    // Init Scene elements
    m_billboard_vbo = rect(0.0,0.0,1.0,1.0).getVbo();

    //  Load Geometry
    //
    if (geom_index != -1) {
        m_scene.setup(_commands, uniforms);
        m_scene.loadModel( _files[geom_index].path, defines );
        m_vbo = m_scene.getModel().getVbo();
    }
    else {
        m_vbo = m_billboard_vbo;
    }

    //  Build shader
    //
    if (frag_index != -1) {
        m_frag_source = "";
        m_frag_dependencies.clear();

        if ( !loadFromPath(_files[frag_index].path, &m_frag_source, include_folders, &m_frag_dependencies) ) {
            return;
        }
    }
    else {
        m_frag_source = m_vbo->getVertexLayout()->getDefaultFragShader();
    }

    if (vert_index != -1) {
        m_vert_source = "";
        m_vert_dependencies.clear();

        loadFromPath(_files[vert_index].path, &m_vert_source, include_folders, &m_vert_dependencies);
    }
    else {
        m_vert_source = m_vbo->getVertexLayout()->getDefaultVertShader();
    }

    m_shader.load(m_frag_source, m_vert_source, defines, verbose);
    _updateDependencies( _files );

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    reload();

    // TODO:
    //      - this seams to solve the problem of buffers not properly initialize
    //      - digg deeper
    //
    uniforms.buffers.clear();
    _updateBuffers();

    flagChange();
}

void Sandbox::addDefine(const std::string &_define) {
    defines.push_back(_define);
}

void Sandbox::delDefine(const std::string &_define) {
    for (int i = (int)defines.size() - 1; i >= 0 ; i--) {
        if ( defines[i] == _define ) {
            defines.erase(defines.begin() + i);
        }
    }
}

// ------------------------------------------------------------------------- GET

bool Sandbox::isReady() {
    return m_frame > 0;
}

bool Sandbox::haveChange() { 
    return  m_change || m_record ||
            uniforms.functions["u_time"].present || 
            uniforms.functions["u_delta"].present ||
            uniforms.functions["u_date"].present ||
            m_scene.haveChange(); 
}

void Sandbox::unfalgChange() {
    m_change = false;
    m_scene.unfalgChange();
}

std::string Sandbox::getSource(ShaderType _type) const {
    if (_type == FRAGMENT) return m_frag_source;
    else return m_vert_source;
}

int Sandbox::getRecordedPorcentage() {
    return ((m_record_head - m_record_start) / (m_record_end - m_record_start)) * 100 ;
}

// ------------------------------------------------------------------------- RELOAD SHADER

bool Sandbox::reload() {
    // Reload the shader
    m_shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
    bool success = m_shader.load(m_frag_source, m_vert_source, defines, verbose);

    // If it doesn't work put a default one
    if (!success) {
        std::string default_vert;
        std::string default_frag;

        default_vert = m_vbo->getVertexLayout()->getDefaultVertShader(); 
        default_frag = m_vbo->getVertexLayout()->getDefaultFragShader();
        
        m_shader.load(default_frag, default_vert, defines, false);
    }
    // If it work check for present uniforms, buffers, background and postprocesing
    else {
        // Check active native uniforms
        uniforms.checkPresenceIn(m_vert_source, m_frag_source);

        // Flag all user defined uniforms as changed
        uniforms.flagChange();

         // Buffers
        m_buffers_total = count_buffers(m_frag_source);
        _updateBuffers();
        
        // Background pass
        m_background_enabled = check_for_background(getSource(FRAGMENT));
        if (m_background_enabled) {
            // Specific defines for this buffer
            std::vector<std::string> sub_defines = defines;
            sub_defines.push_back("BACKGROUND");
            m_background_shader.load(m_frag_source, billboard_vert, sub_defines, verbose);
        }

        // Postprocessing
        bool havePostprocessing = check_for_postprocessing(getSource(FRAGMENT));
        if (m_postprocessing_enabled != havePostprocessing) {
            m_scene_fbo.allocate(getWindowWidth(), getWindowHeight(), uniforms.functions["u_sceneDepth"].present ? COLOR_DEPTH_TEXTURES : COLOR_TEXTURE_DEPTH_BUFFER );
        }
        m_postprocessing_enabled = havePostprocessing;
        if (m_postprocessing_enabled) {
            // Specific defines for this buffer
            std::vector<std::string> sub_defines = defines;
            sub_defines.push_back("POSTPROCESSING");
            m_postprocessing_shader.load(m_frag_source, billboard_vert, sub_defines, verbose);
        }
    }

    flagChange();

    return success;
}

// ------------------------------------------------------------------------- UPDATE
void Sandbox::_updateBuffers() {
    if ( m_buffers_total != int(uniforms.buffers.size()) ) {
        uniforms.buffers.clear();
        m_buffers_shaders.clear();

        for (int i = 0; i < m_buffers_total; i++) {
            // New FBO
            uniforms.buffers.push_back( Fbo() );
            uniforms.buffers[i].allocate(getWindowWidth(), getWindowHeight(), COLOR_TEXTURE);
            
            // Specific defines for this buffer
            std::vector<std::string> sub_defines = defines;
            sub_defines.push_back("BUFFER_" + toString(i));

            // New SHADER
            m_buffers_shaders.push_back( Shader() );
            m_buffers_shaders[i].load(m_frag_source, billboard_vert, sub_defines, verbose);
        }
    }
    else {
        for (unsigned int i = 0; i < m_buffers_shaders.size(); i++) {
            // Specific defines for this buffer
            std::vector<std::string> sub_defines = defines;
            sub_defines.push_back("BUFFER_" + toString(i));

            // Reload shader code
            m_buffers_shaders[i].load(m_frag_source, billboard_vert, sub_defines, verbose);
        }
    }
}

void Sandbox::_updateDependencies(WatchFileList &_files) {
    FileList new_dependencies = mergeList(m_frag_dependencies, m_vert_dependencies);

    // remove old dependencies
    for (int i = _files.size() - 1; i >= 0; i--) {
        if (_files[i].type == GLSL_DEPENDENCY) {
            _files.erase( _files.begin() + i);
        }
    }

    // Add new dependencies
    struct stat st;
    for (unsigned int i = 0; i < new_dependencies.size(); i++) {
        WatchFile file;
        file.type = GLSL_DEPENDENCY;
        file.path = new_dependencies[i];
        stat( file.path.c_str(), &st );
        file.lastChange = st.st_mtime;
        _files.push_back(file);
    }
}


// ------------------------------------------------------------------------- DRAW
void Sandbox::_renderBuffers() {
    glDisable(GL_BLEND);

    for (unsigned int i = 0; i < uniforms.buffers.size(); i++) {
        uniforms.buffers[i].bind();
        m_buffers_shaders[i].use();

        // Update uniforms and textures
        uniforms.feedTo( m_buffers_shaders[i] );

        // Pass textures for the other buffers
        for (unsigned int j = 0; j < uniforms.buffers.size(); j++) {
            if (i != j) {
                m_buffers_shaders[i].setUniformTexture("u_buffer" + toString(j), &uniforms.buffers[j] );
            }
        }

        m_billboard_vbo->draw( &m_buffers_shaders[i] );

        uniforms.buffers[i].unbind();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Sandbox::_renderBackground() {
    if (m_background_enabled) {
        m_background_shader.use();

        // Update Uniforms and textures
        uniforms.feedTo( m_background_shader );

        // Pass all buffers
        for (unsigned int i = 0; i < uniforms.buffers.size(); i++) {
            m_background_shader.setUniformTexture("u_buffer" + toString(i), &uniforms.buffers[i]);
        }

        m_billboard_vbo->draw( &m_background_shader );
    }
    // CUBEMAP
    else if (geom_index != -1) {
        m_scene.renderCubeMap();
    }
}

void Sandbox::draw() {
    // RENDER SHADOW MAP
    // -----------------------------------------------
    if (geom_index != -1) {
        if (uniforms.functions["u_ligthShadowMap"].present)
            m_scene.renderShadowMap(m_shader, uniforms);
    }
    
    // BUFFERS
    // -----------------------------------------------
    if (uniforms.buffers.size() > 0) {
        _renderBuffers();
    }
    

    // MAIN SCENE
    // ----------------------------------------------- < main scene start
    if (m_postprocessing_enabled) {
        m_scene_fbo.bind();
    }
    else if (screenshotFile != "" || m_record) {
        if (!m_record_fbo.isAllocated()) {
            m_record_fbo.allocate(getWindowWidth(), getWindowHeight(), COLOR_TEXTURE_DEPTH_BUFFER);
        }
        m_record_fbo.bind();
    }

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // RENDER BACKGROUND
    _renderBackground();

    if (geom_index == -1) {
        // Load main shader
        m_shader.use();

        // Update Uniforms and textures variables
        uniforms.feedTo( m_shader );

        // Pass special uniforms
        m_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.));
        m_vbo->draw( &m_shader );
    }
    else {
        m_scene.render(m_shader, uniforms);
        if (debug)
            m_scene.renderDebug();
    }
    
    // ----------------------------------------------- < main scene end

    // POST PROCESSING
    if (m_postprocessing_enabled) {
        m_scene_fbo.unbind();

        if (screenshotFile != "" || m_record) {
             if (!m_record_fbo.isAllocated()) {
                m_record_fbo.allocate(getWindowWidth(), getWindowHeight(), COLOR_TEXTURE_DEPTH_BUFFER);
            }
            m_record_fbo.bind();
        }
    
        m_postprocessing_shader.use();

        // Update uniforms and textures
        uniforms.feedTo( m_postprocessing_shader );

        // Pass textures of buffers
        for (unsigned int i = 0; i < uniforms.buffers.size(); i++) {
            m_postprocessing_shader.setUniformTexture("u_buffer" + toString(i), &uniforms.buffers[i]);
        }

        m_billboard_vbo->draw( &m_postprocessing_shader );
    }
    
    if (screenshotFile != "" || m_record) {
        m_record_fbo.unbind();
    }
}


void Sandbox::drawUI() {
    if (debug) {        
        glDisable(GL_DEPTH_TEST);

        // DEBUG BUFFERS
        int nTotal = uniforms.buffers.size();
        if (m_postprocessing_enabled) {
            nTotal += uniforms.functions["u_scene"].present;
            nTotal += uniforms.functions["u_sceneDepth"].present;
        }
        nTotal += uniforms.functions["u_ligthShadowMap"].present;
        if (nTotal > 0) {
            float w = (float)(getWindowWidth());
            float h = (float)(getWindowHeight());
            float scale = fmin(1.0f / (float)(nTotal), 0.25) * 0.5;
            float xStep = w * scale;
            float yStep = h * scale;
            float margin = 5.0;
            float xOffset = xStep + margin;
            float yOffset = h - yStep - margin;

            if (!m_billboard_shader.isLoaded())
                m_billboard_shader.load(dynamic_billboard_frag, dynamic_billboard_vert, defines, false);

            m_billboard_shader.use();
            uniforms.feedTo(m_billboard_shader);

            for (unsigned int i = 0; i < uniforms.buffers.size(); i++) {
                m_billboard_shader.setUniform("u_depth", float(0.0));
                m_billboard_shader.setUniform("u_scale", xStep, yStep);
                m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                m_billboard_shader.setUniformTexture("u_tex0", &uniforms.buffers[i]);
                m_billboard_vbo->draw(&m_billboard_shader);
                yOffset -= yStep * 2.0 + margin;
            }

            if (m_postprocessing_enabled) {
                if (uniforms.functions["u_scene"].present) {
                    m_billboard_shader.setUniform("u_depth", float(0.0));
                    m_billboard_shader.setUniform("u_scale", xStep, yStep);
                    m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                    m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                    m_billboard_shader.setUniformTexture("u_tex0", &m_scene_fbo);
                    m_billboard_vbo->draw(&m_billboard_shader);
                    yOffset -= yStep * 2.0 + margin;
                }

                if (uniforms.functions["u_sceneDepth"].present) {
                    m_billboard_shader.setUniform("u_scale", xStep, yStep);
                    m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                    m_billboard_shader.setUniform("u_depth", float(1.0));
                    // m_billboard_shader.setUniform("u_cameraNearClip", m_scene.getCamera().getNearClip());
                    // m_billboard_shader.setUniform("u_cameraFarClip", m_scene.getCamera().getFarClip());
                    // m_billboard_shader.setUniform("u_cameraDistance", m_scene.getCamera().getDistance());
                    m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                    m_billboard_shader.setUniformDepthTexture("u_tex0", &m_scene_fbo);
                    m_billboard_vbo->draw(&m_billboard_shader);
                    yOffset -= yStep * 2.0 + margin;
                }
            }


            if (uniforms.functions["u_ligthShadowMap"].present && m_scene.getLightMap().getDepthTextureId() ) {
                m_billboard_shader.setUniform("u_scale", xStep, yStep);
                m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                m_billboard_shader.setUniform("u_depth", float(0.0));
                // m_billboard_shader.setUniform("u_cameraNearClip", m_scene.getCamera().getNearClip());
                // m_billboard_shader.setUniform("u_cameraFarClip", m_scene.getCamera().getFarClip());
                // m_billboard_shader.setUniform("u_cameraDistance", m_scene.getCamera().getDistance());
                m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                m_billboard_shader.setUniformDepthTexture("u_tex0", &m_scene.getLightMap());
                m_billboard_vbo->draw(&m_billboard_shader);
                yOffset -= yStep * 2.0 + margin;
            }
        }
    }

    if (cursor) {
        if (m_cross_vbo == nullptr) 
            m_cross_vbo = cross(glm::vec3(0.0, 0.0, 0.0), 10.).getVbo();

        if (!m_wireframe2D_shader.isLoaded())
            m_wireframe2D_shader.load(wireframe2D_frag, wireframe2D_vert, defines, false);

        glLineWidth(2.0f);
        m_wireframe2D_shader.use();
        m_wireframe2D_shader.setUniform("u_color", glm::vec4(1.0));
        m_wireframe2D_shader.setUniform("u_scale", 1.0f, 1.0f);
        m_wireframe2D_shader.setUniform("u_translate", getMouseX(), getMouseY());
        m_wireframe2D_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
        m_cross_vbo->draw(&m_wireframe2D_shader);
        glLineWidth(1.0f);
    }
}

void Sandbox::drawDone() {

    // RECORD
    if (m_record) {
        onScreenshot(toString(m_record_counter, 0, 5, '0') + ".png");

        m_record_head += FRAME_DELTA;
        m_record_counter++;

        if (m_record_head >= m_record_end) {
            m_record = false;
        }
    }
    // SCREENSHOT 
    else if (screenshotFile != "") {
        onScreenshot(screenshotFile);
        screenshotFile = "";
    }

    m_frame++;
    unfalgChange();
}

// ------------------------------------------------------------------------- ACTIONS

void Sandbox::clear() {
    uniforms.clear();

    if (geom_index != -1)
        m_scene.clear();

    if (m_billboard_vbo)
        delete m_billboard_vbo;

    if (m_cross_vbo)
        delete m_cross_vbo;
}

void Sandbox::record(float _start, float _end) {
    m_record_start = _start;
    m_record_head = _start;
    m_record_end = _end;
    m_record_counter = 0;
    m_record = true;
}

void Sandbox::printDependencies(ShaderType _type) const {
    if (_type == FRAGMENT) {
        for (unsigned int i = 0; i < m_frag_dependencies.size(); i++) {
            std::cout << m_frag_dependencies[i] << std::endl;
        }
    }
    else {
        for (unsigned int i = 0; i < m_vert_dependencies.size(); i++) {
            std::cout << m_vert_dependencies[i] << std::endl;
        }
    }
}

// ------------------------------------------------------------------------- EVENTS

void Sandbox::onFileChange(WatchFileList &_files, int index) {
    FileType type = _files[index].type;
    std::string filename = _files[index].path;

    if (type == GLSL_DEPENDENCY) {
        if (std::find(m_frag_dependencies.begin(), m_frag_dependencies.end(), filename) != m_frag_dependencies.end()) {
            type = FRAG_SHADER;
            filename = _files[frag_index].path;
        }
        else if(std::find(m_vert_dependencies.begin(), m_vert_dependencies.end(), filename) != m_vert_dependencies.end()) {
            type = VERT_SHADER;
            filename = _files[vert_index].path;
        }
    }

    if (type == FRAG_SHADER) {
        m_frag_source = "";
        m_frag_dependencies.clear();

        if (loadFromPath(filename, &m_frag_source, include_folders, &m_frag_dependencies)) {
            reload();
            _updateDependencies(_files);
        }
    }
    else if (type == VERT_SHADER) {
        m_vert_source = "";
        m_vert_dependencies.clear();

        if (loadFromPath(filename, &m_vert_source, include_folders, &m_vert_dependencies)) {
            reload();
            _updateDependencies(_files);
        }
    }
    else if (type == GEOMETRY) {
        // TODO
    }
    else if (type == IMAGE) {
        for (TextureList::iterator it = uniforms.textures.begin(); it!=uniforms.textures.end(); ++it) {
            if (filename == it->second->getFilePath()) {
                it->second->load(filename, _files[index].vFlip);
                break;
            }
        }
    }
    else if (type == CUBEMAP) {
        if (m_scene.getCubeMap()) {
            m_scene.getCubeMap()->load(filename, _files[index].vFlip);
        }
    }

    m_change = true;
}

void Sandbox::onScroll(float _yoffset) {
    // Vertical scroll button zooms u_view2d and view3d.
    /* zoomfactor 2^(1/4): 4 scroll wheel clicks to double in size. */
    constexpr float zoomfactor = 1.1892;
    if (_yoffset != 0) {
        float z = pow(zoomfactor, _yoffset);

        // zoom view2d
        glm::vec2 zoom = glm::vec2(z,z);
        glm::vec2 origin = {getWindowWidth()/2, getWindowHeight()/2};
        m_view2d = glm::translate(m_view2d, origin);
        m_view2d = glm::scale(m_view2d, zoom);
        m_view2d = glm::translate(m_view2d, -origin);
        
        flagChange();
    }
}

void Sandbox::onMouseDrag(float _x, float _y, int _button) {
    if (geom_index != -1)
        m_scene.onMouseDrag(_x, _y, _button);

    if (_button == 1) {
        // Left-button drag is used to pan u_view2d.
        m_view2d = glm::translate(m_view2d, -getMouseVelocity());
        flagChange();
    } 
}

void Sandbox::onViewportResize(int _newWidth, int _newHeight) {
    if (geom_index != -1)
        m_scene.onViewportResize(_newWidth, _newHeight);
    
    for (unsigned int i = 0; i < uniforms.buffers.size(); i++) {
        uniforms.buffers[i].allocate(_newWidth, _newHeight, COLOR_TEXTURE);
    }

    if (m_postprocessing_enabled) {
        m_scene_fbo.allocate(_newWidth, _newHeight, uniforms.functions["u_sceneDepth"].present ? COLOR_DEPTH_TEXTURES : COLOR_TEXTURE_DEPTH_BUFFER);
    }

    if (m_record_fbo.isAllocated()) {
        m_scene_fbo.allocate(_newWidth, _newHeight, COLOR_TEXTURE_DEPTH_BUFFER);
    }

    flagChange();
}

void Sandbox::onScreenshot(std::string _file) {
    if (_file != "" && isGL()) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_record_fbo.getId());

        unsigned char* pixels = new unsigned char[getWindowWidth() * getWindowHeight()*4];
        glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        savePixels(_file, pixels, getWindowWidth(), getWindowHeight());
        delete[] pixels;

        if (!m_record) {
            std::cout << "// Screenshot saved to " << _file << std::endl;
            std::cout << "// > ";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

