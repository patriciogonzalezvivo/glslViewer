#include "sandbox.h"

#include "window.h"

#include "tools/text.h"
#include "tools/geom.h"
#include "types/shapes.h"

#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/rotate_vector.hpp"

#define FRAME_DELTA 0.03333333333

Sandbox::Sandbox(): 
    iFrag(-1), iVert(-1), iGeom(-1),
    verbose(false), vFlip(true),
    m_fragPath(nullptr), m_vertPath(nullptr),
    m_lat(180.0), m_lon(0.0),
    m_background_enabled(false),
    m_cubemap_vbo(nullptr), m_cubemap(false) {

    m_view2d = glm::mat3(1.);

    // These are the 'view3d' uniforms.
    // Note: the up3d vector must be orthogonal to (eye3d - centre3d),
    // or rotation doesn't work correctly.
    m_centre3d = glm::vec3(0.,0.,0.);

    // The following initial value for 'eye3d' is derived by starting with [0,0,6],
    // then rotating 30 degrees around the X and Y axes.
    m_eye3d = glm::vec3(2.598076,3.0,4.5);

    // The initial value for up3d is derived by starting with [0,1,0], then
    // applying the same rotations as above, so that up3d is orthogonal to eye3d.
    m_up3d = glm::vec3(-0.25,0.866025,-0.433013);

    // model matrix
    m_vbo_matrix = glm::mat4(1.);

    // Adding default deines
    defines.push_back("GLSLVIEWER 1");
    // Define PLATFORM
    #ifdef PLATFORM_OSX
    defines.push_back("PLATFORM_OSX");
    #endif

    #ifdef PLATFORM_LINUX
    defines.push_back("PLATFORM_LINUX");
    #endif

    #ifdef PLATFORM_RPI
    defines.push_back("PLATFORM_RPI");
    #endif
}

void Sandbox::setup( WatchFileList &_files ) {
    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    //  Load Geometry
    //
    if (iGeom == -1) {
        m_vbo = rect(0.0,0.0,1.0,1.0).getVbo();
    }
    else {
        Mesh model;
        model.load( _files[iGeom].path );
        m_vbo = model.getVbo();
        m_centre3d = getCentroid( model.getVertices() );
        m_vbo_matrix = glm::translate( -m_centre3d );

        // Init camera
        //
        m_cam.setViewport(getWindowWidth(), getWindowHeight());
        m_cam.setPosition(glm::vec3(0.0,0.0,-3.));
        
        float size = getSize( model.getVertices() );
        m_cam.setDistance( size * 2.0 );
    }

    //  Build shader
    //
    if (iFrag != -1) {
        m_fragPath = &_files[iFrag].path;
        m_fragSource = "";
        if ( !loadFromPath(*m_fragPath, &m_fragSource, include_folders) ) {
            return;
        }
    }
    else {
        m_fragSource = m_vbo->getVertexLayout()->getDefaultFragShader();
    }

    if (iVert != -1) {
        m_vertPath = &_files[iVert].path;
        m_vertSource = "";
        loadFromPath(*m_vertPath, &m_vertSource, include_folders);
    }
    else {
        m_vertSource = m_vbo->getVertexLayout()->getDefaultVertShader();
    }

    m_shader.load(m_fragSource, m_vertSource, defines, verbose);

    // Init buffers
    m_billboard_vbo = rect(0.0,0.0,1.0,1.0).getVbo();
    m_billboard_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
attribute vec4 a_position;\n\
attribute vec2 a_texcoord;\n\
varying vec2 v_texcoord;\n\
\n\
void main(void) {\n\
    gl_Position = a_position;\n\
    v_texcoord = a_texcoord;\n\
}";

    _updateBackground();
    _updateBuffers();

    // CUBEMAP
    if (m_cubemap) {
        m_cubemap_vbo = cube(1.0f).getVbo();

        std::string vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform mat4    u_modelViewProjectionMatrix;\n\
attribute vec4  a_position;\n\
varying vec4    v_position;\n\
\n\
void main(void) {\n\
    v_position = a_position;\n\
    gl_Position = u_modelViewProjectionMatrix * vec4(v_position.xyz, 1.0);\n\
}";

        std::string frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform samplerCube " + m_cubemap_name + ";\n\
\n\
varying vec4    v_position;\n\
\n\
void main(void) {\n\
    gl_FragColor = textureCube(" + m_cubemap_name + ", v_position.xyz);\n\
}";

        m_cubemap_shader.load(frag, vert, defines, false);
    }

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // WARNING HACK!!!!
    // 
    //  Note: in order to make the multibuffer to work I had to create it and destroy it
    //
    //  TODO: 
    //          - FIX THIS
    m_buffers.clear();
    m_buffers_shaders.clear();
}

std::string Sandbox::get3DView() const {
    return  "up,"      + toString(m_up3d.x)    + "," + toString(m_up3d.y)      + "," + toString(m_up3d.z) + "\n" +
            "eye,"     + toString(m_eye3d.x)   + "," + toString(m_eye3d.y)     + "," + toString(m_eye3d.z) + "\n" +
            "centre,"  + toString(m_centre3d.x) + "," + toString(m_centre3d.y) + "," + toString(m_centre3d.z) + "\n";            
}

void Sandbox::_updateBackground() {
    m_background_enabled = m_shader.isBackground();

    if (m_background_enabled) {
        // Specific defines for this buffer
        std::vector<std::string> sub_defines = defines;
        sub_defines.push_back("BACKGROUND");
        m_background_shader.load(m_fragSource, m_billboard_vert, sub_defines, verbose);
    }
}

void Sandbox::_updateBuffers() {
    // Update BUFFERS
    int totalBuffers = m_shader.getTotalBuffers();
    // std::cout << "TOTAL BUFFERS: " << totalBuffers << std::endl; 
    
    if ( totalBuffers != int(m_buffers.size()) ) {
        m_buffers.clear();
        m_buffers_shaders.clear();

        for (int i = 0; i < totalBuffers; i++) {
            // New FBO
            m_buffers.push_back( Fbo() );
            m_buffers[i].allocate(getWindowWidth(), getWindowHeight(), false);
            
            // Specific defines for this buffer
            std::vector<std::string> sub_defines = defines;
            sub_defines.push_back("BUFFER_" + toString(i));

            // New SHADER
            m_buffers_shaders.push_back( Shader() );
            m_buffers_shaders[i].load(m_fragSource, m_billboard_vert, sub_defines, verbose);
        }
    }
    else {
        for (unsigned int i = 0; i < m_buffers_shaders.size(); i++) {
            // Specific defines for this buffer
            std::vector<std::string> sub_defines = defines;
            sub_defines.push_back("BUFFER_" + toString(i));

            // Reload shader code
            m_buffers_shaders[i].load(m_fragSource, m_billboard_vert, sub_defines, verbose);
        }
    }
 }

void Sandbox::_updateUniforms( Shader &_shader ) {
    // Pass uniforms
    _shader.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
    if (_shader.needTime()) {
        if (m_record) {
            _shader.setUniform("u_time", m_record_head);
        }
        else {
            _shader.setUniform("u_time", float(getTime()));
        }
    }
    if (_shader.needDelta()) {
        if (m_record) {
            _shader.setUniform("u_delta", float(FRAME_DELTA));
        }    
        else {
            _shader.setUniform("u_delta", float(getDelta()));
        }
    }
    if (_shader.needDate()) {
        _shader.setUniform("u_date", getDate());
    }
    
    if (_shader.needMouse4()) {
        _shader.setUniform("u_mouse", getMouse4());
    }
    else if (_shader.needMouse()) {
        _shader.setUniform("u_mouse", getMouseX(), getMouseY());
    }
    
    if (_shader.needView2d()) {
        _shader.setUniform("u_view2d", m_view2d);
    }
    if (_shader.needView3d()) {
        _shader.setUniform("m_eye3d", m_eye3d);
        _shader.setUniform("m_centre3d", m_centre3d);
        _shader.setUniform("m_up3d", m_up3d);
    }

    for (UniformList::iterator it=uniforms.begin(); it!=uniforms.end(); ++it) {
        if (it->second.bInt) {
            _shader.setUniform(it->first, int(it->second.value[0]));
        }
        else {
            _shader.setUniform(it->first, it->second.value, it->second.size);
        }
    }
}

void Sandbox::_updateTextures( Shader &_shader, int &_textureIndex ) {
    // Pass Textures Uniforms
    for (TextureList::iterator it = textures.begin(); it!=textures.end(); ++it) {
        _shader.setUniformTexture(it->first, it->second, _textureIndex );
        _shader.setUniform(it->first+"Resolution", it->second->getWidth(), it->second->getHeight());
        _textureIndex++;
    }
}

void Sandbox::draw() {
    // Need to update Buffers
    if ( m_shader.getTotalBuffers() != int(m_buffers.size()) ) {
        _updateBuffers();
    }

    // Update buffers
    int textureIndex = 0;
    for (unsigned int i = 0; i < m_buffers.size(); i++) {
        m_buffers[i].bind();
        m_buffers_shaders[i].use();

        // Update uniforms variables
        _updateUniforms( m_buffers_shaders[i] );

        // Update textures
        _updateTextures( m_buffers_shaders[i], textureIndex );

        // Pass textures for the other buffers
        for (unsigned int j = 0; j < m_buffers.size(); j++) {
            if (i != j) {
                m_buffers_shaders[i].setUniformTexture("u_buffer" + toString(j), &m_buffers[j], textureIndex );
                textureIndex++;
            }
        }

        m_billboard_vbo->draw( &m_buffers_shaders[i] );

        m_buffers[i].unbind();
    }

    // need to update Background
    if ( m_shader.isBackground() != m_background_enabled) {
        _updateBackground();
    }

    if (m_background_enabled) {
        m_background_shader.use();

        // Update Uniforms variables
        _updateUniforms( m_background_shader );

        // Update textures
        _updateTextures( m_background_shader, textureIndex );

        // Pass all buffers
        for (unsigned int i = 0; i < m_buffers.size(); i++) {
            m_background_shader.setUniformTexture("u_buffer" + toString(i), &m_buffers[i], textureIndex );
            textureIndex++;
        }

        m_billboard_vbo->draw( &m_background_shader );
    }
    // Cubemap
    else if (iGeom != -1 && m_cubemap) {
        m_cubemap_shader.use();

        // _updateUniforms( m_cubemap_shader );
        m_cubemap_shader.setUniform("u_modelViewProjectionMatrix", m_cam.getProjectionMatrix() * glm::toMat4(m_cam.getOrientationQuat()) );
        m_cubemap_shader.setUniformTextureCube( m_cubemap_name, ((TextureCube*)textures[m_cubemap_name]), textureIndex++ );

        // glDisable(GL_DEPTH_TEST);
        m_cubemap_vbo->draw( &m_cubemap_shader );
    }

    if (iGeom != -1) {
        glEnable(GL_DEPTH_TEST);
    }

    // MAIN SHADER
    m_shader.use();

    // Update Uniforms variables
    _updateUniforms( m_shader );

    // Update textures
    _updateTextures( m_shader, textureIndex );

    if (m_cubemap) {
        m_shader.setUniformTextureCube( m_cubemap_name, ((TextureCube*)textures[m_cubemap_name]), textureIndex++ );
    }

    // Pass all buffers
    for (unsigned int i = 0; i < m_buffers.size(); i++) {
        m_shader.setUniformTexture("u_buffer" + toString(i), &m_buffers[i], textureIndex );
        textureIndex++;
    }

    glm::mat4 mvp = glm::mat4(1.);
    // Pass special uniforms
    if (iGeom != -1) {
        m_shader.setUniform("u_eye", -m_cam.getPosition());
        m_shader.setUniform("u_normalMatrix", m_cam.getNormalMatrix());

        m_shader.setUniform("u_modelMatrix", m_vbo_matrix);
        m_shader.setUniform("u_viewMatrix", m_cam.getViewMatrix());
        m_shader.setUniform("u_projectionMatrix", m_cam.getProjectionMatrix());

        mvp = m_cam.getProjectionViewMatrix() * m_vbo_matrix;
    }
    m_shader.setUniform("u_modelViewProjectionMatrix", mvp);

    m_vbo->draw( &m_shader );

    if (iGeom != -1) {
        glDisable(GL_DEPTH_TEST);
    }

    if (m_record) {
        onScreenshot(toString(m_record_counter,0,3,'0') + ".png");
        m_record_head += FRAME_DELTA;
        m_record_counter++;

        if (m_record_head >= m_record_end) {
            m_record = false;
        }
    } else if (screenshotFile != "") {
        onScreenshot(screenshotFile);
        screenshotFile = "";
    }
}

int Sandbox::getRecordedPorcentage() {
    return ((m_record_head - m_record_start) / (m_record_end - m_record_start)) * 100 ;
}

bool Sandbox::reload() {
    m_shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
    bool success = m_shader.load(m_fragSource, m_vertSource, defines, verbose);

    if (!success) {
        std::string default_vert = m_vbo->getVertexLayout()->getDefaultVertShader(); 
        std::string default_frag = m_vbo->getVertexLayout()->getDefaultFragShader(); 
        m_shader.load(default_frag, default_vert, defines, false);
    }

    return success;
}

void Sandbox::onFileChange(WatchFileList &_files, int index) {
    std::string type = _files[index].type;
    std::string path = _files[index].path;

    if (type == "fragment") {
        m_fragSource = "";
        if (loadFromPath(path, &m_fragSource, include_folders)) {
            reload();
            _updateBackground();
            _updateBuffers();
        }
    }
    else if (type == "vertex") {
        m_vertSource = "";
        if (loadFromPath(path, &m_vertSource, include_folders)) {
            reload();
        }
    }
    else if (type == "geometry") {
        // TODO
    }
    else if (type == "image") {
        for (TextureList::iterator it = textures.begin(); it!=textures.end(); ++it) {
            if (path == it->second->getFilePath()) {
                it->second->load(path, _files[index].vFlip);
                break;
            }
        }
    }
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

        // zoom view3d
        m_eye3d = m_centre3d + (m_eye3d - m_centre3d)*z;
    }
}

void Sandbox::onMouseDrag(float _x, float _y, int _button) {
    if (_button == 1) {

        if (iGeom != -1) {
            // Left-button drag is used to rotate geometry.
            float dist = m_cam.getDistance();
            m_lat -= getMouseVelX();
            m_lon -= getMouseVelY()*0.5;
            m_cam.orbit(m_lat ,m_lon, dist);
            m_cam.lookAt(glm::vec3(0.0));
        }

        // Left-button drag is used to pan u_view2d.
        m_view2d = glm::translate(m_view2d, -getMouseVelocity());

        // Left-button drag is used to rotate eye3d around centre3d.
        // One complete drag across the screen width equals 360 degrees.
        constexpr double tau = 6.283185307179586;
        m_eye3d -= m_centre3d;
        m_up3d -= m_centre3d;
        // Rotate about vertical axis, defined by the 'up' vector.
        float xangle = (getMouseVelX() / getWindowWidth()) * tau;
        m_eye3d = glm::rotate(m_eye3d, -xangle, m_up3d);
        // Rotate about horizontal axis, which is perpendicular to
        // the (centre3d,eye3d,up3d) plane.
        float yangle = (getMouseVelY() / getWindowHeight()) * tau;
        glm::vec3 haxis = glm::cross(m_eye3d - m_centre3d, m_up3d);
        m_eye3d = glm::rotate(m_eye3d, -yangle, haxis);
        m_up3d = glm::rotate(m_up3d, -yangle, haxis);
        //
        m_eye3d += m_centre3d;
        m_up3d += m_centre3d;
    } 
    else {

        if (iGeom != -1) {
            // Right-button drag is used to zoom geometry.
            float dist = m_cam.getDistance();
            dist += (-.008f * getMouseVelY());
            if (dist > 0.0f) {
                m_cam.setDistance( dist );
            }
        }

        // TODO: rotate view2d.

        // pan view3d.
        float dist3d = glm::length(m_eye3d - m_centre3d);
        glm::vec3 voff = glm::normalize(m_up3d) * (getMouseVelY()/getWindowHeight()) * dist3d;
        m_centre3d -= voff;
        m_eye3d -= voff;
        glm::vec3 haxis = glm::cross(m_eye3d - m_centre3d, m_up3d);
        glm::vec3 hoff = glm::normalize(haxis) * (getMouseVelX()/getWindowWidth()) * dist3d;
        m_centre3d += hoff;
        m_eye3d += hoff;
    }
}

void Sandbox::onViewportResize(int _newWidth, int _newHeight) {
    if (iGeom != -1) {
        m_cam.setViewport(_newWidth, _newHeight);
    }
    for (unsigned int i = 0; i < m_buffers.size(); i++) {
        m_buffers[i].allocate(_newWidth, _newHeight, false);
    }
}

void Sandbox::onScreenshot(std::string _file) {
    if (_file != "" && isGL()) {
        unsigned char* pixels = new unsigned char[getWindowWidth()*getWindowHeight()*4];
        glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        Texture::savePixels(_file, pixels, getWindowWidth(), getWindowHeight());

        if (!m_record) {
            std::cout << "// Screenshot saved to " << _file << std::endl;
            std::cout << "// > ";
        }
    }
}

void Sandbox::addDefines(const std::string &_define) {
    defines.push_back(_define);
}

void Sandbox::delDefines(const std::string &_define) {
    for (int i = (int)defines.size() - 1; i >= 0 ; i--) {
        if ( defines[i] == _define ) {
            defines.erase(defines.begin() + i);
        }
    }
}

void Sandbox::setCubeMapName(const std::string &_name) { 
    m_cubemap_name = _name; 
    m_cubemap = true; 
};

void Sandbox::record(float _start, float _end) {
    m_record_start = _start;
    m_record_head = _start;
    m_record_end = _end;
    m_record_counter = 0;
    m_record = true;
}

void Sandbox::clean() {
    for (TextureList::iterator i = textures.begin(); i != textures.end(); ++i) {
        delete i->second;
        i->second = NULL;
    }
    textures.clear();

    delete m_vbo;

    if (m_billboard_vbo)
        delete m_billboard_vbo;

    if (m_cubemap)
        delete m_cubemap_vbo;
}