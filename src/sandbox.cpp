#include "sandbox.h"

#include <sys/stat.h>   // stat
#include <algorithm>    // std::find
#include <math.h>

#include "window.h"

#include "io/pixels.h"
#include "tools/text.h"
#include "tools/geom.h"

#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "shaders/default.h"
#include "shaders/dynamic_billboard.h"
#include "shaders/histogram.h"
#include "shaders/wireframe2D.h"
#include "shaders/fxaa.h"
#include "shaders/holoplay.h"
#include "shaders/poissonfill.h"

#include <memory>

std::string default_scene_frag = default_scene_frag0 + default_scene_frag1 + default_scene_frag2 + default_scene_frag3;

int holoplay_width = 2048;
int holoplay_height = 2048;
int holoplay_columns = 4;
int holoplay_rows = 8;
int holoplay_totalViews = 32;

// ------------------------------------------------------------------------- CONTRUCTOR
Sandbox::Sandbox(): 
    frag_index(-1), vert_index(-1), geom_index(-1), holoplay(-1),
    verbose(false), cursor(true), fxaa(false),
    // Main Vert/Frag/Geom
    m_frag_source(""), m_vert_source(""),
    // Buffers
    m_buffers_total(0),
    // Poisson Fill
    m_convolution_pyramid_total(0),
    // PostProcessing
    m_postprocessing(false),
    // Geometry helpers
    m_billboard_vbo(nullptr), m_cross_vbo(nullptr),
    // Record
    m_record_fdelta(0.04166666667), m_record_start(0.0f), m_record_head(0.0f), m_record_end(0.0f), m_record_counter(0), m_record(false),
    // Histogram
    m_histogram_texture(nullptr), m_histogram(false),
    // Scene
    m_view2d(1.0), m_time_offset(0.0), m_lat(180.0), m_lon(0.0), m_frame(0), m_change(true), m_initialized(false), m_error_screen(true),
    // Debug
    m_showTextures(false), m_showPasses(false),
    m_task_count(0),

    /** allow 500 MB to be used for the image save queue **/
    m_max_mem_in_queue(500 * 1024 * 1024),

    m_save_threads(std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1))
{

    // TIME UNIFORMS
    //
    uniforms.functions["u_frame"] = UniformFunction( "int", [this](Shader& _shader) {
        _shader.setUniform("u_frame", (int)m_frame);
    }, [this]() { return toString(m_frame); } );

    uniforms.functions["u_time"] = UniformFunction( "float", [this](Shader& _shader) {
        if (m_record) _shader.setUniform("u_time", m_record_head);
        else _shader.setUniform("u_time", float(getTime()) - m_time_offset);
    }, [this]() { return toString(getTime() - m_time_offset); } );

    uniforms.functions["u_delta"] = UniformFunction("float", [this](Shader& _shader) {
        if (m_record) _shader.setUniform("u_delta", float(m_record_fdelta));
        else _shader.setUniform("u_delta", float(getDelta()));
    },
    []() { return toString(getDelta()); });

    uniforms.functions["u_date"] = UniformFunction("vec4", [](Shader& _shader) {
        _shader.setUniform("u_date", getDate());
    },
    []() { return toString(getDate(), ','); });

    // MOUSE
    uniforms.functions["u_mouse"] = UniformFunction("vec2", [](Shader& _shader) {
        _shader.setUniform("u_mouse", float(getMouseX()), float(getMouseY()));
    },
    []() { return toString(getMouseX()) + "," + toString(getMouseY()); } );

    // VIEWPORT
    uniforms.functions["u_resolution"]= UniformFunction("vec2", [](Shader& _shader) {
        _shader.setUniform("u_resolution", float(getWindowWidth()), float(getWindowHeight()));
    },
    []() { return toString(getWindowWidth()) + "," + toString(getWindowHeight()); });

    // SCENE
    uniforms.functions["u_scene"] = UniformFunction("sampler2D", [this](Shader& _shader) {
        if (m_postprocessing && m_scene_fbo.getTextureId()) {
            _shader.setUniformTexture("u_scene", &m_scene_fbo, _shader.textureIndex++ );
        }
    });

    #if !defined(PLATFORM_RPI)
    uniforms.functions["u_sceneDepth"] = UniformFunction("sampler2D", [this](Shader& _shader) {
        if (m_postprocessing && m_scene_fbo.getTextureId()) {
            _shader.setUniformDepthTexture("u_sceneDepth", &m_scene_fbo, _shader.textureIndex++ );
        }
    });

    uniforms.functions["u_lightShadowMap"] = UniformFunction("sampler2D", [this](Shader& _shader) {
        if (uniforms.lights.size() > 0) {
            _shader.setUniformDepthTexture("u_lightShadowMap", uniforms.lights[0].getShadowMap(), _shader.textureIndex++ );
        }
    });
    #endif

    uniforms.functions["u_view2d"] = UniformFunction("mat3", [this](Shader& _shader) {
        _shader.setUniform("u_view2d", m_view2d);
    });

    uniforms.functions["u_modelViewProjectionMatrix"] = UniformFunction("mat4");
}

Sandbox::~Sandbox() {
    /** make sure every frame is saved before exiting **/
    if (m_task_count > 0) {
        std::cout << "saving remaining frames to disk, this might take a while ..." << std::endl;
    }
    while (m_task_count > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
}

// ------------------------------------------------------------------------- SET

void Sandbox::setup( WatchFileList &_files, CommandList &_commands ) {

    // Add Sandbox Commands
    // ----------------------------------------
    _commands.push_back(Command("debug", [&](const std::string& _line){
        if (_line == "debug") {
            std::string rta = m_showPasses ? "on" : "off";
            std::cout << "buffers," << rta << std::endl; 
            rta = m_showTextures ? "on" : "off";
            std::cout << "textures," << rta << std::endl; 
            if (geom_index != -1) {
                rta = m_scene.showGrid ? "on" : "off";
                std::cout << "grid," << rta << std::endl; 
                rta = m_scene.showAxis ? "on" : "off";
                std::cout << "axis," << rta << std::endl; 
                rta = m_scene.showBBoxes ? "on" : "off";
                std::cout << "bboxes," << rta << std::endl;
            }
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                m_showPasses = (values[1] == "on");
                m_showTextures = (values[1] == "on");
                // m_histogram = (values[1] == "on");
                if (geom_index != -1) {
                    m_scene.showGrid = (values[1] == "on");
                    m_scene.showAxis = (values[1] == "on");
                    m_scene.showBBoxes = (values[1] == "on");
                    if (values[1] == "on") {
                        m_scene.addDefine("DEBUG", values[1]);
                    }
                    else {
                        m_scene.delDefine("DEBUG");
                    }
                }
            }
        }
        return false;
    },
    "debug[,on|off]                 show/hide passes and textures elements", false));

    _commands.push_back(Command("reset", [&](const std::string& _line){
        if (_line == "reset") {
            m_time_offset = getTime();
            return true;
        }
        return false;
    },
    "reset                          reset timestamp back to zero", false));

    _commands.push_back(Command("time", [&](const std::string& _line){ 
        if (_line == "time") {
            // Force the output in floats
            printf("%f\n", getTime() - m_time_offset);
            return true;
        }
        return false;
    },
    "time                           return u_time, the elapsed time.", false));

    _commands.push_back(Command("histogram", [&](const std::string& _line){
        if (_line == "histogram") {
            std::string rta = m_histogram ? "on" : "off";
            std::cout << "histogram," << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                m_histogram = (values[1] == "on");
            }
        }
        return false;
    },
    "histogram[,on|off]             show/hide histogram", false));

    _commands.push_back(Command("defines", [&](const std::string& _line){ 
        if (_line == "defines") {
            if (geom_index == -1)
                m_canvas_shader.printDefines();
            else
                m_scene.printDefines();
            return true;
        }
        return false;
    },
    "defines                        return a list of active defines", false));
    
    _commands.push_back(Command("uniforms", [&](const std::string& _line){ 
        uniforms.print(_line == "uniforms,all");
        return true;
    },
    "uniforms[,all|active]          return a list of all or active uniforms and their values.", false));

    _commands.push_back(Command("textures", [&](const std::string& _line){ 
        if (_line == "textures") {
            uniforms.printTextures();
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                m_showTextures = (values[1] == "on");
            }
        }
        return false;
    },
    "textures                       return a list of textures as their uniform name and path.", false));

    _commands.push_back(Command("buffers", [&](const std::string& _line){ 
        if (_line == "buffers") {
            uniforms.printBuffers();
            if (m_postprocessing) {
                if (holoplay >= 0)
                    std::cout << "HOLO";
                else if (fxaa)
                    std::cout << "FXAA";
                else
                    std::cout << "Custom";
                std::cout << " postProcessing pass" << std::endl;
            }
            
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                m_showPasses = (values[1] == "on");
            }
        }
        return false;
    },
    "buffers                        return a list of buffers as their uniform name.", false));

    _commands.push_back(Command("error_screen", [&](const std::string& _line){ 
        if (_line == "error_screen") {
            std::string rta = m_error_screen ? "on" : "off";
            std::cout << "error_screen," << rta << std::endl; 
            
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                m_error_screen = (values[1] == "on");
            }
        }
        return false;
    },
    "error_screen                   display magenta screen on errors", false));

    // LIGTH
    _commands.push_back(Command("lights", [&](const std::string& _line){ 
        if (_line == "lights") {
            uniforms.printLights();
            return true;
        }
        return false;
    },
    "lights                         get all light data."));

    _commands.push_back(Command("light_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            if (uniforms.lights.size() > 0) 
                uniforms.lights[0].setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            return true;
        }
        else if (values.size() == 5) {
            unsigned int i = toInt(values[1]);
            if (uniforms.lights.size() > i) 
                uniforms.lights[i].setPosition(glm::vec3(toFloat(values[2]),toFloat(values[3]),toFloat(values[4])));
            return true;
        }
        else {
            if (uniforms.lights.size() > 0) {
                glm::vec3 pos = uniforms.lights[0].getPosition();
                std::cout << ',' << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            }
            return true;
        }
        return false;
    },
    "light_position[,<x>,<y>,<z>]   get or set the light position."));

    _commands.push_back(Command("light_color", [&](const std::string& _line){ 
         std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            if (uniforms.lights.size() > 0) {
                uniforms.lights[0].color = glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
                uniforms.lights[0].bChange = true;
            }
            return true;
        }
        else if (values.size() == 5) {
            unsigned int i = toInt(values[1]);
            if (uniforms.lights.size() > i) {
                uniforms.lights[i].color = glm::vec3(toFloat(values[2]),toFloat(values[3]),toFloat(values[4]));
                uniforms.lights[i].bChange = true;
            }
            return true;
        }
        else {
            if (uniforms.lights.size() > 0) {
                glm::vec3 color = uniforms.lights[0].color;
                std::cout << color.x << ',' << color.y << ',' << color.z << std::endl;
            }
            
            return true;
        }
        return false;
    },
    "light_color[,<r>,<g>,<b>]      get or set the light color."));

    _commands.push_back(Command("light_falloff", [&](const std::string& _line){ 
         std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            if (uniforms.lights.size() > 0) {
                uniforms.lights[0].falloff = toFloat(values[1]);
                uniforms.lights[0].bChange = true;
            }
            return true;
        }
        else if (values.size() == 5) {
            unsigned int i = toInt(values[1]);
            if (uniforms.lights.size() > i) {
                uniforms.lights[i].falloff = toFloat(values[2]);
                uniforms.lights[i].bChange = true;
            }
            return true;
        }
        else {
            if (uniforms.lights.size() > 0) {
                std::cout <<  uniforms.lights[0].falloff << std::endl;
            }
            return true;
        }
        return false;
    },
    "light_falloff[,<value>]        get or set the light falloff distance."));

    _commands.push_back(Command("light_intensity", [&](const std::string& _line){ 
         std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            if (uniforms.lights.size() > 0) {
                uniforms.lights[0].intensity = toFloat(values[1]);
                uniforms.lights[0].bChange = true;
            }
            return true;
        }
        else if (values.size() == 5) {
            unsigned int i = toInt(values[1]);
            if (uniforms.lights.size() > i) {
                uniforms.lights[i].intensity = toFloat(values[2]);
                uniforms.lights[i].bChange = true;
            }
            return true;
        }
        else {
            if (uniforms.lights.size() > 0) {
                std::cout <<  uniforms.lights[0].intensity << std::endl;
            }
            
            return true;
        }
        return false;
    },
    "light_intensity[,<value>]      get or set the light intensity."));

    // CAMERA
    _commands.push_back(Command("camera_distance", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            uniforms.getCamera().setDistance(toFloat(values[1]));
            return true;
        }
        else {
            std::cout << uniforms.getCamera().getDistance() << std::endl;
            return true;
        }
        return false;
    },
    "camera_distance[,<dist>]       get or set the camera distance to the target."));

    _commands.push_back(Command("camera_type", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            if (values[1] == "ortho")
                uniforms.getCamera().setType(CameraType::ORTHO);
            else if (values[1] == "perspective")
                uniforms.getCamera().setType(CameraType::PERSPECTIVE);
            return true;
        }
        else {
            CameraType type = uniforms.getCamera().getType();
            if (type == CameraType::ORTHO)
                std::cout << "ortho" << std::endl;
            else
                std::cout << "perspective" << std::endl;
            return true;
        }
        return false;
    },
    "camera_type[,<ortho|perspective>] get or set the camera type"));

    _commands.push_back(Command("camera_fov", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            uniforms.getCamera().setFOV(toFloat(values[1]));
            return true;
        }
        else {
            std::cout << uniforms.getCamera().getFOV() << std::endl;
            return true;
        }
        return false;
    },
    "camera_fov[,<field_of_view>]   get or set the camera field of view."));

    _commands.push_back(Command("camera_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            uniforms.getCamera().setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            uniforms.getCamera().lookAt(uniforms.getCamera().getTarget());
            return true;
        }
        else {
            glm::vec3 pos = uniforms.getCamera().getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "camera_position[,<x>,<y>,<z>]  get or set the camera position."));

    _commands.push_back(Command("camera_exposure", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            uniforms.getCamera().setExposure(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            return true;
        }
        else {
            std::cout << uniforms.getCamera().getExposure() << std::endl;
            return true;
        }
        return false;
    },
    "camera_exposure[,<aper.>,<shutter>,<sensit.>]  get or set the camera exposure values."));

    _commands.push_back(Command("max_mem_in_queue", [&](const std::string & line) {
        std::vector<std::string> values = split(line,',');
        if (values.size() == 2) {
            m_max_mem_in_queue = std::stoll(values[1]);
        }
        else {
            std::cout << m_max_mem_in_queue.load() << std::endl;
        }
        return false;
    }, "max_mem_in_queue[,<bytes>]     set the maximum amount of memory used by a queue to export images to disk"));

    // LOAD SHACER 
    // -----------------------------------------------


    if (frag_index != -1) {
        // If there is a Fragment shader load it
        m_frag_source = "";
        m_frag_dependencies.clear();

        if ( !loadFromPath(_files[frag_index].path, &m_frag_source, include_folders, &m_frag_dependencies) ) {
            return;
        }
    }
    else {
        // If there is no use the default one
        if (geom_index == -1)
            m_frag_source = default_frag;
        else
            m_frag_source = default_scene_frag;
    }

    if (vert_index != -1) {
        // If there is a Vertex shader load it
        m_vert_source = "";
        m_vert_dependencies.clear();

        loadFromPath(_files[vert_index].path, &m_vert_source, include_folders, &m_vert_dependencies);
    }
    else 
        m_vert_source = getDefaultSrc(VERT_DEFAULT);   // If there is no use the default one

    // Init Scene elements
    m_billboard_vbo = rect(0.0,0.0,1.0,1.0).getVbo();

    // LOAD GEOMETRY
    // -----------------------------------------------

    if (geom_index == -1) {
        m_canvas_shader.addDefine("MODEL_VERTEX_TEXCOORD");
        uniforms.getCamera().orbit(m_lat, m_lon, 2.0);
    }
    else {
        m_scene.setup( _commands, uniforms);
        m_scene.loadGeometry( uniforms, _files, geom_index, verbose );
        uniforms.getCamera().orbit(m_lat, m_lon, m_scene.getArea() * 2.0);
    }

    // FINISH SCENE SETUP
    // -------------------------------------------------
    uniforms.getCamera().setViewport(getWindowWidth(), getWindowHeight());

    if (holoplay >= 0) {
        addDefine("HOLOPLAY", toString(holoplay));

        uniforms.getCamera().setFOV(glm::radians(14.0f));
        uniforms.getCamera().setType(CameraType::PERSPECTIVE_VIRTUAL_OFFSET);
        // uniforms.getCamera().setClipping(0.01, 100.0);

        if (geom_index != -1)
            uniforms.getCamera().orbit(m_lat, m_lon, m_scene.getArea() * 8.5);

        if (holoplay == 0) {
            holoplay_width = 2048;
            holoplay_height = 2048;
            holoplay_columns = 4;
            holoplay_rows = 8;
            holoplay_totalViews = 32;
        }
        else if (holoplay == 1) {
            holoplay_width = 4096;
            holoplay_height = 4096;
            holoplay_columns = 5;
            holoplay_rows = 9;
            holoplay_totalViews = 45;
        }
        else if (holoplay == 2) {
            holoplay_width = 4096 * 2;
            holoplay_height = 4096 * 2;
            holoplay_columns = 5;
            holoplay_rows = 9;
            holoplay_totalViews = 45;
        }
    } 

    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // LOAD SHADERS
    reloadShaders( _files );

    // TODO:
    //      - this seams to solve the problem of buffers not properly initialize
    //      - digg deeper
    //
    uniforms.buffers.clear();
    _updateBuffers();

    flagChange();
}

void Sandbox::addDefine(const std::string &_define, const std::string &_value) {
    for (int i = 0; i < m_buffers_total; i++)
        m_buffers_shaders[i].addDefine(_define, _value);

    if (geom_index == -1)
        m_canvas_shader.addDefine(_define, _value);
    else
        m_scene.addDefine(_define, _value);

    m_postprocessing_shader.addDefine(_define, _value);
}

void Sandbox::delDefine(const std::string &_define) {
    for (int i = 0; i < m_buffers_total; i++)
        m_buffers_shaders[i].delDefine(_define);

    if (geom_index == -1)
        m_canvas_shader.delDefine(_define);
    else
        m_scene.delDefine(_define);

    m_postprocessing_shader.delDefine(_define);
}

// ------------------------------------------------------------------------- GET

bool Sandbox::isReady() {
    return m_initialized;
}

void Sandbox::flagChange() { 
    m_change = true;
}

void Sandbox::unflagChange() {
    m_change = false;
    m_scene.unflagChange();
    uniforms.unflagChange();
}

bool Sandbox::haveChange() { 

    // std::cout << "CHANGE " << m_change << std::endl;
    // std::cout << "RECORD " << m_record << std::endl;
    // std::cout << "SCENE " << m_scene.haveChange() << std::endl;
    // std::cout << "UNIFORMS " << uniforms.haveChange() << std::endl;
    // std::cout << std::endl;

    return  m_change ||
            m_record ||
            screenshotFile != "" ||
            m_scene.haveChange() ||
            uniforms.haveChange();
}

std::string Sandbox::getSource(ShaderType _type) const {
    return (_type == FRAGMENT)? m_frag_source : m_vert_source;
}

std::string Sandbox::getDefaultSrc( DefaultShaders _type ) {
    size_t versionNumber = 100;
    std::string rta = get_version(m_frag_source, versionNumber);

    if (_type == VERT_DEFAULT) {
        if (versionNumber < 300)
            rta += (geom_index == -1) ? default_vert : default_scene_vert;
        else if (versionNumber >= 300) 
            rta += (geom_index == -1) ? default_vert_300 : default_scene_vert_300;
    }
    else if (_type == VERT_BILLBOARD) {
        if (versionNumber < 300)
            rta += billboard_vert;
        else if (versionNumber >= 300) 
            rta += billboard_vert_300;
    }
    else if (_type == VERT_DYNAMIC_BILLBOARD) {
        if (versionNumber < 300)
            rta += dynamic_billboard_vert;
        else if (versionNumber >= 300) 
            rta += dynamic_billboard_vert_300;
    }
    else if (_type == FRAG_DYNAMIC_BILLBOARD) {
        if (versionNumber < 300)
            rta += dynamic_billboard_frag;
        else if (versionNumber >= 300) 
            rta += dynamic_billboard_frag_300;
    }

    return rta;
}

int Sandbox::getRecordedPercentage() {
    return ((m_record_head - m_record_start) / (m_record_end - m_record_start)) * 100;
}

// ------------------------------------------------------------------------- RELOAD SHADER

void Sandbox::_updateSceneBuffer(int _width, int _height) {
    FboType type = uniforms.functions["u_sceneDepth"].present ? COLOR_DEPTH_TEXTURES : COLOR_TEXTURE_DEPTH_BUFFER;

    if (holoplay >= 0) {
        _width = holoplay_width;
        _height= holoplay_height;
    }

    if (!m_scene_fbo.isAllocated() ||
        m_scene_fbo.getType() != type || 
        m_scene_fbo.getWidth() != _width || 
        m_scene_fbo.getHeight() != _height )
        m_scene_fbo.allocate(_width, _height, type);
}

bool Sandbox::reloadShaders( WatchFileList &_files ) {
    flagChange();

    // UPDATE scene shaders of models (materials)
    if (geom_index == -1) {

        if (verbose)
            std::cout << "// Reload 2D shaders" << std::endl;

        // Reload the shader
        m_canvas_shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
        m_canvas_shader.load(m_frag_source, m_vert_source, verbose, m_error_screen);
    }
    else {
        if (verbose)
            std::cout << "// Reload 3D scene shaders" << std::endl;

        m_scene.loadShaders(m_frag_source, m_vert_source, verbose);
    }

    // UPDATE shaders dependencies
    {
        List new_dependencies = merge(m_frag_dependencies, m_vert_dependencies);

        // remove old dependencies
        for (int i = _files.size() - 1; i >= 0; i--)
            if (_files[i].type == GLSL_DEPENDENCY)
                _files.erase( _files.begin() + i);

        // Add new dependencies
        struct stat st;
        for (unsigned int i = 0; i < new_dependencies.size(); i++) {
            WatchFile file;
            file.type = GLSL_DEPENDENCY;
            file.path = new_dependencies[i];
            stat( file.path.c_str(), &st );
            file.lastChange = st.st_mtime;
            _files.push_back(file);

            if (verbose)
                std::cout << " Watching file " << new_dependencies[i] << " as a dependency " << std::endl;
        }
    }

    // UPDATE uniforms
    uniforms.checkPresenceIn(m_vert_source, m_frag_source); // Check active native uniforms
    uniforms.flagChange();                                  // Flag all user defined uniforms as changed

    if (uniforms.cubemap) {
        addDefine("SCENE_SH_ARRAY", "u_SH");
        addDefine("SCENE_CUBEMAP", "u_cubeMap");
    }

    // UPDATE Buffers
    m_buffers_total = count_buffers(m_frag_source);
    _updateBuffers();

    // Convolution Pyramids
    m_convolution_pyramid_total = count_convolution_pyramid(getSource(FRAGMENT));
    _updateConvolutionPyramids();
    
    // UPDATE Postprocessing
    bool havePostprocessing = check_for_postprocessing(getSource(FRAGMENT));
    if (havePostprocessing) {
        // Specific defines for this buffer
        m_postprocessing_shader.addDefine("POSTPROCESSING");
        m_postprocessing_shader.load(m_frag_source, getDefaultSrc(VERT_BILLBOARD), false);
        m_postprocessing = havePostprocessing;
    }
    else if (holoplay >= 0) {
        m_postprocessing_shader.load(holoplay_frag, getDefaultSrc(VERT_BILLBOARD), false);
        uniforms.functions["u_scene"].present = true;
        m_postprocessing = true;
    }
    else if (fxaa) {
        m_postprocessing_shader.load(fxaa_frag, getDefaultSrc(VERT_BILLBOARD), false);
        uniforms.functions["u_scene"].present = true;
        m_postprocessing = true;
    }
    else 
        m_postprocessing = false;

    if (m_postprocessing || m_histogram)
        _updateSceneBuffer(getWindowWidth(), getWindowHeight());

    return true;
}

// ------------------------------------------------------------------------- UPDATE
void Sandbox::_updateBuffers() {
    if ( m_buffers_total != int(uniforms.buffers.size()) ) {

        if (verbose)
            std::cout << " Creating/Removing " << uniforms.buffers.size() << " buffers to " << m_buffers_total << std::endl;

        uniforms.buffers.clear();
        m_buffers_shaders.clear();

        for (int i = 0; i < m_buffers_total; i++) {
            // New FBO
            uniforms.buffers.push_back( Fbo() );
            uniforms.buffers[i].allocate(getWindowWidth(), getWindowHeight(), COLOR_TEXTURE);
            
            // New Shader
            m_buffers_shaders.push_back( Shader() );
            m_buffers_shaders[i].addDefine("BUFFER_" + toString(i));
            m_buffers_shaders[i].load(m_frag_source, getDefaultSrc(VERT_BILLBOARD), false);
        }
    }
    else {
        for (unsigned int i = 0; i < m_buffers_shaders.size(); i++) {

            // Reload shader code
            m_buffers_shaders[i].addDefine("BUFFER_" + toString(i));
            m_buffers_shaders[i].load(m_frag_source, getDefaultSrc(VERT_BILLBOARD), false);
        }
    }
}

void Sandbox::_updateConvolutionPyramids() {
    if ( m_convolution_pyramid_total != int(uniforms.convolution_pyramids.size()) ) {

        if (verbose)
            std::cout << "Removing " << uniforms.convolution_pyramids.size() << " convolution pyramids to create  " << m_convolution_pyramid_total << std::endl;

        uniforms.convolution_pyramids.clear();
        m_convolution_pyramid_fbos.clear();
        m_convolution_pyramid_subshaders.clear();
        for (int i = 0; i < m_convolution_pyramid_total; i++) {
            uniforms.convolution_pyramids.push_back( ConvolutionPyramid() );
            uniforms.convolution_pyramids[i].allocate(getWindowWidth(), getWindowHeight());
            uniforms.convolution_pyramids[i].pass = [this](Fbo *_target, const Fbo *_tex0, const Fbo *_tex1, int _depth) {
                _target->bind();
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                m_convolution_pyramid_shader.use();

                uniforms.feedTo(m_convolution_pyramid_shader);

                m_convolution_pyramid_shader.setUniform("u_convolutionPyramidDepth", _depth);
                m_convolution_pyramid_shader.setUniform("u_convolutionPyramidTotalDepth", (int)uniforms.convolution_pyramids[0].getDepth());
                m_convolution_pyramid_shader.setUniform("u_convolutionPyramidUpscaling", _tex1 != NULL);

                m_convolution_pyramid_shader.textureIndex = geom_index == -1 ? 1 : 0;
                m_convolution_pyramid_shader.setUniformTexture("u_convolutionPyramidTex0", _tex0);
                if (_tex1 != NULL)
                    m_convolution_pyramid_shader.setUniformTexture("u_convolutionPyramidTex1", _tex1);
                m_convolution_pyramid_shader.setUniform("u_resolution", ((float)_target->getWidth()), ((float)_target->getHeight()));
                m_convolution_pyramid_shader.setUniform("u_pixel", 1.0f/((float)_target->getWidth()), 1.0f/((float)_target->getHeight()));

                m_billboard_vbo->render( &m_convolution_pyramid_shader );
                _target->unbind();
            };
            m_convolution_pyramid_fbos.push_back( Fbo() );
            m_convolution_pyramid_fbos[i].allocate(getWindowWidth(), getWindowHeight(), COLOR_TEXTURE);
            m_convolution_pyramid_subshaders.push_back( Shader() );
        }
    }
    
    if (check_for_convolution_pyramid_algorithm(getSource(FRAGMENT))) {
        m_convolution_pyramid_shader.addDefine("CONVOLUTION_PYRAMID_ALGORITHM");
        m_convolution_pyramid_shader.load(m_frag_source, getDefaultSrc(VERT_BILLBOARD), false);
    }
    else
        m_convolution_pyramid_shader.load(poissonfill_frag, billboard_vert, false);

    for (unsigned int i = 0; i < m_convolution_pyramid_subshaders.size(); i++) {
        m_convolution_pyramid_subshaders[i].addDefine("CONVOLUTION_PYRAMID_" + toString(i));
        m_convolution_pyramid_subshaders[i].load(m_frag_source, getDefaultSrc(VERT_BILLBOARD), false);
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

        m_billboard_vbo->render( &m_buffers_shaders[i] );

        uniforms.buffers[i].unbind();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Sandbox::_renderConvolutionPyramids() {
    // 
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (unsigned int i = 0; i < m_convolution_pyramid_subshaders.size(); i++) {
        glDisable(GL_BLEND);

        m_convolution_pyramid_fbos[i].bind();
        m_convolution_pyramid_subshaders[i].use();

        // Clear the background
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update uniforms and textures
        uniforms.feedTo( m_convolution_pyramid_subshaders[i] );
        m_billboard_vbo->render( &m_convolution_pyramid_subshaders[i] );

        m_convolution_pyramid_fbos[i].unbind();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        uniforms.convolution_pyramids[i].process(&m_convolution_pyramid_fbos[i]);
    }

}

void Sandbox::render() {

    // UPDATE STREAMING TEXTURES
    // -----------------------------------------------
    if (m_initialized)
        uniforms.updateStreammingTextures();

    // RENDER SHADOW MAP
    // -----------------------------------------------
    if (geom_index != -1)
        if (uniforms.functions["u_lightShadowMap"].present)
            m_scene.renderShadowMap(uniforms);
    
    // BUFFERS
    // -----------------------------------------------
    if (uniforms.buffers.size() > 0)
        _renderBuffers();

    if (m_convolution_pyramid_total > 0)
        _renderConvolutionPyramids();
    
    // MAIN SCENE
    // ----------------------------------------------- < main scene start
    if (screenshotFile != "" || m_record)
        if (!m_record_fbo.isAllocated())
            m_record_fbo.allocate(getWindowWidth(), getWindowHeight(), COLOR_TEXTURE_DEPTH_BUFFER);

    if (m_postprocessing || m_histogram ) {
        _updateSceneBuffer(getWindowWidth(), getWindowHeight());
        m_scene_fbo.bind();
    }
    else if (screenshotFile != "" || m_record )
        m_record_fbo.bind();

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // RENDER CONTENT
    if (geom_index == -1) {

         if (holoplay >= 0) {
            // save the viewport for the total quilt
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);

            // get quilt view dimensions
            int qs_viewWidth = int(float(holoplay_width) / float(holoplay_columns));
            int qs_viewHeight = int(float(holoplay_height) / float(holoplay_rows));

            // Load main shader
            m_canvas_shader.use();

            // render views and copy each view to the quilt
            for (int viewIndex = 0; viewIndex < holoplay_totalViews; viewIndex++) {
                // get the x and y origin for this view
                int x = (viewIndex % holoplay_columns) * qs_viewWidth;
                int y = int(float(viewIndex) / float(holoplay_columns)) * qs_viewHeight;

                // get the x and y origin for this view
                // set the viewport to the view to control the projection extent
                glViewport(x, y, qs_viewWidth, qs_viewHeight);

                // // set the scissor to the view to restrict calls like glClear from making modifications
                glEnable(GL_SCISSOR_TEST);
                glScissor(x, y, qs_viewWidth, qs_viewHeight);

                // set up the camera rotation and position for current view
                uniforms.getCamera().setVirtualOffset(5.0, viewIndex, holoplay_totalViews);
                uniforms.set("u_holoPlayTile", float(holoplay_columns), float(holoplay_rows), float(holoplay_totalViews));
                uniforms.set("u_holoPlayViewport", float(x), float(y), float(qs_viewWidth), float(qs_viewHeight));

                // Update Uniforms and textures variables
                uniforms.feedTo( m_canvas_shader );

                // Pass special uniforms
                m_canvas_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.));
                m_billboard_vbo->render( &m_canvas_shader );

                // reset viewport
                glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

                // // restore scissor
                glDisable(GL_SCISSOR_TEST);
                glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
            }
        }
        else {
            // Load main shader
            m_canvas_shader.use();

            // Update Uniforms and textures variables
            uniforms.feedTo( m_canvas_shader );

            // Pass special uniforms
            m_canvas_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.));
            m_billboard_vbo->render( &m_canvas_shader );
        }
    }
    else {

        if (holoplay >= 0) {
            // save the viewport for the total quilt
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);

            // get quilt view dimensions
            int qs_viewWidth = int(float(holoplay_width) / float(holoplay_columns));
            int qs_viewHeight = int(float(holoplay_height) / float(holoplay_rows));


            // render views and copy each view to the quilt
            for (int viewIndex = 0; viewIndex < holoplay_totalViews; viewIndex++) {
                // get the x and y origin for this view
                int x = (viewIndex % holoplay_columns) * qs_viewWidth;
                int y = int(float(viewIndex) / float(holoplay_columns)) * qs_viewHeight;

                // get the x and y origin for this view
                // set the viewport to the view to control the projection extent
                glViewport(x, y, qs_viewWidth, qs_viewHeight);

                // set the scissor to the view to restrict calls like glClear from making modifications
                glEnable(GL_SCISSOR_TEST);
                glScissor(x, y, qs_viewWidth, qs_viewHeight);

                // set up the camera rotation and position for current view
                uniforms.getCamera().setVirtualOffset(m_scene.getArea(), viewIndex, holoplay_totalViews);
                uniforms.set("u_holoPlayTile", float(holoplay_columns), float(holoplay_rows), float(holoplay_totalViews));
                uniforms.set("u_holoPlayViewport", float(x), float(y), float(qs_viewWidth), float(qs_viewHeight));

                m_scene.render(uniforms);

                if (m_scene.showGrid || m_scene.showAxis || m_scene.showBBoxes)
                    m_scene.renderDebug(uniforms);

                // reset viewport
                glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

                // restore scissor
                glDisable(GL_SCISSOR_TEST);
                glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);
            }
        }
        else {
            m_scene.render(uniforms);
            if (m_scene.showGrid || m_scene.showAxis || m_scene.showBBoxes)
                m_scene.renderDebug(uniforms);
        }

    }
    
    // ----------------------------------------------- < main scene end

    // POST PROCESSING
    if (m_postprocessing) {
        m_scene_fbo.unbind();

        if (screenshotFile != "" || m_record)
            m_record_fbo.bind();
    
        m_postprocessing_shader.use();

        // Update uniforms and textures
        uniforms.feedTo( m_postprocessing_shader );

        if (holoplay >= 0) {
            m_postprocessing_shader.setUniform("u_holoPlayTile", float(holoplay_columns), float(holoplay_rows), float(holoplay_totalViews));
            m_postprocessing_shader.setUniform("u_holoPlayCalibration", holoplay_dpi, holoplay_pitch, holoplay_slope, holoplay_center);
            m_postprocessing_shader.setUniform("u_holoPlayRB", float(holoplay_ri), float(holoplay_bi));
        }

        // Pass textures of buffers
        for (unsigned int i = 0; i < uniforms.buffers.size(); i++)
            m_postprocessing_shader.setUniformTexture("u_buffer" + toString(i), &uniforms.buffers[i]);

        m_billboard_vbo->render( &m_postprocessing_shader );
    }
    else if (m_histogram) {
        m_scene_fbo.unbind();

        if (screenshotFile != "" || m_record)
            m_record_fbo.bind();

        if (!m_billboard_shader.isLoaded())
            m_billboard_shader.load(getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD), false);

        m_billboard_shader.use();
        m_billboard_shader.setUniform("u_depth", 0.0f);
        m_billboard_shader.setUniform("u_scale", 1.0f, 1.0f);
        m_billboard_shader.setUniform("u_translate", 0.0f, 0.0f);
        m_billboard_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.0) );
        m_billboard_shader.setUniformTexture("u_tex0", &m_scene_fbo, 0);
        m_billboard_vbo->render( &m_billboard_shader );
    }
    
    if (screenshotFile != "" || m_record) {
        m_record_fbo.unbind();

        if (!m_billboard_shader.isLoaded())
            m_billboard_shader.load(getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD), false);

        m_billboard_shader.use();
        m_billboard_shader.setUniform("u_depth", 0.0f);
        m_billboard_shader.setUniform("u_scale", 1.0f, 1.0f);
        m_billboard_shader.setUniform("u_translate", 0.0f, 0.0f);
        m_billboard_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.0) );
        m_billboard_shader.setUniformTexture("u_tex0", &m_record_fbo, 0);
        m_billboard_vbo->render( &m_billboard_shader );
    }
}


void Sandbox::renderUI() {
    // IN PUT TEXTURES
    if (m_showTextures) {        
        glDisable(GL_DEPTH_TEST);

        int nTotal = uniforms.textures.size();
        if (nTotal > 0) {
            float w = (float)(getWindowWidth());
            float h = (float)(getWindowHeight());
            float scale = fmin(1.0f / (float)(nTotal), 0.25) * 0.5;
            float xStep = w * scale;
            float yStep = h * scale;
            float xOffset = xStep;
            float yOffset = h - yStep;

            if (!m_billboard_shader.isLoaded())
                m_billboard_shader.load(getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD), false);

            m_billboard_shader.use();

            for (std::map<std::string, Texture*>::iterator it = uniforms.textures.begin(); it != uniforms.textures.end(); it++) {
                m_billboard_shader.setUniform("u_depth", 0.0f);
                m_billboard_shader.setUniform("u_scale", xStep, yStep);
                m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                m_billboard_shader.setUniformTexture("u_tex0", it->second, 0);
                m_billboard_vbo->render(&m_billboard_shader);
                yOffset -= yStep * 2.0;
            }
        }
    }

    // RESULTING BUFFERS
    if (m_showPasses) {        
        glDisable(GL_DEPTH_TEST);

        // DEBUG BUFFERS
        int nTotal = uniforms.buffers.size();
        if (m_convolution_pyramid_total > 0)
            nTotal += uniforms.convolution_pyramids.size();
        if (m_postprocessing) {
            nTotal += uniforms.functions["u_scene"].present;
            nTotal += uniforms.functions["u_sceneDepth"].present;
        }
        nTotal += uniforms.functions["u_lightShadowMap"].present;
        if (nTotal > 0) {
            float w = (float)(getWindowWidth());
            float h = (float)(getWindowHeight());
            float scale = fmin(1.0f / (float)(nTotal), 0.25) * 0.5;
            float xStep = w * scale;
            float yStep = h * scale;
            float xOffset = w - xStep;
            float yOffset = h - yStep;

            if (!m_billboard_shader.isLoaded())
                m_billboard_shader.load(getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD), false);

            m_billboard_shader.use();

            for (unsigned int i = 0; i < uniforms.buffers.size(); i++) {
                m_billboard_shader.setUniform("u_depth", 0.0f);
                m_billboard_shader.setUniform("u_scale", xStep, yStep);
                m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                m_billboard_shader.setUniformTexture("u_tex0", &uniforms.buffers[i]);
                m_billboard_vbo->render(&m_billboard_shader);
                yOffset -= yStep * 2.0;
            }

            for (unsigned int i = 0; i < uniforms.convolution_pyramids.size(); i++) {
                float _x = 0;
                float _sw = xStep;
                float _sh = yStep; 
                for (unsigned int j = 0; j < uniforms.convolution_pyramids[i].getDepth() * 2; j++ ) {
                    m_billboard_shader.setUniform("u_depth", 0.0f);
                    m_billboard_shader.setUniform("u_scale", _sw, _sh);
                    m_billboard_shader.setUniform("u_translate", xOffset + _x, yOffset);
                    m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                    m_billboard_shader.setUniformTexture("u_tex0", uniforms.convolution_pyramids[i].getResult(j), 0);
                    m_billboard_vbo->render(&m_billboard_shader);

                    _x -= _sw;
                    if (j < uniforms.convolution_pyramids[i].getDepth()) {
                        _sw *= 0.5;
                        _sh *= 0.5;
                    }
                    else {
                        _sw *= 2.0;
                        _sh *= 2.0;
                    }
                    _x -= _sw;
                }
                yOffset -= yStep * 2.0;
            }

            if (m_postprocessing) {
                if (uniforms.functions["u_scene"].present) {
                    m_billboard_shader.setUniform("u_depth", 0.0f);
                    m_billboard_shader.setUniform("u_scale", xStep, yStep);
                    m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                    m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                    m_billboard_shader.setUniformTexture("u_tex0", &m_scene_fbo, 0);
                    m_billboard_vbo->render(&m_billboard_shader);
                    yOffset -= yStep * 2.0;
                }

                #if !defined(PLATFORM_RPI)
                if (uniforms.functions["u_sceneDepth"].present) {
                    m_billboard_shader.setUniform("u_scale", xStep, yStep);
                    m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                    m_billboard_shader.setUniform("u_depth", 1.0f);
                    uniforms.functions["u_cameraNearClip"].assign(m_billboard_shader);
                    uniforms.functions["u_cameraFarClip"].assign(m_billboard_shader);
                    uniforms.functions["u_cameraDistance"].assign(m_billboard_shader);
                    m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                    m_billboard_shader.setUniformDepthTexture("u_tex0", &m_scene_fbo);
                    m_billboard_vbo->render(&m_billboard_shader);
                    yOffset -= yStep * 2.0;
                }
                #endif
            }

        #if !defined(PLATFORM_RPI) 
            if (uniforms.functions["u_lightShadowMap"].present) {
                for (unsigned int i = 0; i < uniforms.lights.size(); i++) {
                    if ( uniforms.lights[i].getShadowMap()->getDepthTextureId() ) {
                        m_billboard_shader.setUniform("u_scale", xStep, yStep);
                        m_billboard_shader.setUniform("u_translate", xOffset, yOffset);
                        m_billboard_shader.setUniform("u_depth", 0.0f);
                        m_billboard_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
                        m_billboard_shader.setUniformDepthTexture("u_tex0", uniforms.lights[i].getShadowMap());
                        m_billboard_vbo->render(&m_billboard_shader);
                        yOffset -= yStep * 2.0;
                    }
                }
            }
        #endif
        }
    }

    if (m_histogram && m_histogram_texture) {
        glDisable(GL_DEPTH_TEST);

        float w = 100;
        float h = 50;
        float x = (float)(getWindowWidth()) * 0.5;
        float y = h;

        if (!m_histogram_shader.isLoaded())
            m_histogram_shader.load(histogram_frag, getDefaultSrc(VERT_DYNAMIC_BILLBOARD), false);

        m_histogram_shader.use();
        for (std::map<std::string, Texture*>::iterator it = uniforms.textures.begin(); it != uniforms.textures.end(); it++) {
            m_histogram_shader.setUniform("u_scale", w, h);
            m_histogram_shader.setUniform("u_translate", x, y);
            m_histogram_shader.setUniform("u_resolution", (float)getWindowWidth(), (float)getWindowHeight());
            m_histogram_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
            m_histogram_shader.setUniformTexture("u_sceneHistogram", m_histogram_texture, 0);
            m_billboard_vbo->render(&m_histogram_shader);
        }
    }

    if (cursor && getMouseEntered()) {
        if (m_cross_vbo == nullptr) 
            m_cross_vbo = cross(glm::vec3(0.0, 0.0, 0.0), 10.).getVbo();

        if (!m_wireframe2D_shader.isLoaded())
            m_wireframe2D_shader.load(wireframe2D_frag, wireframe2D_vert, false);

        glLineWidth(2.0f);
        m_wireframe2D_shader.use();
        m_wireframe2D_shader.setUniform("u_color", glm::vec4(1.0));
        m_wireframe2D_shader.setUniform("u_scale", 1.0f, 1.0f);
        m_wireframe2D_shader.setUniform("u_translate", (float)getMouseX(), (float)getMouseY());
        m_wireframe2D_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
        m_cross_vbo->render(&m_wireframe2D_shader);
        glLineWidth(1.0f);
    }
}

void Sandbox::renderDone() {
    // RECORD
    if (m_record) {
        onScreenshot(toString(m_record_counter, 0, 5, '0') + ".png");

        m_record_head += m_record_fdelta;
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

    if (m_histogram)
        onHistogram();


    unflagChange();

    if (!m_initialized) {
        m_initialized = true;
        updateViewport();
        flagChange();
    }
    else {
        m_frame++;
    }
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

void Sandbox::record(float _start, float _end, float fps) {
    m_record_fdelta = 1.0/fps;
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

    // IF the change is on a dependency file, re route to the correct shader that need to be reload
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
        if ( loadFromPath(filename, &m_frag_source, include_folders, &m_frag_dependencies) )
            reloadShaders(_files);
    }
    else if (type == VERT_SHADER) {
        m_vert_source = "";
        m_vert_dependencies.clear();
        if ( loadFromPath(filename, &m_vert_source, include_folders, &m_vert_dependencies) )
            reloadShaders(_files);
    }
    else if (type == GEOMETRY) {
        // TODO
    }
    else if (type == IMAGE) {
        for (TextureList::iterator it = uniforms.textures.begin(); it!=uniforms.textures.end(); it++) {
            if (filename == it->second->getFilePath()) {
                std::cout << filename << std::endl;
                it->second->load(filename, _files[index].vFlip);
                break;
            }
        }
    }
    else if (type == CUBEMAP) {
        if (uniforms.cubemap)
            uniforms.cubemap->load(filename, _files[index].vFlip);
    }

    flagChange();
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
    if (_button == 1) {
        // Left-button drag is used to pan u_view2d.
        m_view2d = glm::translate(m_view2d, -getMouseVelocity());

        // Left-button drag is used to rotate geometry.
        float dist = uniforms.getCamera().getDistance();

        float vel_x = getMouseVelX();
        float vel_y = getMouseVelY();

        if (fabs(vel_x) < 50.0 && fabs(vel_y) < 50.0) {
            m_lat -= vel_x;
            m_lon -= vel_y * 0.5;
            uniforms.getCamera().orbit(m_lat, m_lon, dist);
            uniforms.getCamera().lookAt(glm::vec3(0.0));
        }
    } 
    else {
        // Right-button drag is used to zoom geometry.
        float dist = uniforms.getCamera().getDistance();
        dist += (-.008f * getMouseVelY());
        if (dist > 0.0f) {
            uniforms.getCamera().orbit(m_lat, m_lon, dist);
        }
    }
}

void Sandbox::onViewportResize(int _newWidth, int _newHeight) {
    uniforms.getCamera().setViewport(_newWidth, _newHeight);
    
    for (unsigned int i = 0; i < uniforms.buffers.size(); i++) 
        uniforms.buffers[i].allocate(_newWidth, _newHeight, COLOR_TEXTURE);

    if (m_convolution_pyramid_fbos.size() > 0) {
        for (unsigned int i = 0; i < uniforms.convolution_pyramids.size(); i++) {
            m_convolution_pyramid_fbos[i].allocate(_newWidth, _newHeight, COLOR_TEXTURE);
            uniforms.convolution_pyramids[i].allocate(getWindowWidth(), getWindowHeight());
        }
    }

    if (m_postprocessing || m_histogram)
        _updateSceneBuffer(_newWidth, _newHeight);

    if (m_record || screenshotFile != "")
        m_record_fbo.allocate(_newWidth, _newHeight, COLOR_TEXTURE_DEPTH_BUFFER);

    flagChange();
}

void Sandbox::onScreenshot(std::string _file) {
    if (_file != "" && isGL()) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_record_fbo.getId());

        if (getExt(_file) == "hdr") {
            float* pixels = new float[getWindowWidth() * getWindowHeight()*4];
            glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_FLOAT, pixels);
            savePixelsHDR(_file, pixels, getWindowWidth(), getWindowHeight());
        }
        else {
            int width = getWindowWidth();
            int height = getWindowHeight();
            auto pixels = std::unique_ptr<unsigned char[]>(new unsigned char [width * height * 4]);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

            /** Just a small helper that captures all the relevant data to save an image **/
            class Job {
                std::string m_file_name;
                int m_width;
                int m_height;
                std::unique_ptr<unsigned char[]> m_pixels;
                std::atomic<int> * m_task_count;
                std::atomic<long long> * m_max_mem_in_queue;

                long mem_consumed_by_pixels() const { return m_width * m_height * 4; }
                /** the function that is being invoked when the task is done **/
                public:
                void operator()() {
                    if (m_pixels) {
                        savePixels(m_file_name, m_pixels.get(), m_width, m_height);
                        m_pixels = nullptr;
                        (*m_task_count)--;
                        (*m_max_mem_in_queue) += mem_consumed_by_pixels();
                    }
                }


                Job (const Job& ) = delete;
                Job (Job && ) = default;
                Job(std::string file_name, int width, int height, std::unique_ptr<unsigned char[]>&& pixels,
                        std::atomic<int>& task_count, std::atomic<long long>& max_mem_in_queue):
                    m_file_name(std::move(file_name)),
                    m_width(width),
                    m_height(height),
                    m_pixels(std::move(pixels)),
                    m_task_count(&task_count),
                    m_max_mem_in_queue(&max_mem_in_queue) {
                    if (m_pixels) {
                        task_count++;
                        max_mem_in_queue -= mem_consumed_by_pixels();
                    }
                }
            };

            std::shared_ptr<Job> saverPtr = std::make_shared<Job>(std::move(_file), width, height, std::move(pixels), m_task_count, m_max_mem_in_queue);
            /** In the case that we render faster than we can safe frames, more and more frames
			 * have to be stored temporary in the save queue. That means that more and more ram is used.
			 * If to much is memory is used, we save the current frame directly to prevent that the system
			 * is running out of memory. Otherwise we put the frame in to the thread queue, so that we can utilize
			 * multilple cpu cores */
            if (m_max_mem_in_queue <= 0) {
                Job& saver = *saverPtr;
                saver();
            }
            else {
                auto func = [saverPtr]()
                {
                    Job& saver = *saverPtr;
                    saver();
                };
                m_save_threads.Submit(std::move(func));
            }
        }
    
        if (!m_record) {
            std::cout << "// Screenshot saved to " << _file << std::endl;
            std::cout << "// > ";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Sandbox::onHistogram() {
    if ( isGL() && haveChange() ) {

        // Extract pixels
        glBindFramebuffer(GL_FRAMEBUFFER, m_scene_fbo.getId());
        int w = getWindowWidth();
        int h = getWindowHeight();
        int c = 4;
        int total = w * h * c;
        unsigned char* pixels = new unsigned char[total];
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Count frequencies of appearances 
        float max_rgb_freq = 0;
        float max_luma_freq = 0;
        glm::vec4 freqs[256];
        for (int i = 0; i < total; i += c) {
            freqs[pixels[i]].r++;
            if (freqs[pixels[i]].r > max_rgb_freq)
                max_rgb_freq = freqs[pixels[i]].r;

            freqs[pixels[i+1]].g++;
            if (freqs[pixels[i+1]].g > max_rgb_freq)
                max_rgb_freq = freqs[pixels[i+1]].g;

            freqs[pixels[i+2]].b++;
            if (freqs[pixels[i+2]].b > max_rgb_freq)
                max_rgb_freq = freqs[pixels[i+2]].b;

            int luma = 0.299 * pixels[i] + 0.587 * pixels[i+1] + 0.114 * pixels[i+2];
            freqs[luma].a++;
            if (freqs[luma].a > max_luma_freq)
                max_luma_freq = freqs[luma].a;
        }
        delete[] pixels;

        // Normalize frequencies
        for (int i = 0; i < 255; i ++)
            freqs[i] = freqs[i] / glm::vec4(max_rgb_freq, max_rgb_freq, max_rgb_freq, max_luma_freq);

        if (m_histogram_texture == nullptr)
            m_histogram_texture = new Texture();

        m_histogram_texture->load(256, 1, 4, 32, &freqs[0]);

        uniforms.textures["u_sceneHistogram"] = m_histogram_texture;
    }
}

