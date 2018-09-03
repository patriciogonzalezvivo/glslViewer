#include "sandbox.h"

#include <sys/stat.h>   // stat
#include <algorithm>    // std::find

#include "window.h"

#include "tools/text.h"
#include "tools/geom.h"
#include "tools/image.h"
#include "types/shapes.h"

#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "default_shaders.h"

#define FRAME_DELTA 0.03333333333

// ------------------------------------------------------------------------- CONTRUCTOR
Sandbox::Sandbox(): 
    frag_index(-1), vert_index(-1), geom_index(-1),
    verbose(false), cursor(false), debug(false),
    // Main Vert/Frag/Geom
    m_frag_source(""), m_vert_source(""),
    m_model_vbo(nullptr), // m_model_matrix(1.0),
    // Camera.
    m_mvp(1.0), m_view2d(1.0), m_lat(180.0), m_lon(0.0),
    // CubeMap
    m_cubemap_vbo(nullptr), m_cubemap(nullptr),
    // Background
    m_background_enabled(false),
    // Buffers
    m_buffers_total(0),
    // PostProcessing
    m_postprocessing_enabled(false),
    // Geometry helpers
    m_billboard_vbo(nullptr), m_cross_vbo(nullptr), m_bbox_vbo(nullptr),
    // Record
    m_record_start(0.0f), m_record_head(0.0f), m_record_end(0.0f), m_record_counter(0), m_record(false),
    // Scene
    m_culling(NONE), m_textureIndex(0), m_change(true), m_ready(false) {

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

    // TIME UNIFORMS
    //
    uniforms_functions["u_time"] = UniformFunction( "float", 
    [this](Shader& _shader) {
        if (m_record) _shader.setUniform("u_time", m_record_head);
        else _shader.setUniform("u_time", float(getTime()));
    }, []() { return toString(getTime()); } );

    uniforms_functions["u_delta"] = UniformFunction("float", 
    [this](Shader& _shader) {
        if (m_record) _shader.setUniform("u_delta", float(FRAME_DELTA));
        else _shader.setUniform("u_delta", float(getDelta()));
    },
    []() { return toString(getDelta()); });

    uniforms_functions["u_date"] = UniformFunction("vec4", [](Shader& _shader) {
        _shader.setUniform("u_date", getDate());
    },
    []() { return toString(getDate(), ','); });

    // MOUSE
    uniforms_functions["u_mouse"] = UniformFunction("vec2", [](Shader& _shader) {
        _shader.setUniform("u_mouse", getMouseX(), getMouseY());
    },
    []() { return toString(getMouseX()) + "," + toString(getMouseY()); } );

    // VIEWPORT
    uniforms_functions["u_resolution"]= UniformFunction("vec2", [](Shader& _shader) {
        _shader.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
    },
    []() { return toString(getWindowWidth()) + "," + toString(getWindowHeight()); });

    // SCENE
    uniforms_functions["u_scene"] = UniformFunction("sampler2D");
    uniforms_functions["u_scene_depth"] = UniformFunction("sampler2D");

    // LIGHT UNIFORMS
    //
    uniforms_functions["u_light"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_light", m_light.getPosition());
    },
    [this]() { return toString(m_light.getPosition(), ','); });

    uniforms_functions["u_lightColor"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_lightColor", m_light.color);
    },
    [this]() { return toString(m_light.color, ','); });

    // IBL UNIFORM
    uniforms_functions["u_cubeMap"] = UniformFunction("samplerCube", [this](Shader& _shader) {
        if (m_cubemap != nullptr) {
            m_shader.setUniformTextureCube( "u_cubeMap", (TextureCube*)m_cubemap, m_textureIndex++ );
        }
    });

    uniforms_functions["u_SH"] = UniformFunction("vec3", [this](Shader& _shader) {
        if (m_cubemap != nullptr) {
            m_shader.setUniform("u_SH", m_cubemap->SH, 9);
        }
    });

    uniforms_functions["u_iblLuminance"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_iblLuminance", 30000.0f * m_cam.getExposure());
    },
    [this]() { return toString(30000.0f * m_cam.getExposure()); });
    

    // CAMERA UNIFORMS
    //
    uniforms_functions["u_camera"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_camera", -m_cam.getPosition());
    },
    [this]() { return toString(-m_cam.getPosition(), ','); });

    uniforms_functions["u_cameraDistance"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraDistance", m_cam.getDistance());
    },
    [this]() { return toString(m_cam.getDistance()); });

    uniforms_functions["u_cameraNearClip"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraNearClip", m_cam.getNearClip());
    },
    [this]() { return toString(m_cam.getNearClip()); });

    uniforms_functions["u_cameraFarClip"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraFarClip", m_cam.getFarClip());
    },
    [this]() { return toString(m_cam.getFarClip()); });

    uniforms_functions["u_cameraEv100"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraEv100", m_cam.getEv100());
    },
    [this]() { return toString(m_cam.getEv100()); });

    uniforms_functions["u_cameraExposure"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraExposure", m_cam.getExposure());
    },
    [this]() { return toString(m_cam.getExposure()); });

    uniforms_functions["u_cameraAperture"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraAperture", m_cam.getAperture());
    },
    [this]() { return toString(m_cam.getAperture()); });

    uniforms_functions["u_cameraShutterSpeed"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraShutterSpeed", m_cam.getShutterSpeed());
    },
    [this]() { return toString(m_cam.getShutterSpeed()); });

    uniforms_functions["u_cameraSensitivity"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraSensitivity", m_cam.getSensitivity());
    },
    [this]() { return toString(m_cam.getSensitivity()); });
    
    // MATRIX UNIFORMS
    uniforms_functions["u_model"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_model", m_model_node.getPosition());
    },
    [this]() { return toString(m_model_node.getPosition(), ','); });

    uniforms_functions["u_view2d"] = UniformFunction("mat3", [this](Shader& _shader) {
        _shader.setUniform("u_view2d", m_view2d);
    });
    uniforms_functions["u_normalMatrix"] = UniformFunction("mat3", [this](Shader& _shader) {
        _shader.setUniform("u_normalMatrix", m_cam.getNormalMatrix());
    });
    uniforms_functions["u_modelMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_modelMatrix", m_model_node.getTransformMatrix() );
    });
    uniforms_functions["u_viewMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_viewMatrix", m_cam.getViewMatrix());
    });
    uniforms_functions["u_projectionMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_projectionMatrix", m_cam.getProjectionMatrix());
    });
    uniforms_functions["u_modelViewProjectionMatrix"] = UniformFunction("mat4");
}

// ------------------------------------------------------------------------- SET

void Sandbox::setup( WatchFileList &_files ) {
    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    // Init Scene elements
    m_cam.setViewport(getWindowWidth(), getWindowHeight());
    m_cam.setPosition(glm::vec3(0.0,0.0,-3.));
    m_light.setPosition(glm::vec3(0.0,100.0,100.0));
    m_billboard_vbo = rect(0.0,0.0,1.0,1.0).getVbo();

    //  Load Geometry
    //
    if (geom_index == -1) {
        m_model_vbo = m_billboard_vbo;
    }
    else {
        Mesh model;
        model.load( _files[geom_index].path );
        m_model_vbo = model.getVbo();
        m_model_node.setPosition( -getCentroid( model.getVertices() ) );
        // 
        // Build bbox
        m_bbox_vbo = cubeCorners( model.getVertices(), 0.25 ).getVbo();
        
        float size = getSize( model.getVertices() );
        m_cam.setDistance( size * 2.0 );

        if (model.hasColors()) {
            defines.push_back("MODEL_HAS_COLORS");
        }

        if (model.hasNormals()) {
            defines.push_back("MODEL_HAS_NORMALS");
        }

        if (model.hasTexCoords()) {
            defines.push_back("MODEL_HAS_TEXCOORDS");
        }

        if (model.hasTangents()) {
            defines.push_back("MODEL_HAS_TANGENTS");
        }
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
        m_frag_source = m_model_vbo->getVertexLayout()->getDefaultFragShader();
    }

    if (vert_index != -1) {
        m_vert_source = "";
        m_vert_dependencies.clear();

        loadFromPath(_files[vert_index].path, &m_vert_source, include_folders, &m_vert_dependencies);
    }
    else {
        m_vert_source = m_model_vbo->getVertexLayout()->getDefaultVertShader();
    }

    m_shader.load(m_frag_source, m_vert_source, defines, verbose);
    _updateDependencies( _files );
    
    // CUBEMAP
    if (m_cubemap) {
        m_cubemap_vbo = cube(1.0f).getVbo();
        m_cubemap_shader.load(cube_frag, cube_vert, defines, false);
    }

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    reload();

    // TODO:
    //      - this seams to solve the problem of buffers 
    //      not properly initialize
    //      - digg deeper
    //
    m_buffers.clear();
    _updateBuffers();

    m_ready = true;
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

// ------------------------------------------------------------------------- GET

bool Sandbox::isReady() {
    return m_ready;
}

bool Sandbox::haveChange() { 
    return  m_change || 
            m_record ||
            uniforms_functions["u_time"].present || 
            uniforms_functions["u_delta"].present ||
            uniforms_functions["u_date"].present; 
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
        std::string default_vert = m_model_vbo->getVertexLayout()->getDefaultVertShader(); 
        std::string default_frag = m_model_vbo->getVertexLayout()->getDefaultFragShader(); 
        m_shader.load(default_frag, default_vert, defines, false);
    }
    // If it work check for present uniforms, buffers, background and postprocesing
    else {
        // Check active native uniforms
        for (UniformFunctionsList::iterator it = uniforms_functions.begin(); it != uniforms_functions.end(); ++it) {
            it->second.present = ( find_id(m_frag_source, it->first.c_str()) != 0 || find_id(m_vert_source, it->first.c_str()) != 0 );
        }
        // Flag all user defined uniforms as changed
        for (UniformDataList::iterator it = uniforms_data.begin(); it != uniforms_data.end(); ++it) {
            it->second.change = true;
        }

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
            m_scene_fbo.allocate(getWindowWidth(), getWindowHeight(), true, uniforms_functions["u_scene_depth"].present);
        }
        m_postprocessing_enabled = havePostprocessing;
        if (m_postprocessing_enabled) {
            // Specific defines for this buffer
            std::vector<std::string> sub_defines = defines;
            sub_defines.push_back("POSTPROCESSING");
            m_postprocessing_shader.load(m_frag_source, billboard_vert, sub_defines, verbose);
        }
    }

    m_change = true;

    return success;
}

// ------------------------------------------------------------------------- UPDATE
void Sandbox::_updateBuffers() {
    if ( m_buffers_total != int(m_buffers.size()) ) {
        m_buffers.clear();
        m_buffers_shaders.clear();

        for (int i = 0; i < m_buffers_total; i++) {
            // New FBO
            m_buffers.push_back( Fbo() );
            m_buffers[i].allocate(getWindowWidth(), getWindowHeight(), false);
            
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

void Sandbox::_updateUniforms( Shader &_shader ) {
    // Pass Native uniforms 
    for (UniformFunctionsList::iterator it=uniforms_functions.begin(); it!=uniforms_functions.end(); ++it) {
        if (it->second.present) {
            if (it->second.assign) {
                it->second.assign(_shader);
            }
        }
    }

    // Pass User defined uniforms
    for (UniformDataList::iterator it=uniforms_data.begin(); it!=uniforms_data.end(); ++it) {
        if (it->second.change) {
            if (it->second.bInt) {
                _shader.setUniform(it->first, int(it->second.value[0]));
                m_change = true;
            }
            else {
                _shader.setUniform(it->first, it->second.value, it->second.size);
                m_change = true;
            }
            it->second.change = false;
        }
    }
}

void Sandbox::_updateTextures( Shader &_shader, int &_m_textureIndex ) {
    // Pass Textures Uniforms
    for (TextureList::iterator it = textures.begin(); it != textures.end(); ++it) {
        _shader.setUniformTexture(it->first, it->second, _m_textureIndex++ );
        _shader.setUniform(it->first+"Resolution", it->second->getWidth(), it->second->getHeight());
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

void Sandbox::draw() {
    // BUFFERS
    // -----------------------------------------------

    m_textureIndex = 0;
    for (unsigned int i = 0; i < m_buffers.size(); i++) {
        m_textureIndex = 0;
        m_buffers[i].bind();
        m_buffers_shaders[i].use();

        // Update uniforms variables
        _updateUniforms( m_buffers_shaders[i] );

        // Update textures
        _updateTextures( m_buffers_shaders[i], m_textureIndex );

        // Pass textures for the other buffers
        for (unsigned int j = 0; j < m_buffers.size(); j++) {
            if (i != j) {
                m_buffers_shaders[i].setUniformTexture("u_buffer" + toString(j), &m_buffers[j], m_textureIndex++ );
            }
        }

        if (m_postprocessing_enabled) {
            m_buffers_shaders[i].setUniformTexture("u_scene", &m_scene_fbo, m_textureIndex++ );
        }

        m_billboard_vbo->draw( &m_buffers_shaders[i] );

        m_buffers[i].unbind();
    }

    // MAIN SCENE
    // ----------------------------------------------- < main scene start
    if (m_postprocessing_enabled) {
        m_scene_fbo.bind();
    }

    // BACKGROUND
    if (m_background_enabled) {
        m_textureIndex = 0;
        m_background_shader.use();

        // Update Uniforms variables
        _updateUniforms( m_background_shader );

        // Update textures
        _updateTextures( m_background_shader, m_textureIndex );

        // Pass all buffers
        for (unsigned int i = 0; i < m_buffers.size(); i++) {
            m_background_shader.setUniformTexture("u_buffer" + toString(i), &m_buffers[i], m_textureIndex++ );
        }

        if (m_postprocessing_enabled) {
            m_background_shader.setUniformTexture("u_scene", &m_scene_fbo, m_textureIndex++ );
        }

        m_billboard_vbo->draw( &m_background_shader );
    }
    // CUBEMAP
    else if (geom_index != -1 && m_cubemap) {
        m_textureIndex = 0;
        m_cubemap_shader.use();

        m_cubemap_shader.setUniform("u_modelViewProjectionMatrix", m_cam.getProjectionMatrix() * glm::toMat4(m_cam.getOrientationQuat()) );
        m_cubemap_shader.setUniformTextureCube( "u_cubeMap", m_cubemap, m_textureIndex++ );

        // glDisable(GL_DEPTH_TEST);
        m_cubemap_vbo->draw( &m_cubemap_shader );
    }

    // Begining of DEPTH for 3D 
    if (geom_index != -1) {
        glEnable(GL_DEPTH_TEST);

        if (m_culling != 0) {
            glEnable(GL_CULL_FACE);

            if (m_culling == 1) {
                glCullFace(GL_FRONT);
            }
            else if (m_culling == 2) {
                glCullFace(GL_BACK);
            }
            else if (m_culling == 3) {
                glCullFace(GL_FRONT_AND_BACK);
            }
        }
    }

    // MAIN SHADER
    m_textureIndex = 0;
    m_shader.use();

    // Update Uniforms variables
    _updateUniforms( m_shader );

    // Update textures
    _updateTextures( m_shader, m_textureIndex );

    // Pass all buffers
    for (unsigned int i = 0; i < m_buffers.size(); i++) {
        m_shader.setUniformTexture("u_buffer" + toString(i), &m_buffers[i], m_textureIndex++ );
    }

    // Pass special uniforms
    m_mvp = glm::mat4(1.);
    if (geom_index == -1) {
        m_shader.setUniform("u_modelViewProjectionMatrix", m_mvp);
        m_model_vbo->draw( &m_shader );
    }
    else {
        // m_mvp = m_cam.getProjectionViewMatrix() * m_model_matrix;
        m_mvp = m_cam.getProjectionViewMatrix() * m_model_node.getTransformMatrix();
        m_shader.setUniform("u_modelViewProjectionMatrix", m_mvp);
        m_model_vbo->draw( &m_shader );

        glDisable(GL_DEPTH_TEST);

        if (m_culling != 0) {
            glDisable(GL_CULL_FACE);
        }
    }
    // ----------------------------------------------- < main scene end

    // POST PROCESSING
    if (m_postprocessing_enabled) {
        m_scene_fbo.unbind();
    
        m_textureIndex = 0;
        m_postprocessing_shader.use();

        // Update uniforms variables
        _updateUniforms( m_postprocessing_shader );

        // Update textures
        _updateTextures( m_postprocessing_shader, m_textureIndex );

        // Pass textures of buffers
        for (unsigned int i = 0; i < m_buffers.size(); i++) {
            m_postprocessing_shader.setUniformTexture("u_buffer" + toString(i), &m_buffers[i], m_textureIndex++ );
        }

        m_postprocessing_shader.setUniformTexture("u_scene", &m_scene_fbo, m_textureIndex++ );

        m_billboard_vbo->draw( &m_postprocessing_shader );
    }
}

void Sandbox::drawDebug3D() {
    if (debug) {
        if (geom_index != -1) {

            // Bounding box
            if (!m_wireframe3D_shader.isLoaded())
                m_wireframe3D_shader.load(wireframe3D_frag, wireframe3D_vert, defines, false);

            glEnable(GL_DEPTH_TEST);
            glLineWidth(2.0f);
            m_wireframe3D_shader.use();
            m_wireframe3D_shader.setUniform("u_color", glm::vec4(1.0, 1.0, 0.0, 1.0));
            m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp);
            m_bbox_vbo->draw( &m_wireframe3D_shader );
            glLineWidth(1.0f);
            
            if (!m_light_shader.isLoaded())
                m_light_shader.load(light_frag, light_vert, defines, false);

            m_light_shader.use();
            m_light_shader.setUniform("u_scale", 24, 24);
            m_light_shader.setUniform("u_translate", m_light.getPosition());
            m_light_shader.setUniform("u_color", glm::vec4(m_light.color, 1.0));
            m_light_shader.setUniform("u_viewMatrix", m_cam.getViewMatrix());
            m_light_shader.setUniform("u_modelViewProjectionMatrix", m_mvp);
            m_billboard_vbo->draw( &m_light_shader );

            glDisable(GL_DEPTH_TEST);
        }
    }
}

void Sandbox::drawDebug2D() {
    if (debug) {        
        // DEBUG BUFFERS
        int nTotal = m_buffers.size();
        if (m_postprocessing_enabled) {
            nTotal += uniforms_functions["u_scene"].present + uniforms_functions["u_scene_depth"].present;
        }
        if (nTotal > 0) {
            float w = (float)(getWindowWidth());
            float h = (float)(getWindowHeight());
            float scale = MIN(1.0f / (float)(nTotal), 0.25) * 0.5;
            float xStep = w * scale;
            float yStep = h * scale;
            float margin = 5.0;
            float xOffset = xStep + margin;
            float yOffset = h - yStep - margin;

            m_textureIndex = 0;

            if (!m_billboard_shader.isLoaded())
                m_billboard_shader.load(dynamic_billboard_frag, dynamic_billboard_vert, defines, false);

            m_billboard_shader.use();
            for (unsigned int i = 0; i < m_buffers.size(); i++) {
                m_billboard_shader.setUniform("u_scale", xStep, yStep);
                m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                m_billboard_shader.setUniformTexture("u_tex0", &m_buffers[i], m_textureIndex++);
                m_billboard_vbo->draw(&m_billboard_shader);
                yOffset -= yStep * 2.0 + margin;
            }
            if (m_postprocessing_enabled) {
                if (uniforms_functions["u_scene"].present) {
                    m_billboard_shader.setUniform("u_scale", xStep, yStep);
                    m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                    m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                    m_billboard_shader.setUniformTexture("u_tex0", &m_scene_fbo, m_textureIndex++);
                    m_billboard_vbo->draw(&m_billboard_shader);
                    yOffset -= yStep * 2.0 + margin;
                }
            }
        }
    }

    if (cursor) {
        if (m_cross_vbo == nullptr)
            m_cross_vbo = cross(glm::vec3(0.,0.,0.1), 10.).getVbo();

        if (!m_wireframe2D_shader.isLoaded())
            m_wireframe2D_shader.load(wireframe2D_frag, wireframe2D_vert, defines, false);

        glLineWidth(2.0f);
        m_wireframe2D_shader.use();
        m_wireframe2D_shader.setUniform("u_color", glm::vec4(1.0));
        m_wireframe2D_shader.setUniform("u_scale", 1.0, 1.0);
        m_wireframe2D_shader.setUniform("u_translate", getMouseX(), getMouseY());
        m_wireframe2D_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
        m_cross_vbo->draw(&m_wireframe2D_shader);
        glLineWidth(1.0f);
    }
}

void Sandbox::drawDone() {
    // RECORD
    if (m_record) {
        onScreenshot(toString(m_record_counter,0,3,'0') + ".png");
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

    m_change = false;
}

// ------------------------------------------------------------------------- ACTIONS

void Sandbox::clean() {
    // Delete Textures
    for (TextureList::iterator i = textures.begin(); i != textures.end(); ++i) {
        delete i->second;
        i->second = NULL;
    }
    textures.clear();

    if (m_cubemap)
        delete m_cubemap;

    // Delete VBO's
    if (m_model_vbo != nullptr && 
        m_model_vbo != m_billboard_vbo)
        delete m_model_vbo;

    if (m_billboard_vbo)
        delete m_billboard_vbo;
        
    if (m_cross_vbo)
        delete m_cross_vbo;
        

    if (m_bbox_vbo)
        delete m_bbox_vbo;

    if (m_cubemap_vbo)
        delete m_cubemap_vbo;
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
        for (TextureList::iterator it = textures.begin(); it!=textures.end(); ++it) {
            if (filename == it->second->getFilePath()) {
                it->second->load(filename, _files[index].vFlip);
                break;
            }
        }
    }
    else if (type == CUBEMAP) {
        if (m_cubemap) {
            m_cubemap->load(filename, _files[index].vFlip);
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
        
        m_change = true;
    }
}

void Sandbox::onMouseDrag(float _x, float _y, int _button) {
    if (_button == 1) {

        // Left-button drag is used to rotate geometry.
        float dist = m_cam.getDistance();
        m_lat -= getMouseVelX();
        m_lon -= getMouseVelY()*0.5;
        m_cam.orbit(m_lat ,m_lon, dist);
        m_cam.lookAt(glm::vec3(0.0));

        // Left-button drag is used to pan u_view2d.
        m_view2d = glm::translate(m_view2d, -getMouseVelocity());
    } 
    else {

        // Right-button drag is used to zoom geometry.
        float dist = m_cam.getDistance();
        dist += (-.008f * getMouseVelY());
        if (dist > 0.0f) {
            m_cam.setDistance( dist );
        }

        // TODO: rotate view2d.

    }

     m_change = true;
}

void Sandbox::onViewportResize(int _newWidth, int _newHeight) {
    if (geom_index != -1) {
        m_cam.setViewport(_newWidth, _newHeight);
    }
    
    for (unsigned int i = 0; i < m_buffers.size(); i++) {
        m_buffers[i].allocate(_newWidth, _newHeight, false);
    }

    if (m_postprocessing_enabled) {
        m_scene_fbo.allocate(_newWidth, _newHeight, true, uniforms_functions["u_scene_depth"].present);
    }
    m_change = true;
}

void Sandbox::onScreenshot(std::string _file) {
    if (_file != "" && isGL()) {
        unsigned char* pixels = new unsigned char[getWindowWidth() * getWindowHeight()*4];
        glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        savePixels(_file, pixels, getWindowWidth(), getWindowHeight());

        if (!m_record) {
            std::cout << "// Screenshot saved to " << _file << std::endl;
            std::cout << "// > ";
        }
    }
}

