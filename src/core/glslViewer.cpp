#include "glslViewer.h"

#include <sys/stat.h>   // stat
#include <algorithm>    // std::find
#include <fstream>
#include <math.h>
#include <memory>

#include "tools/job.h"
#include "tools/text.h"
#include "tools/record.h"
#include "tools/console.h"

#include "vera/window.h"
#include "vera/ops/fs.h"
#include "vera/ops/draw.h"
#include "vera/ops/geom.h"
#include "vera/ops/math.h"
#include "vera/ops/image.h"
#include "vera/ops/pixel.h"
#include "vera/ops/meshes.h"
#include "vera/ops/string.h"
#include "vera/shaders/defaultShaders.h"
#include "vera/xr/holoPlay.h"

#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/rotate_vector.hpp"

#if defined(DEBUG)

#define TRACK_BEGIN(A) if (uniforms.tracker.isRunning()) uniforms.tracker.begin(A); 
#define TRACK_END(A) if (uniforms.tracker.isRunning()) uniforms.tracker.end(A); 

#else 

#define TRACK_BEGIN(A)
#define TRACK_END(A) 

#endif

// ------------------------------------------------------------------------- CONTRUCTOR
GlslViewer::GlslViewer(): 
    screenshotFile(""), lenticular(""), quilt_resolution(-1), quilt_tile(-1), 
    frag_index(-1), vert_index(-1), geom_index(-1), 
    verbose(false), cursor(true), help(false), fxaa(false),
    // Main Vert/Frag/Geom
    m_frag_source(""), m_vert_source(""),
    // Buffers
    m_buffers_total(0),
    m_doubleBuffers_total(0),
    m_pyramid_total(0),
    m_flood_total(0),
    // PostProcessing
    m_postprocessing(false),
    // Plot helpers
    m_plot(PLOT_OFF),

    // Record
    #if defined(SUPPORT_MULTITHREAD_RECORDING)
    m_task_count(0),
    /** allow 500 MB to be used for the image save queue **/
    m_max_mem_in_queue(500 * 1024 * 1024),
    m_save_threads(std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)),
    #endif

    // Scene
    m_view2d(1.0), m_time_offset(0.0), 
    m_camera_elevation(0.0), m_camera_azimuth(0.0), 
    m_error_screen(vera::SHOW_MAGENTA_SHADER), 
    m_change_viewport(true), m_update_buffers(true), m_initialized(false), 

    // Debug
    m_showTextures(false), m_showPasses(false)
{
    // set vera scene values to uniforms
    vera::scene( (vera::Scene*)&uniforms );

    // TIME UNIFORMS
    //
    uniforms.functions["u_frame"] = UniformFunction( "int", [&](vera::Shader& _shader) {
        if (isRecording()) _shader.setUniform("u_frame", getRecordingFrame());
        _shader.setUniform("u_frame", (int)uniforms.getFrame() );
    }, 
    [&]() { 
        if (isRecording()) return vera::toString( getRecordingFrame() );
        else return vera::toString(uniforms.getFrame(), 1); 
    } );

    uniforms.functions["u_play"] = UniformFunction( "int", [&](vera::Shader& _shader) {
        _shader.setUniform("u_play", uniforms.isPlaying()? 1 : 0);
    }, 
    [&]() { 
        return vera::toString( uniforms.isPlaying()? 1 : 0 );
    } );

    uniforms.functions["u_time"] = UniformFunction( "float", [&](vera::Shader& _shader) {
        if (vera::getWindowStyle() == vera::EMBEDDED) 
            _shader.setUniform("u_time", float(uniforms.getFrame()) * vera::getRestSec());
        else if (isRecording()) 
            _shader.setUniform("u_time", getRecordingTime());
        else 
            _shader.setUniform("u_time", float(vera::getTime()) - m_time_offset);
    }, 
    [&]() {  
        if (isRecording()) return vera::toString( getRecordingTime() );
        else return vera::toString(vera::getTime() - m_time_offset); 
    } );

    uniforms.functions["u_delta"] = UniformFunction("float", [&](vera::Shader& _shader) {
        if (isRecording()) _shader.setUniform("u_delta", getRecordingDelta());
        else _shader.setUniform("u_delta", float(vera::getDelta()));
    }, 
    [&]() { 
        if (isRecording()) return vera::toString( getRecordingDelta() );
        else return vera::toString(vera::getDelta());
    });

    uniforms.functions["u_date"] = UniformFunction("vec4", [](vera::Shader& _shader) {
        _shader.setUniform("u_date", vera::getDate());
    },
    []() { return vera::toString(vera::getDate().x, 0) + "," + vera::toString(vera::getDate().y, 0) + "," + vera::toString(vera::getDate().z, 0) + "," + vera::toString(vera::getDate().w, 2); });

    // MOUSE
    uniforms.functions["u_mouse"] = UniformFunction("vec2", [](vera::Shader& _shader) {
        _shader.setUniform("u_mouse", vera::getMousePositionFlipped());
    },
    []() { return vera::toString(vera::getMouseX(),1) + "," + vera::toString(vera::getMouseYFlipped(),1); } );

    // VIEWPORT
    uniforms.functions["u_pixelDensity"] = UniformFunction("float", [](vera::Shader& _shader) {
        _shader.setUniform("u_pixelDensity", vera::pixelDensity());
    },
    []() { return vera::toString(vera::getViewport().x,1) + "," + vera::toString(vera::getViewport().y,1) + "," + vera::toString(vera::getViewport().z,1) + "," + vera::toString(vera::getViewport().w,1); });
    uniforms.functions["u_resolution"]= UniformFunction("vec2", [](vera::Shader& _shader) {
        _shader.setUniform("u_resolution", float(vera::getWindowWidth()), float(vera::getWindowHeight()));
    },
    []() { return vera::toString((float)vera::getWindowWidth(),1) + "," + vera::toString((float)vera::getWindowHeight(),1); });

    uniforms.functions["u_resolutionChange"] = UniformFunction("bool", [this](vera::Shader& _shader) {
        _shader.setUniform("u_resolutionChange", m_change_viewport);
    },
    [this]() { 
        return vera::toString(m_change_viewport);
        return std::string("");
    });

    // SCENE
    uniforms.functions["u_view2d"] = UniformFunction("mat3", [this](vera::Shader& _shader) {
        _shader.setUniform("u_view2d", m_view2d);
    });


    uniforms.functions["u_modelViewProjectionMatrix"] = UniformFunction("mat4");
}

GlslViewer::~GlslViewer() {
    #if defined(SUPPORT_MULTITHREAD_RECORDING)
    /** make sure every frame is saved before exiting **/
    if (m_task_count > 0)
        std::cout << "saving remaining frames to disk, this might take a while ..." << std::endl;
    
    while (m_task_count > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    
    #endif
}

// ------------------------------------------------------------------------- SET

void GlslViewer::commandsInit(CommandList &_commands ) {

    // Add Sandbox Commands
    // ----------------------------------------
    _commands.push_back(Command("debug", [&](const std::string& _line){
        if (_line == "debug") {
            std::string rta = m_showPasses ? "on" : "off";
            std::cout << "buffers," << rta << std::endl; 
            rta = m_showTextures ? "on" : "off";
            std::cout << "textures," << rta << std::endl;
            if (uniforms.models.size() > 0) {
                rta = m_sceneRender.showGrid ? "on" : "off";
                std::cout << "grid," << rta << std::endl; 
                rta = m_sceneRender.showAxis ? "on" : "off";
                std::cout << "axis," << rta << std::endl; 
                rta = m_sceneRender.showBBoxes ? "on" : "off";
                std::cout << "bboxes," << rta << std::endl;
            }
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                m_showPasses = (values[1] == "on");
                m_showTextures = (values[1] == "on");
                console_uniforms( values[1] == "on" );
                // m_plot = (values[1] == "on")? 1 : 0;
                if (uniforms.models.size() > 0) {
                    m_sceneRender.showGrid = (values[1] == "on");
                    m_sceneRender.showAxis = (values[1] == "on");
                    m_sceneRender.showBBoxes = (values[1] == "on");
                    if (values[1] == "on")
                        addDefine("DEBUG", values[1]);
                    else
                        delDefine("DEBUG");
                }
                // if (values[1] == "on") {
                //     m_plot = PLOT_FPS;
                //     m_plot_shader.addDefine("PLOT_VALUE", "color.rgb += digits(uv * 0.1 + vec2(0.105, -0.01), value.r * 60.0, 1.0);");
                // }
                // else {
                //     m_plot = PLOT_OFF;
                //     m_plot_shader.delDefine("PLOT_VALUE");
                // }
                return true;
            }
        }
        return false;
    },
    "debug[,on|off]", "show/hide debug elements or return the status of them", false));

    _commands.push_back(Command("track", [&](const std::string& _line){
        if (_line == "track") {
            std::cout << "track," << (uniforms.tracker.isRunning() ? "on" : "off") << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {

                if ((values[1] == "on" || values[1] == "start" | values[1] == "begin") &&
                    !uniforms.tracker.isRunning() ) 
                    uniforms.tracker.start();
                else if (   (values[1] == "off" || values[1] == "stop" | values[1] == "end") &&
                            uniforms.tracker.isRunning() ) 
                    uniforms.tracker.stop();
                    
                else if (values[1] == "average") 
                    std::cout << uniforms.tracker.logAverage();

                else if (values[1] == "samples")
                    std::cout << uniforms.tracker.logSamples();

                else if (values[1] == "framerate")
                    std::cout << uniforms.tracker.logFramerate();
            }

            else if (values.size() == 3) {

                if (values[1] == "average" && 
                    vera::haveExt(values[2],"csv") ) {
                    std::ofstream out(values[2]);
                    out << uniforms.tracker.logAverage();
                    out.close();
                }

                else if (values[1] == "average")
                    std::cout << uniforms.tracker.logAverage( values[2] );

                else if (   values[1] == "samples" && 
                            vera::haveExt(values[2],"csv") ) {
                    std::ofstream out(values[2]);
                    out << "track,timeStampMs,durationMs\n";
                    out << uniforms.tracker.logSamples();
                    out.close();
                }

                else if (values[1] == "samples")
                    std::cout << uniforms.tracker.logSamples(values[2]);
                    
            }
            else if (values.size() == 4) {

                if (values[1] == "average" && 
                    vera::haveExt(values[3],"csv") ) {
                    std::ofstream out( values[3] );
                    out << uniforms.tracker.logAverage( values[2] );
                    out.close();
                }

                else if (   values[1] == "samples" && 
                    vera::haveExt(values[3],"csv") ) {
                    std::ofstream out( values[3] );
                    out << uniforms.tracker.logSamples( values[2] );
                    out.close();
                }

            }

        }
        return false;
    },
    "track[,on|off|average|samples]", "start/stop tracking rendering time", false));

    _commands.push_back(Command("glsl_version", [&](const std::string& _line){ 
        if (_line == "glsl_version") {
            // Force the output in floats
            std::cout << vera::getVersionNumber() << std::endl;
            return true;
        }
        return false;
    },
    "glsl_version", "return GLSL Version", false));

    _commands.push_back(Command("error_screen", [&](const std::string& _line){ 
        if (_line == "error_screen") {
            std::string rta = (m_error_screen == vera::SHOW_MAGENTA_SHADER) ? "on" : "off";
            std::cout << "error_screen," << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                m_error_screen = (values[1] == "on")? vera::SHOW_MAGENTA_SHADER : vera::REVERT_TO_PREVIOUS_SHADER;
                return true;
            }
        }
        return false;
    },
    "error_screen,on|off", "enable/disable magenta screen on errors", false));

    _commands.push_back(Command("plot", [&](const std::string& _line){
        if (_line == "plot") {
            std::cout << plot_options[m_plot] << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values[0] == "plot" && values.size() == 2) {
                m_plot_shader.setSource(vera::getDefaultSrc(vera::FRAG_PLOT), vera::getDefaultSrc(vera::VERT_DYNAMIC_BILLBOARD));
                m_plot_shader.delDefine("PLOT_VALUE");

                using plot_value_display_t = std::tuple<std::string, PlotType, std::string>;
                const std::vector<plot_value_display_t> plot_metadata = {
                    {"off", PLOT_OFF, ""}
                    , {"luma", PLOT_LUMA, "color.rgb = vec3(step(st.y, data.a)); color += stroke(fract(st.x * 5.0), 0.5, 0.025) * 0.1;"}
                    , {"red", PLOT_RED, "color.rgb = vec3(step(st.y, data.r), 0.0, 0.0);  color += stroke(fract(st.x * 5.0), 0.5, 0.025) * 0.1;"}
                    , {"green", PLOT_GREEN, "color.rgb = vec3(0.0, step(st.y, data.g), 0.0);  color += stroke(fract(st.x * 5.0), 0.5, 0.025) * 0.1;"}
                    , {"blue", PLOT_BLUE, "color.rgb = vec3(0.0, 0.0, step(st.y, data.b));  color += stroke(fract(st.x * 5.0), 0.5, 0.025) * 0.1;"}
                    , {"rgb", PLOT_RGB, "color += stroke(fract(st.x * 5.0), 0.5, 0.025) * 0.1;"}
                    , {"fps", PLOT_FPS,"color.rgb += digits(uv * 0.1 + vec2(0.0, -0.01), value.r * 60.0, 1.0); color += stroke(fract(st.y * 3.0), 0.5, 0.05) * 0.1;"}
                    , {"ms", PLOT_MS, "color.rgb += digits(uv * 0.1 + vec2(0.105, -0.01), value.r * 60.0, 1.0); color += stroke(fract(st.y * 3.0), 0.5, 0.05) * 0.1;"}
                };

                if (values[1] == "toggle") {
                    auto carousel = std::vector<PlotType>{PLOT_OFF, PLOT_LUMA, PLOT_RGB, PLOT_FPS};
                    const auto it = std::find_if(std::begin(carousel), std::end(carousel), [&](const PlotType& s){return m_plot == s;});
                    if(it != std::end(carousel)) {
                        std::rotate(std::begin(carousel), std::next(it), std::end(carousel));
                        const auto it = std::find_if(std::begin(plot_metadata), std::end(plot_metadata), [&](const plot_value_display_t& s){return carousel.front() == std::get<1>(s);});
                        values[1] = std::get<0>(*it);
                    }
                }

                if (values[1] == "off") 
                    m_plot = PLOT_OFF;
                else {
                    const auto it = std::find_if(std::begin(plot_metadata), std::end(plot_metadata), [&](const plot_value_display_t& i){return std::get<0>(i) == values[1];});
                    if(it != std::end(plot_metadata)) {
                        m_plot = std::get<1>(*it);
                        m_plot_shader.addDefine("PLOT_VALUE", std::get<2>(*it));
                    }
                }
                m_update_buffers = true;
                return true;
            }
        }
        return false;
    },
    "plot[,off|luma|red|green|blue|rgb|fps|ms]", "show/hide a histogram or FPS plot on screen", false));

    _commands.push_back(Command("reset", [&](const std::string& _line){
        if (_line == "reset") {
            m_time_offset = vera::getTime();
            return true;
        }
        return false;
    },
    "reset", "reset timestamp back to zero", false));

    _commands.push_back(Command("time", [&](const std::string& _line){ 
        if (_line == "time") {
            // Force the output in floats
            std::cout << std::setprecision(6) << (vera::getTime() - m_time_offset) << std::endl;
            return true;
        }
        return false;
    },
    "time", "return u_time, the elapsed time.", false));

    _commands.push_back(Command("defines", [&](const std::string& _line){ 
        if (_line == "defines") {
            if (uniforms.models.size() > 0) {
                m_sceneRender.printDefines();
                uniforms.printDefines();
            }
            else
                m_canvas_shader.printDefines();
            
            return true;
        }
        return false;
    },
    "defines", "return a list of active defines", false));
    
    _commands.push_back(Command("uniforms", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');

        if (values[0] != "uniforms")
            return false;

        if (values.size() == 1) {
            uniforms.printAvailableUniforms(false);
            uniforms.printDefinedUniforms();
            return true;
        }
        else if (values[1] == "all") {
            uniforms.printAvailableUniforms(true);
            uniforms.printDefinedUniforms();
            uniforms.printBuffers();
            uniforms.printTextures();
            uniforms.printStreams();
            uniforms.printCubemaps();
            uniforms.printLights();
            uniforms.printCameras();
            m_sceneRender.printBuffers();
            return true;
        }
        else if (values[1] == "active") {
            uniforms.printAvailableUniforms(false);
            uniforms.printDefinedUniforms();
            return true;
        }
        else if (values[1] == "defined") {
            uniforms.printDefinedUniforms(true);
            return true;
        }
        else if (values[1] == "textures") {
            uniforms.printTextures();
            uniforms.printBuffers();
            m_sceneRender.printBuffers();
            uniforms.printStreams();
            return true;
        }
        else if (values[1] == "buffers") {
            uniforms.printBuffers();
            m_sceneRender.printBuffers();
            return true;
        }
        else if (values[1] == "streams") {
            uniforms.printStreams();
            return true;
        }
        else if (values[1] == "cubemaps") {
            uniforms.printCubemaps();
            return true;
        }
        else if (values[1] == "cameras") {
            uniforms.printCameras();
            return true;
        }
        else if (values[1] == "lights") {
            uniforms.printLights();
            return true;
        }
        else if (values[1] == "on" || values[1] == "off") { 
            console_uniforms( values[1] == "on" );
            return true; 
        }

        return false;
    },
    "uniforms[,all|active|defined|textures|buffers|cubemaps|lights|cameras|on|off]", "return a list of uniforms", false));

    _commands.push_back(Command("textures", [&](const std::string& _line){ 
        if (_line == "textures") {
            std::cout << (m_showTextures ? "on" : "off") << std::endl;
            return true;
        }
        else if (_line == "textures,list") {
            uniforms.printTextures();
            uniforms.printStreams();
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                if (values[1] == "toggle")
                    values[1] = m_showTextures ? "off" : "on";

                m_showTextures = (values[1] == "on");
                return true;
            }
        }
        return false;
    },
    "textures[,on|off|list]", "Show/hide textures on viewport, or return the list of textures", false));

    _commands.push_back(Command("buffer", [&](const std::string& _line){ 
        if (_line == "buffers") {
            std::cout << (m_showPasses ? "on" : "off") << std::endl;
            return true;
        }
        else if (_line == "buffers,list") {
            uniforms.printBuffers();
            m_sceneRender.printBuffers();
            if (m_postprocessing) {
                if (lenticular.size() > 0)
                    std::cout << "LENTICULAR";
                else if (fxaa)
                    std::cout << "FXAA";
                else
                    std::cout << "Custom";
                std::cout << " postProcessing pass" << std::endl;
                return true;
            }
            
            return false;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');

            if (values.size() == 2) {
                if (values[1] == "toggle")
                    values[1] = m_showPasses ? "off" : "on";

                m_showPasses = (values[1] == "on" || values[1] == "show");
                return true;
            }
            else if (values.size() == 3) {
                size_t i = vera::toInt(values[1]);
                if (i < uniforms.buffers.size())
                    uniforms.buffers[i]->enabled = (values[1] == "on");
            }
        }
        return false;
    },
    "buffers[,show|hide|list]", "Show/hide buffer on viewport, or return the list of buffers", false));

    // CUBEMAPS
    _commands.push_back(Command("cubemaps", [&](const std::string& _line){
        if (_line == "cubemaps") {
            uniforms.printCubemaps();
            return true;
        }
        return false;
    },
    "cubemaps", "print all cubemaps"));

    _commands.push_back(Command("cubemap", [&](const std::string& _line){
        if (_line == "cubemap") {
            std::string rta = m_sceneRender.showCubebox ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                if (values[1] == "sh") {
                    uniforms.printCubemapSH();
                    return true;
                }
                if (values[1] == "toggle")
                    values[1] = m_sceneRender.showCubebox ? "off" : "on";

                m_sceneRender.showCubebox = values[1] == "on";
                if (values[1] == "on") {
                    addDefine("SCENE_SH_ARRAY", "u_SH");
                    addDefine("SCENE_CUBEMAP", "u_cubeMap");
                }
                else if (values[1] == "off") {
                    delDefine("SCENE_SH_ARRAY");
                    delDefine("SCENE_CUBEMAP");
                }
                return true;
            }
        }
        return false;
    },
    "cubemap[,on|off|toggle|sh]", "show/hide cubemap or return the spherical harmonics of it"));

    _commands.push_back(Command("sky", [&](const std::string& _line){
        if (_line == "sky") {
            std::string rta = (m_sceneRender.showCubebox && uniforms.activeCubemap == uniforms.cubemaps["default"]) ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                if (values[1] == "toggle")
                    values[1] = m_sceneRender.showCubebox ? "off" : "on";
                    // values[1] = (m_sceneRender.showCubebox && uniforms.activeCubemap == uniforms.cubemaps["default"]) ? "off" : "on";

                m_sceneRender.showCubebox = values[1] == "on";
                if (values[1] == "on") {
                    uniforms.activeCubemap = uniforms.cubemaps["default"];
                    addDefine("SCENE_SH_ARRAY", "u_SH");
                    addDefine("SCENE_CUBEMAP", "u_cubeMap");
                }
                else if (values[1] == "off") {
                    delDefine("SCENE_SH_ARRAY");
                    delDefine("SCENE_CUBEMAP");
                }
                return true;
            }
        }
        return false;
    },
    "sky[,on|off]", "show/hide skybox"));

    _commands.push_back(Command("sun_elevation", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {

            float elevation = glm::radians( vera::toFloat(values[1]) );
            float azimuth = uniforms.getSunAzimuth();

            addDefine("SCENE_SH_ARRAY", "u_SH");
            addDefine("SCENE_CUBEMAP", "u_cubeMap");
            addDefine("SUN", "u_light");
            uniforms.setSunPosition(azimuth, elevation, glm::length( uniforms.lights["default"]->getPosition() ));
            uniforms.activeCubemap = uniforms.cubemaps["default"];
            return true;
        }
        else {
            std::cout << uniforms.getSunElevation() << std::endl;
            return true;
        }
        return false;
    },
    "sun_elevation[,<degrees>]", "get or set the sun elevation in degrees (remember to skybox,on)."));

    _commands.push_back(Command("sun_azimuth", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {

            float elevation = uniforms.getSunElevation();
            float azimuth = glm::radians( vera::toFloat(values[1]) );

            addDefine("SUN", "u_light");
            addDefine("SCENE_SH_ARRAY", "u_SH");
            addDefine("SCENE_CUBEMAP", "u_cubeMap");
            uniforms.setSunPosition(azimuth, elevation, glm::length( uniforms.lights["default"]->getPosition() ) );
            uniforms.activeCubemap = uniforms.cubemaps["default"];

            return true;
        }
        else {
            std::cout << uniforms.getSunAzimuth() << std::endl;
            return true;
        }
        return false;
    },
    "sun_azimuth[,<degrees>]", "get or set the sun azimuth in degrees (remember to skybox,on)."));

    _commands.push_back(Command("sky_turbidity", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            uniforms.setSkyTurbidity( vera::toFloat(values[1]) );
            uniforms.activeCubemap = uniforms.cubemaps["default"];
            return true;
        }
        else {
            std::cout << uniforms.getSkyTurbidity() << std::endl;
            return true;
        }
        return false;
    },
    "sky_turbidity[,<sky_turbidty>]", "get or set the sky turbidity."));

    // LIGTH
    _commands.push_back(Command("lights", [&](const std::string& _line){ 
        if (_line == "lights") {
            uniforms.printLights();
            return true;
        }
        return false;
    },
    "lights", "print all light related uniforms"));

    _commands.push_back(Command("light_position", [&](const std::string& _line){ 
        
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 4) {
            if (uniforms.lights.size() == 1)
                uniforms.setSunPosition( glm::vec3(vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3])) );
            
            return true;
        }
        // else if (values.size() == 5) {
        //     size_t i = vera::toInt(values[1]);
        //     if (uniforms.lights.size() > i)
        //         uniforms.lights[i].setPosition(glm::vec3(vera::toFloat(values[2]), vera::toFloat(values[3]), vera::toFloat(values[4])));

        //     return true;
        // }
        else {
            glm::vec3 pos = uniforms.lights["default"]->getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "light_position[[,<index>],<x>,<y>,<z>]", "get or set the light position"));

    _commands.push_back(Command("light_color", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 4) {
            vera::Light* sun = uniforms.lights["default"];
            sun->color = glm::vec3(vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3]));
            sun->bChange = true;
            return true;
        }
        // else if (values.size() == 5) {
        //     size_t i = vera::toInt(values[1]);
        //     if (uniforms.lights.size() > i) {
        //         uniforms.lights[i].color = glm::vec3(vera::toFloat(values[2]), vera::toFloat(values[3]), vera::toFloat(values[4]));
        //         uniforms.lights[i].bChange = true;
        //     }
        //     return true;
        // }
        else {
            glm::vec3 color = uniforms.lights["default"]->color;
            std::cout << color.x << ',' << color.y << ',' << color.z << std::endl;
            return true;
        }
        return false;
    },
    "light_color[,<r>,<g>,<b>]", "get or set the light color"));

    _commands.push_back(Command("light_falloff", [&](const std::string& _line){ 
         std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            vera::Light* sun = uniforms.lights["default"];
            sun->falloff = vera::toFloat(values[1]);
            sun->bChange = true;
            return true;
        }
        // else if (values.size() == 5) {
        //     size_t i = vera::toInt(values[1]);
        //     if (uniforms.lights.size() > i) {
        //         uniforms.lights[i].falloff = vera::toFloat(values[2]);
        //         uniforms.lights[i].bChange = true;
        //     }
        //     return true;
        // }
        else {
            std::cout << uniforms.lights["default"]->falloff << std::endl;
            return true;
        }
        return false;
    },
    "light_falloff[,<value>]", "get or set the light falloff distance"));

    _commands.push_back(Command("light_intensity", [&](const std::string& _line){ 
         std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            vera::Light* sun = uniforms.lights["default"];
            sun->intensity = vera::toFloat(values[1]);
            sun->bChange = true;
            return true;
        }
        // else if (values.size() == 5) {
        //     size_t i = vera::toInt(values[1]);
        //     if (uniforms.lights.size() > i) {
        //         uniforms.lights[i].intensity = vera::toFloat(values[2]);
        //         uniforms.lights[i].bChange = true;
        //     }
        //     return true;
        // }
        else {
            std::cout <<  uniforms.lights["default"]->intensity << std::endl;
            return true;
        }
        return false;
    },
    "light_intensity[,<value>]", "get or set the light intensity"));

    // CAMERA
    _commands.push_back(Command("camera_distance", [&](const std::string& _line){ 
        if (!uniforms.activeCamera) 
            return false;

        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            if (uniforms.activeCamera != uniforms.cameras["default"]) {
                uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
                uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
                
                uniforms.activeCamera = uniforms.cameras["default"];
            }
            float dist = vera::toFloat(values[1]);
            uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, dist);

            return true;
        }
        else {
            
            std::cout << uniforms.activeCamera->getDistance() << std::endl;
            return true;
        }
        return false;
    },
    "camera_distance[,<dist>]", "get or set the camera distance to the target"));

    _commands.push_back(Command("camera_type", [&](const std::string& _line){ 
        if (!uniforms.activeCamera) 
            return false;

        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            if (uniforms.activeCamera != uniforms.cameras["default"]) {
                uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
                uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
                
                uniforms.activeCamera = uniforms.cameras["default"];
            }
            if (values[1] == "ortho")
                uniforms.activeCamera->setProjection(vera::ProjectionType::ORTHO);
            else if (values[1] == "perspective")
                uniforms.activeCamera->setProjection(vera::ProjectionType::PERSPECTIVE);
            return true;
        }
        else {
            vera::ProjectionType type = uniforms.activeCamera->getProjectionType();
            if (type == vera::ProjectionType::ORTHO)
                std::cout << "ortho" << std::endl;
            else
                std::cout << "perspective" << std::endl;
            
            return true;
        }
        return false;
    },
    "camera_type[,<ortho|perspective>]", "get or set the camera type"));

    _commands.push_back(Command("camera_fov", [&](const std::string& _line){ 
        if (!uniforms.activeCamera) 
            return false;

        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 2) {
            uniforms.activeCamera->setFOV( glm::radians(vera::toFloat(values[1])) );
            return true;
        }
        else {
            std::cout << glm::degrees(uniforms.activeCamera->getFOV()) << std::endl;
            return true;
        }
        return false;
    },
    "camera_fov[,<field_of_view>]", "get or set the camera field of view."));

    _commands.push_back(Command("camera_position", [&](const std::string& _line) {
        if (!uniforms.activeCamera) 
            return false;

        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 4) {
            glm::vec3 target = uniforms.activeCamera->getTarget();
            if (uniforms.activeCamera != uniforms.cameras["default"]) {
                uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
                uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
                
                uniforms.activeCamera = uniforms.cameras["default"];
            }
            uniforms.activeCamera->setPosition( -glm::vec3(vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3])));
            glm::vec3 v = uniforms.activeCamera->getPosition() - target;
            m_camera_azimuth = glm::degrees( atan2(v.x, v.z) );
            m_camera_elevation = glm::degrees( atan2(-v.y, sqrt(v.x * v.x + v.z * v.z)) );

            return true;
        }
        else {
            glm::vec3 pos = -uniforms.activeCamera->getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "camera_position[,<x>,<y>,<z>]", "get or set the camera position."));


    _commands.push_back(Command("camera_move", [&](const std::string& _line) {
        if (!uniforms.activeCamera) 
            return false;

        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 4) {
            if (uniforms.activeCamera != uniforms.cameras["default"]) {
                uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
                uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
                uniforms.activeCamera = uniforms.cameras["default"];
            }
            uniforms.activeCamera->move(vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3]));
            return true;
        }
        return false;
    },
    "camera_move,<x>,<y>,<z>", "move camera camera position."));


    _commands.push_back(Command("camera_look_at", [&](const std::string& _line) {
        if (!uniforms.activeCamera) 
            return false;

        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 4) {
            glm::vec3 position = uniforms.activeCamera->getPosition();
            if (uniforms.activeCamera != uniforms.cameras["default"]) {
                uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
                uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
                uniforms.activeCamera = uniforms.cameras["default"];
            }
            uniforms.activeCamera->setTarget( glm::vec3(vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3])) );
            glm::vec3 v = uniforms.activeCamera->getPosition() - uniforms.activeCamera->getTarget();
            m_camera_azimuth = glm::degrees( atan2(v.x, v.z) );
            m_camera_elevation = glm::degrees( atan2(-v.y, sqrt(v.x * v.x + v.z * v.z)) );

            return true;
        }
        else {
            glm::vec3 target = uniforms.activeCamera->getTarget();
            std::cout << target.x << ',' << target.y << ',' << target.z << std::endl;
            return true;
        }
        return false;
    },
    "camera_look_at[,<x>,<y>,<z>]", "point the camera towards the direction of a particular position."));

    _commands.push_back(Command("camera_exposure", [&](const std::string& _line) { 
        if (!uniforms.activeCamera) 
            return false;

        std::vector<std::string> values = vera::split(_line,',');
        if (values.size() == 4) {
            uniforms.activeCamera->setExposure( vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3]));
            return true;
        }
        else {
            std::cout << uniforms.activeCamera->getExposure() << std::endl;
            return true;
        }
        return false;
    },
    "camera_exposure[,<aper.>,<shutter>,<sensit.>]", "get or set the camera exposure values."));

    _commands.push_back(Command("camera", [&](const std::string& _line){ 
        if (_line == "camera") {
            // TODO: print active camera name
            return false;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                if (values[1] == "list") {
                    uniforms.printCameras();
                    return true;
                }
                else if (values[1] == "default") {
                    if (uniforms.cameras.find("default") != uniforms.cameras.end()) {
                        uniforms.activeCamera = uniforms.cameras["default"];
                        return true;
                    }
                }
                else if (uniforms.cameras.find(values[1]) != uniforms.cameras.end()) {
                    glm::vec3 target = uniforms.activeCamera->getTarget();

                    uniforms.activeCamera = uniforms.cameras[ values[1] ];
                    uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
                    uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
                    uniforms.cameras["default"]->bFlipped = uniforms.activeCamera->bFlipped;

                    // convert camera_transform to vector<float>
                    std::vector<float> camera_transform(16);
                    const glm::mat4& tm = uniforms.activeCamera->getTransformMatrix();
                    for (int i = 0; i < 4; i++)
                        for (int j = 0; j < 4; j++)
                            camera_transform[i * 4 + j] = tm[j][i];
                    uniforms.set("u_cameraTransformMatrix", camera_transform);

                    std::vector<float> camera_projection(16);
                    const glm::mat4& pm = uniforms.activeCamera->getProjectionMatrix();
                    for (int i = 0; i < 4; i++)
                        for (int j = 0; j < 4; j++)
                            camera_projection[i * 4 + j] = pm[j][i];
                    uniforms.set("u_cameraProjectionMatrix", camera_projection);

                    // extract camera m_camera_azimuth and m_camera_elevation
                    glm::vec3 v = uniforms.activeCamera->getPosition() - target;
                    m_camera_azimuth = glm::degrees( atan2(v.x, v.z) ) - 180.0f;
                    m_camera_elevation = glm::degrees( atan2(-v.y, sqrt(v.x * v.x + v.z * v.z)) );
                    
                    std::string camera_texture_name = "_camera" + values[1];
                    if (uniforms.textures.find(camera_texture_name) != uniforms.textures.end()) {
                        uniforms.textures["u_cameraTex"] = uniforms.textures[camera_texture_name];
                    }

                    return true;
                }
            }
        }
        return false;
    },
    "camera[,<name>|default|none|list]", "get or set the active camera."));

    _commands.push_back(Command("stream", [&](const std::string& _line) { 
        std::vector<std::string> values = vera::split(_line,',');

        if (values.size() == 3) {
            if ( values[2] == "play") {
                uniforms.setStreamPlay( values[1] );
                return true;
            }
            else if ( values[2] == "stop") {
                uniforms.setStreamStop( values[1] );
                return true;
            }
            else if ( values[2] == "restart") {
                uniforms.setStreamRestart( values[1] );
                return true;
            }
            else if ( values[2] == "speed") {
                std::cout << uniforms.getStreamSpeed( values[1] ) << std::endl;
                return true;
            }
            else if ( values[2] == "time") {
                std::cout << uniforms.getStreamTime( values[1] ) << std::endl;
                return true;
            }
            else if ( values[2] == "pct") {
                std::cout << uniforms.getStreamPct( values[1] ) << std::endl;
                return true;
            }
        }
        else if (values.size() == 4) {
            if ( values[2] == "speed") {
                uniforms.setStreamSpeed( values[1], vera::toFloat(values[3]) );
                return true;
            }
            else if ( values[2] == "time") {
                uniforms.setStreamTime( values[1], vera::toFloat(values[3]) );
                return true;
            }
            else if ( values[2] == "pct") {
                uniforms.setStreamPct( values[1], vera::toFloat(values[3]) );
                return true;
            }
        }

        return false;
    },
    "stream,<uniform_name>,stop|play|speed|time[,<value>]", "play/stop or change speed or time of a specific stream"));

    _commands.push_back(Command("streams", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');

        if (_line == "streams")
            uniforms.printStreams();

        else if (values.size() == 2) {
            if ( values[1] == "stop") {
                uniforms.setStreamsStop();
                return true;
            }
            else if ( values[1] == "play") {
                uniforms.setStreamsPlay();
                return true;
            }
            else if ( values[1] == "restart") {
                uniforms.setStreamsRestart();
                return true;
            }
        }
        else if (values.size() == 3) {
            if ( values[1] == "speed") {
                uniforms.setStreamsSpeed( vera::toFloat(values[2]) );
                return true;
            }
            else if ( values[1] == "time") {
                uniforms.setStreamsTime( vera::toFloat(values[2]) );
                return true;
            }
            else if ( values[1] == "pct") {
                uniforms.setStreamsPct( vera::toFloat(values[2]) );
                return true;
            }
            else if ( values[1] == "frame") {
                uniforms.setStreamsStop();
                int frame = vera::toInt(values[2]);
                uniforms.setStreamsFrame( frame );
                addDefine("STREAMS_FRAME", values[2]);
                return true;
            }
            else if ( values[1] == "prevs") {
                int prevs = vera::toInt(values[2]);

                if (prevs == 0) {
                    uniforms.setStreamsPrevs( 0 );
                    delDefine("STREAMS_PREVS");
                }
                else {
                    uniforms.setStreamsPrevs( vera::toInt(values[2]) );
                    addDefine("STREAMS_PREVS", values[2]);
                }
                return true;
            }
        }

        return false;
    },
    "streams[,stop|play|restart|frame|speed|prevs[,<value>]]", "print all streams or get/set streams speed and previous frames"));

    _commands.push_back(Command("models", [&](const std::string& _line){ 
        std::vector<std::string> values = vera::split(_line,',');

        if (_line == "models") {
            uniforms.printModels();
            return true;
        }
        else if (values.size() == 2) {
            if ( values[1] == "clear") {
                uniforms.clearModels();
                m_sceneRender.clearScene();
                return true;
            }
        }
        
        return false;
    },
    "models[,clear]", "print all or clear all loaded models."));

    _commands.push_back(Command("generate_sdf", [&](const std::string& _line) { 
        if (geom_index != -1) {
            std::vector<std::string> values = vera::split(_line,',');
            float padding = 0.01f;
            if (values.size() > 1)
                padding = vera::toFloat(values[1]);

            for (vera::ModelsMap::iterator it = uniforms.models.begin(); it != uniforms.models.end(); ++it) {
                std::string name = "u_" + it->first + "Sdf";
                addDefine("MODEL_SDF_TEXTURE", name );

                clock_t start, end;
                start = clock();

                vera::Mesh mesh = it->second->mesh;
                mesh.setMaterial(it->second->mesh.getMaterial());
                vera::center(mesh);
                
                std::vector<vera::Triangle> tris = mesh.getTriangles();
                vera::BVH acc(tris, vera::SPLIT_MIDPOINT );
                acc.square();

                // // Calculate scale difference between model and SDF
                // vera::BoundingBox bbox  = it->second->getBoundingBox();
                // float area = bbox.getArea();
                // glm::vec3   bdiagonal   = bbox.getDiagonal();
                // float       max_dist    = glm::length(bdiagonal);
                // bbox.expand( (max_dist*max_dist) * padding );
                // it->second->addDefine("MODEL_SDF_SCALE", vera::toString(2.0f-bbox.getArea()/area) );

                int voxel_resolution_lod_0  = std::pow(2, 2);
                glm::vec3   bdiagonal       = acc.getDiagonal();
                float       max_dist        = glm::length(bdiagonal);
                acc.expand( (max_dist*max_dist) * padding );

                std::vector<vera::Image> current_lod;
                for (int z = 0; z < voxel_resolution_lod_0; z++)
                    current_lod.push_back( vera::toSdfLayer( &acc, voxel_resolution_lod_0, z) );
                
                uniforms.loadMutex.lock();
                uniforms.loadQueue[ name ] = packSprite(current_lod);
                uniforms.loadMutex.unlock();
                addDefine("MODEL_SDF_VOXEL_RESOLUTION", vera::toString(voxel_resolution_lod_0) + ".0" );
                addDefine("MODEL_SDF_TEXTURE_RESOLUTION", "vec2(" + vera::toString( uniforms.loadQueue[ name ].getWidth() ) + ".0)" );

                size_t lod_total = 4;
                for (int l = 1; l < lod_total; l++) {
                    std::vector<vera::Image> new_lod = vera::scaleSprite(current_lod, 4);
                    uniforms.loadMutex.lock();
                    uniforms.loadQueue[ name ] = vera::packSprite(new_lod);
                    uniforms.loadMutex.unlock();
                    addDefine("MODEL_SDF_VOXEL_RESOLUTION", vera::toString(new_lod.size()) + ".0" );
                    addDefine("MODEL_SDF_TEXTURE_RESOLUTION", "vec2(" + vera::toString( uniforms.loadQueue[ name ].getWidth() ) + ".0)" );

                    if (l < (lod_total-1)) {
                        for (int z = 0; z < new_lod.size(); z++) {
                            new_lod[z] = vera::toSdfLayer( &acc, new_lod.size(), z);
                            console_draw_pct( float(z) / float(new_lod.size() - 1) );
                            uniforms.loadMutex.lock();
                            uniforms.loadQueue[ name ] = vera::packSprite(new_lod);
                            uniforms.loadMutex.unlock();
                        }
                    }
                    
                    current_lod = new_lod;
                }

                end = clock();
                double duration_sec = double(end-start)/CLOCKS_PER_SEC;
                std::cout << "Took " << duration_sec << "secs" << std::endl;
            }

            return true;
        }
        return false;
    },
    "generate_sdf[,padding[,resolution]]", "create an 3D SDF texture of loaded models, default padding = 0.01, resolution = 6"));


    #if defined(SUPPORT_MULTITHREAD_RECORDING)
    _commands.push_back(Command("max_mem_in_queue", [&](const std::string & line) {
        std::vector<std::string> values = vera::split(line,',');
        if (values.size() == 2) {
            m_max_mem_in_queue = std::stoll(values[1]);
        }
        else {
            std::cout << m_max_mem_in_queue.load() << std::endl;
        }
        return false;
    }, "max_mem_in_queue[,<bytes>]", "set the maximum amount of memory used by a queue to export images to disk"));
    #endif

    if (vert_index != -1 || geom_index != -1) {
        m_sceneRender.commandsInit(_commands, uniforms);
        m_sceneRender.uniformsInit(uniforms);
    }
}

void GlslViewer::loadAssets(WatchFileList &_files) {
    // LOAD SHACER 
    // -----------------------------------------------
    if (frag_index != -1) {
        // If there is a Fragment shader load it
        m_frag_source = "";
        m_frag_dependencies.clear();

        if ( !vera::loadGlslFrom(_files[frag_index].path, &m_frag_source, include_folders, &m_frag_dependencies) )
            return;

        vera::setVersionFromCode(m_frag_source);
    }
    else {
        // If there is no use the default one
        if (geom_index == -1)
            m_frag_source = vera::getDefaultSrc(vera::FRAG_DEFAULT);
        else {
            std::string defaultFrag = vera::getDefaultSrc(vera::FRAG_DEFAULT_SCENE);
            m_frag_source = vera::resolveGlsl(defaultFrag, include_folders, &m_frag_dependencies);
            // there is no need to track default dependencies because they won't change, also may come from memory and not from file
            m_frag_dependencies.clear();
        }
    }

    if (vert_index != -1) {
        // If there is a Vertex shader load it
        m_vert_source = "";
        m_vert_dependencies.clear();
        vera::loadGlslFrom(_files[vert_index].path, &m_vert_source, include_folders, &m_vert_dependencies);
    }
    else {
        // If there is no use the default one
        if (geom_index == -1) {
            m_vert_source = vera::getDefaultSrc(vera::VERT_DEFAULT);
        }
        else {
            std::string defaultVert = vera::getDefaultSrc(vera::VERT_DEFAULT_SCENE);
            m_vert_source = vera::resolveGlsl(defaultVert, include_folders, &m_vert_dependencies);
            // there is no need to track default dependencies because they won't change, also may come from memory and not from file
            m_vert_dependencies.clear();
        }
    }

    // purge all dependecies that are not present in the file system
    for (int i = 0; i < m_frag_dependencies.size(); i++) {
        if (!vera::urlExists( m_frag_dependencies[i] )) {
            m_frag_dependencies.erase( m_frag_dependencies.begin() + i);
        }
    }
    
    for (int i = 0; i < m_vert_dependencies.size(); i++) {  
        if (!vera::urlExists( m_vert_dependencies[i] )) {
            m_vert_dependencies.erase( m_vert_dependencies.begin() + i);
        }
    }

    // LOAD GEOMETRY
    // -----------------------------------------------
    if (geom_index != -1) {
        uniforms.load(_files[geom_index].path, verbose);
        m_sceneRender.loadScene(uniforms);
        uniforms.activeCamera->setTarget(m_sceneRender.getCenter());
        
        float dist = m_sceneRender.getArea() * 2.0; 
        if (vera::getExt(_files[geom_index].path) == "splat")
            dist = m_sceneRender.getArea() * 1.5;

        uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, dist);
    }
    else {
        m_canvas_shader.addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");
        uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, 2.0);
    }

    // FINISH SCENE SETUP
    // -------------------------------------------------
    uniforms.activeCamera->setViewport(vera::getWindowWidth(), vera::getWindowHeight());
    
    if (lenticular.size() > 0)
        vera::setLenticularProperties(lenticular);

    if (quilt_resolution >= 0) {
        vera::setQuiltProperties(quilt_resolution);
        addDefine("QUILT", vera::toString(quilt_resolution));
        addDefine("QUILT_WIDTH", vera::toString( vera::getQuiltWidth() ));
        addDefine("QUILT_HEIGHT", vera::toString( vera::getQuiltHeight() ));
        addDefine("QUILT_COLUMNS", vera::toString( vera::getQuiltColumns() ));
        addDefine("QUILT_ROWS", vera::toString( vera::getQuiltRows() ));
        addDefine("QUILT_TOTALVIEWS", vera::toString( vera::getQuiltTotalViews() ));

        uniforms.activeCamera->setFOV(glm::radians(14.0f));
        uniforms.activeCamera->setProjection(vera::ProjectionType::PERSPECTIVE_VIRTUAL_OFFSET);
        // uniforms.activeCamera->setClipping(0.01, 100.0);

        if (geom_index != -1)
            uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, m_sceneRender.getArea() * 8.5);

        if (lenticular.size() == 0) {
            if (quilt_tile >= 0) {
                vera::setWindowSize(vera::getQuiltWidth()/vera::getQuiltColumns(), 
                                    vera::getQuiltHeight()/vera::getQuiltRows());
            }
            else
                vera::setWindowSize(vera::getQuiltWidth(), vera::getQuiltHeight());
        }
    }

    // Prepare viewport
    vera::setDepthTest(false);
    glFrontFace(GL_CCW);

    // Turn on Alpha blending
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    vera::blendMode(vera::BLEND_ALPHA);

    // Clear the background
    vera::clear(0.0f);

    addDefine("GLSLVIEWER", vera::toString(GLSLVIEWER_VERSION_MAJOR) + vera::toString(GLSLVIEWER_VERSION_MINOR) + vera::toString(GLSLVIEWER_VERSION_PATCH) );
    if (uniforms.activeCubemap) {
        addDefine("SCENE_SH_ARRAY", "u_SH");
        addDefine("SCENE_CUBEMAP", "u_cubeMap");
    }

    if (uniforms.models.size() > 0 ) {
        float area = getSceneRender().getArea();
        uniforms.setSunPosition( glm::vec3(0.0,area*10.0,area*10.0) );
    }

    // LOAD SHADERS
    resetShaders( _files );
    vera::flagChange();
}

void GlslViewer::loadModel(vera::Model* _model) {
    _model->setShader(m_frag_source, m_vert_source);

    uniforms.models[_model->getName()] = _model;
    m_sceneRender.loadScene(uniforms);
    uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, m_sceneRender.getArea() * 2.0);
    vera::flagChange();
}

void GlslViewer::addDefine(const std::string &_define, const std::string &_value) {
    for (int i = 0; i < m_buffers_total; i++)
        m_buffers_shaders[i].addDefine(_define, _value);

    for (int i = 0; i < m_doubleBuffers_total; i++)
        m_doubleBuffers_shaders[i].addDefine(_define, _value);

    if (uniforms.models.size() > 0) {
        uniforms.addDefine(_define, _value);
        m_sceneRender.addDefine(_define, _value);
    }
    else
        m_canvas_shader.addDefine(_define, _value);

    m_postprocessing_shader.addDefine(_define, _value);
    vera::flagChange();
}

void GlslViewer::delDefine(const std::string &_define) {
    for (int i = 0; i < m_buffers_total; i++)
        m_buffers_shaders[i].delDefine(_define);

    for (int i = 0; i < m_doubleBuffers_total; i++)
        m_doubleBuffers_shaders[i].delDefine(_define);

    if (uniforms.models.size() > 0) {
        uniforms.delDefine(_define);
        m_sceneRender.delDefine(_define);
    }
    else
        m_canvas_shader.delDefine(_define);

    m_postprocessing_shader.delDefine(_define);
    vera::flagChange();
}

// ------------------------------------------------------------------------- GET

bool GlslViewer::isReady() {
    return m_initialized;
}

bool GlslViewer::haveChange() { 
    return  vera::haveChanged() ||
            uniforms.haveChange() ||
            isRecording() ||
            screenshotFile != "";
}

const std::string& GlslViewer::getSource(ShaderType _type) const {
    return (_type == FRAGMENT)? m_frag_source : m_vert_source;
}

void GlslViewer::setFrame(size_t _frame) { 
    uniforms.setFrame(_frame);
}

// ------------------------------------------------------------------------- RELOAD SHADER

void GlslViewer::setSource(ShaderType _type, const std::string& _source) {
    if (_type == FRAGMENT) {
        m_frag_dependencies.clear();
        m_frag_source = vera::resolveGlsl(_source, include_folders, &m_frag_dependencies);
    }
    else {
        m_vert_dependencies.clear();
        m_vert_source = vera::resolveGlsl(_source, include_folders, &m_vert_dependencies);;
    }

    // purge all dependecies that are not present in the file system
    for (int i = 0; i < m_frag_dependencies.size(); i++) {
        if (!vera::urlExists( m_frag_dependencies[i] )) {
            m_frag_dependencies.erase( m_frag_dependencies.begin() + i);
        }
    }
    
    for (int i = 0; i < m_vert_dependencies.size(); i++) {  
        if (!vera::urlExists( m_vert_dependencies[i] )) {
            m_vert_dependencies.erase( m_vert_dependencies.begin() + i);
        }
    }
};

void GlslViewer::resetShaders( WatchFileList &_files ) {
    if (vera::getWindowStyle() != vera::EMBEDDED)
        console_clear();
        
    vera::flagChange();

    // UPDATE scene shaders of models (materials)
    if (uniforms.models.size() > 0) {
        if (verbose)
            std::cout << "Reset 3D scene shaders" << std::endl;

        m_sceneRender.setShaders(uniforms, m_frag_source, m_vert_source);

        addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
        #if defined(PLATFORM_RPI)
        addDefine("LIGHT_SHADOWMAP_SIZE", "512.0");
        #else
        addDefine("LIGHT_SHADOWMAP_SIZE", "2048.0");
        #endif
    }
    else {
        if (verbose)
            std::cout << "Reset 2D shaders" << std::endl;

        // Reload the shader
        m_canvas_shader.setDefaultErrorBehaviour(m_error_screen);
        m_canvas_shader.setSource(m_frag_source, m_vert_source);
    }

    // UPDATE shaders dependencies
    {
        // purge all dependecies that are not present in the file system
        for (int i = 0; i < m_frag_dependencies.size(); i++) {
            if (!vera::urlExists( m_frag_dependencies[i] )) {
                m_frag_dependencies.erase( m_frag_dependencies.begin() + i);
            }
        }
        
        for (int i = 0; i < m_vert_dependencies.size(); i++) {  
            if (!vera::urlExists( m_vert_dependencies[i] )) {
                m_vert_dependencies.erase( m_vert_dependencies.begin() + i);
            }
        }

        vera::StringList new_dependencies = vera::mergeLists(m_frag_dependencies, m_vert_dependencies);

        // remove old dependencies
        for (int i = _files.size() - 1; i >= 0; i--)
            if (_files[i].type == GLSL_DEPENDENCY)
                _files.erase( _files.begin() + i);

        // Add new dependencies
        struct stat st;
        for (size_t i = 0; i < new_dependencies.size(); i++) {
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
    uniforms.checkUniforms(m_vert_source, m_frag_source); // Check active native uniforms
    uniforms.flagChange();                                // Flag all user defined uniforms as changed

    // UPDATE Buffers
    m_buffers_total = countBuffers(m_frag_source);
    m_doubleBuffers_total = countDoubleBuffers(m_frag_source);
    m_pyramid_total = countPyramid( m_frag_source );
    m_flood_total = countFlood( m_frag_source );

    // UPDATE Postprocessing
    if (checkPostprocessing( getSource(FRAGMENT) )) {
        // Specific defines for this buffer
        m_postprocessing_shader.addDefine("POSTPROCESSING");
        m_postprocessing_shader.setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
        uniforms.functions["u_scene"].present = true;
        m_postprocessing = true;
    }
    else if (lenticular.size() > 0) {
        m_postprocessing_shader.setSource(vera::getLenticularFragShader(vera::getVersionNumber()), vera::getDefaultSrc(vera::VERT_BILLBOARD));
        uniforms.functions["u_scene"].present = true;
        m_postprocessing = true;
    }
    else if (fxaa) {
        m_postprocessing_shader.setSource(vera::getDefaultSrc(vera::FRAG_FXAA), vera::getDefaultSrc(vera::VERT_BILLBOARD));
        uniforms.functions["u_scene"].present = true;
        m_postprocessing = true;
    }
    else 
        m_postprocessing = false;

    if (vera::getWindowStyle() != vera::EMBEDDED)
        console_refresh();

    // Make sure this runs in main loop
    m_update_buffers = true;
}

// ------------------------------------------------------------------------- UPDATE
void GlslViewer::_updateBuffers() {
    // Update Buffers
    if ( m_buffers_total != int(uniforms.buffers.size())) {
        if (verbose)
            std::cout << "Creating/removing " << uniforms.buffers.size() << " buffers to match " << m_buffers_total << std::endl;

        for (size_t i = 0; i < m_buffers_shaders.size(); i++)
            if (m_buffers_shaders[i].isLoaded())
                m_buffers_shaders[i].detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

        for (size_t i = 0; i < uniforms.buffers.size(); i++)
            delete uniforms.buffers[i];

        uniforms.buffers.clear();
        m_buffers_shaders.clear();

        for (int i = 0; i < m_buffers_total; i++) {
            // New FBO
            uniforms.buffers.push_back( new vera::Fbo() );
            glm::vec3 size = getBufferSize(m_frag_source, "u_buffer" + vera::toString(i));
            uniforms.buffers[i]->allocate(size.x, size.y, vera::COLOR_FLOAT_TEXTURE);
            uniforms.buffers[i]->scale = size.z;
            
            // New Shader
            m_buffers_shaders.push_back( vera::Shader() );
            m_buffers_shaders[i].addDefine("BUFFER_" + vera::toString(i));
            m_buffers_shaders[i].setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
        }
    }
    else
        for (size_t i = 0; i < m_buffers_shaders.size(); i++)
            m_buffers_shaders[i].setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
            
    // Update Double Buffers
    if ( m_doubleBuffers_total != int(uniforms.doubleBuffers.size()) ) {

        if (verbose)
            std::cout << "Creating/removing " << uniforms.doubleBuffers.size() << " double buffers to match " << m_doubleBuffers_total << std::endl;

        for (size_t i = 0; i < m_doubleBuffers_shaders.size(); i++)
            if (m_doubleBuffers_shaders[i].isLoaded())
                m_doubleBuffers_shaders[i].detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

        for (size_t i = 0; i < uniforms.doubleBuffers.size(); i++)
            delete uniforms.doubleBuffers[i];

        uniforms.doubleBuffers.clear();
        m_doubleBuffers_shaders.clear();

        for (int i = 0; i < m_doubleBuffers_total; i++) {
            // New FBO
            uniforms.doubleBuffers.push_back( new vera::PingPong() );

            glm::vec3 size = getBufferSize(m_frag_source, "u_doubleBuffer" + vera::toString(i));
            uniforms.doubleBuffers[i]->allocate(size.x, size.y, vera::COLOR_FLOAT_TEXTURE);
            uniforms.doubleBuffers[i]->buffer(0).scale = size.z;
            uniforms.doubleBuffers[i]->buffer(1).scale = size.z;
            
            // New Shader
            m_doubleBuffers_shaders.push_back( vera::Shader() );
            m_doubleBuffers_shaders[i].addDefine("DOUBLE_BUFFER_" + vera::toString(i));
            m_doubleBuffers_shaders[i].setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
        }
    }
    else 
        for (size_t i = 0; i < m_doubleBuffers_shaders.size(); i++)
            m_doubleBuffers_shaders[i].setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));

    // Update PYRAMID buffers
    if ( m_pyramid_total != int(uniforms.pyramids.size()) ) {

        if (verbose)
            std::cout << "Removing " << uniforms.pyramids.size() << " pyramids to create  " << m_pyramid_total << std::endl;

        for (size_t i = 0; i < m_pyramid_subshaders.size(); i++)
            if (m_pyramid_subshaders[i].isLoaded())
                m_pyramid_subshaders[i].detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);        

        uniforms.pyramids.clear();
        m_pyramid_fbos.clear();
        m_pyramid_subshaders.clear();

        for (int i = 0; i < m_pyramid_total; i++) {
            glm::vec3 size = getBufferSize(m_frag_source, "u_pyramid" + vera::toString(i));

            // Create Subshader
            m_pyramid_subshaders.push_back( vera::Shader() );
            
            // Create Pyramid structure
            uniforms.pyramids.push_back( vera::Pyramid() );
            uniforms.pyramids[i].allocate(size.x, size.y);
            uniforms.pyramids[i].scale = size.z;

            // Create pass function for this pyramid
            uniforms.pyramids[i].pass = [this](vera::Fbo *_target, const vera::Fbo *_tex0, const vera::Fbo *_tex1, int _depth) {
                _target->bind();
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                m_pyramid_shader.use();

                uniforms.feedTo( &m_pyramid_shader );

                m_pyramid_shader.setUniform("u_pyramidDepth", _depth);
                m_pyramid_shader.setUniform("u_pyramidTotalDepth", (int)uniforms.pyramids[0].getDepth());
                m_pyramid_shader.setUniform("u_pyramidUpscaling", _tex1 != NULL);

                m_pyramid_shader.textureIndex = (uniforms.models.size() == 0) ? 1 : 0;
                m_pyramid_shader.setUniformTexture("u_pyramidTex0", _tex0);
                if (_tex1 != NULL)
                    m_pyramid_shader.setUniformTexture("u_pyramidTex1", _tex1);
                m_pyramid_shader.setUniform("u_resolution", ((float)_target->getWidth()), ((float)_target->getHeight()));
                m_pyramid_shader.setUniform("u_pixel", 1.0f/((float)_target->getWidth()), 1.0f/((float)_target->getHeight()));

                vera::billboard()->render( &m_pyramid_shader );
                _target->unbind();
            };

            // Create input FBO
            m_pyramid_fbos.push_back( vera::Fbo() );
            m_pyramid_fbos[i].allocate(size.x, size.y, vera::COLOR_FLOAT_TEXTURE);
            m_pyramid_fbos[i].scale = size.z;
        }
    }

    // Update PYRAMID algo
    if (m_pyramid_total > 0 ) {
        if ( checkPyramidAlgorithm( getSource(FRAGMENT) ) ) {
            m_pyramid_shader.addDefine("PYRAMID_ALGORITHM");
            m_pyramid_shader.setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
        }
        else
            m_pyramid_shader.setSource(vera::getDefaultSrc(vera::FRAG_POISSONFILL), vera::getDefaultSrc(vera::VERT_BILLBOARD));
    }
    
    // Update PYRAMID subshaders
    for (size_t i = 0; i < m_pyramid_subshaders.size(); i++) {
        m_pyramid_subshaders[i].addDefine("PYRAMID_" + vera::toString(i));
        m_pyramid_subshaders[i].setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
    }

    // Update FLOOD Buffers
    if ( m_flood_total != int(uniforms.floods.size()) ) {

        if (verbose)
            std::cout << "Removing " << uniforms.floods.size() << " flood to create " << m_flood_total << std::endl;

        for (size_t i = 0; i < m_flood_subshaders.size(); i++)
            if (m_flood_subshaders[i].isLoaded())
                m_flood_subshaders[i].detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);        

        uniforms.floods.clear();
        m_flood_subshaders.clear();

        for (int i = 0; i < m_flood_total; i++) {
            glm::vec3 size = getBufferSize(m_frag_source, "u_flood" + vera::toString(i));

            // Create Subshader
            m_flood_subshaders.push_back( vera::Shader() );
            
            // Create Pyramid structure
            uniforms.floods.push_back( vera::Flood() );
            uniforms.floods[i].allocate(size.x, size.y, vera::COLOR_FLOAT_TEXTURE);
            uniforms.floods[i].scale = size.z;

            // Create pass function for this flood
            uniforms.floods[i].pass = [this](vera::Fbo *_dst, const vera::Fbo *_src, int _index) {
                _dst->bind();
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                m_flood_shader.use();

                // uniforms.feedTo( &m_flood_shader, false, false );

                m_flood_shader.setUniform("u_resolution", ((float)_dst->getWidth()), ((float)_dst->getHeight()));
                m_flood_shader.setUniform("u_floodIndex",_index);
                m_flood_shader.setUniform("u_floodTotal", (int)uniforms.floods[0].getTotalIterations());

                m_flood_shader.textureIndex = (uniforms.models.size() == 0) ? 1 : 0;
                m_flood_shader.setUniformTexture("u_floodSrc", _src);

                vera::billboard()->render( &m_flood_shader );
                _dst->unbind();
            };
        }
    }

    if (m_flood_total > 0 ) {
        if ( checkFloodAlgorithm( getSource(FRAGMENT) ) ) {
            m_flood_shader.addDefine("FLOOD_ALGORITHM");
            m_flood_shader.setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
        }
        else
            m_flood_shader.setSource(vera::getDefaultSrc(vera::FRAG_JUMPFLOOD), vera::getDefaultSrc(vera::VERT_BILLBOARD));
    }
    
    for (size_t i = 0; i < m_flood_subshaders.size(); i++) {
        m_flood_subshaders[i].addDefine("FLOOD_" + vera::toString(i));
        m_flood_subshaders[i].setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
    }

    // Update Postprocessing
    if (m_postprocessing || m_plot == PLOT_RGB || m_plot == PLOT_RED || m_plot == PLOT_GREEN || m_plot == PLOT_BLUE || m_plot == PLOT_LUMA) {
        if (quilt_resolution >= 0)
            m_sceneRender.updateBuffers(uniforms, vera::getQuiltWidth(), vera::getQuiltHeight());
        else 
            m_sceneRender.updateBuffers(uniforms, vera::getWindowWidth(), vera::getWindowHeight());
    }

}

// ------------------------------------------------------------------------- DRAW
void GlslViewer::_renderBuffers() {
    glDisable(GL_BLEND);

    bool reset_viewport = false;
    for (size_t i = 0; i < uniforms.buffers.size(); i++) {
        if (!(uniforms.buffers[i]->enabled || m_update_buffers))
            continue;

        TRACK_BEGIN("render:buffer" + vera::toString(i))

        reset_viewport += uniforms.buffers[i]->scale <= 0.0;

        uniforms.buffers[i]->bind();

        m_buffers_shaders[i].use();
        m_buffers_shaders[i].setUniform("u_model", glm::vec3(1.0f));
        m_buffers_shaders[i].setUniform("u_modelMatrix", glm::mat4(1.0f));
        m_buffers_shaders[i].setUniform("u_viewMatrix", glm::mat4(1.0f));
        m_buffers_shaders[i].setUniform("u_projectionMatrix", glm::mat4(1.0f));

        // Pass textures for the other buffers
        for (size_t j = 0; j < uniforms.buffers.size(); j++)
            if (i != j)
                m_buffers_shaders[i].setUniformTexture("u_buffer" + vera::toString(j), uniforms.buffers[j]  );

        for (size_t j = 0; j < uniforms.doubleBuffers.size(); j++)
            m_buffers_shaders[i].setUniformTexture("u_doubleBuffer" + vera::toString(j), uniforms.doubleBuffers[j]->src );

        for (size_t j = 0; j < uniforms.floods.size(); j++)
            m_buffers_shaders[i].setUniformTexture("u_flood" + vera::toString(j), uniforms.floods[j].dst );

        for (size_t j = 0; j < m_sceneRender.buffersFbo.size(); j++)
            m_buffers_shaders[i].setUniformTexture("u_sceneBuffer" + vera::toString(j), m_sceneRender.buffersFbo[j] );

        // Update uniforms and textures
        uniforms.feedTo( &m_buffers_shaders[i], true, false);

        vera::billboard()->render( &m_buffers_shaders[i] );
        
        uniforms.buffers[i]->unbind();

        TRACK_END("render:buffer" + vera::toString(i))
    }

    for (size_t i = 0; i < uniforms.doubleBuffers.size(); i++) {
        TRACK_BEGIN("render:doubleBuffer" + vera::toString(i))

        reset_viewport += uniforms.doubleBuffers[i]->src->scale <= 0.0;

        uniforms.doubleBuffers[i]->dst->bind();

        m_doubleBuffers_shaders[i].use();

        // Pass textures for the other buffers
        for (size_t j = 0; j < uniforms.buffers.size(); j++)
            m_doubleBuffers_shaders[i].setUniformTexture("u_buffer" + vera::toString(j), uniforms.buffers[j] );

        for (size_t j = 0; j < uniforms.doubleBuffers.size(); j++)
            m_doubleBuffers_shaders[i].setUniformTexture("u_doubleBuffer" + vera::toString(j), uniforms.doubleBuffers[j]->src );

        for (size_t j = 0; j < uniforms.floods.size(); j++)
            m_doubleBuffers_shaders[i].setUniformTexture("u_flood" + vera::toString(j), uniforms.floods[j].dst );

        for (size_t j = 0; j < m_sceneRender.buffersFbo.size(); j++)
            m_doubleBuffers_shaders[i].setUniformTexture("u_sceneBuffer" + vera::toString(j), m_sceneRender.buffersFbo[j] );

        // Update uniforms and textures
        uniforms.feedTo( &m_doubleBuffers_shaders[i], true, false);

        vera::billboard()->render( &m_doubleBuffers_shaders[i] );
        
        uniforms.doubleBuffers[i]->dst->unbind();
        uniforms.doubleBuffers[i]->swap();

        TRACK_END("render:doubleBuffer" + vera::toString(i))
    }

    for (size_t i = 0; i < m_pyramid_subshaders.size(); i++) {
        TRACK_BEGIN("render:pyramid" + vera::toString(i))

        reset_viewport += m_pyramid_fbos[i].scale <= 0.0;

        m_pyramid_fbos[i].bind();
        m_pyramid_subshaders[i].use();

        // Clear the background
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update uniforms and textures
        uniforms.feedTo( &m_pyramid_subshaders[i], true, true );

        for (size_t j = 0; j < m_sceneRender.buffersFbo.size(); j++)
            if (m_sceneRender.buffersFbo[j]->isAllocated())
                m_pyramid_subshaders[i].setUniformTexture("u_sceneBuffer" + vera::toString(j), m_sceneRender.buffersFbo[j] );

        vera::billboard()->render( &m_pyramid_subshaders[i] );

        m_pyramid_fbos[i].unbind();

        vera::blendMode(vera::BLEND_ALPHA);
        uniforms.pyramids[i].process(&m_pyramid_fbos[i]);

        TRACK_END("render:pyramid" + vera::toString(i))
    }

    for (size_t i = 0; i < m_flood_subshaders.size(); i++) {
        TRACK_BEGIN("render:flood" + vera::toString(i))

        reset_viewport += uniforms.floods[i].scale <= 0.0;

        uniforms.floods[i].dst->bind();
        m_flood_subshaders[i].use();

        // Clear the background
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (size_t j = 0; j < uniforms.buffers.size(); j++)
            m_flood_subshaders[i].setUniformTexture("u_buffer" + vera::toString(j), uniforms.buffers[j] );

        for (size_t j = 0; j < uniforms.doubleBuffers.size(); j++)
            m_flood_subshaders[i].setUniformTexture("u_doubleBuffer" + vera::toString(j), uniforms.doubleBuffers[j]->src );

        for (size_t j = 0; j < uniforms.floods.size(); j++)
            m_flood_subshaders[i].setUniformTexture("u_flood" + vera::toString(j), uniforms.floods[j].src );

        for (size_t j = 0; j < m_sceneRender.buffersFbo.size(); j++)
            if (m_sceneRender.buffersFbo[j]->isAllocated())
                m_flood_subshaders[i].setUniformTexture("u_sceneBuffer" + vera::toString(j), m_sceneRender.buffersFbo[j] );

        // Update uniforms and textures
        uniforms.feedTo( &m_flood_subshaders[i], true, false );

        vera::billboard()->render( &m_flood_subshaders[i] );

        uniforms.floods[i].dst->unbind();

        vera::blendMode(vera::BLEND_ALPHA);
        uniforms.floods[i].process();

        TRACK_END("render:flood" + vera::toString(i))
    }

    #if defined(__EMSCRIPTEN__)
    if (vera::getWebGLVersionNumber() == 1)
        reset_viewport = true;
    #endif

    if (vera::getWindowStyle() != vera::EMBEDDED && reset_viewport)
        glViewport(0.0f, 0.0f, vera::getWindowWidth(), vera::getWindowHeight());
    
    m_update_buffers = false;

    vera::blendMode(vera::BLEND_ALPHA);
}

void GlslViewer::renderPrep() {
    TRACK_BEGIN("render")

    // UPDATE STREAMING TEXTURES
    // -----------------------------------------------
    if (m_initialized) {
        uniforms.update();
        if (uniforms.loadQueue.size() > 0) {
            uniforms.loadMutex.lock();
            for (ImagesMap::iterator it = uniforms.loadQueue.begin(); it != uniforms.loadQueue.end(); ++it)
                uniforms.addTexture( it->first, it->second );
            uniforms.loadQueue.clear();
            uniforms.loadMutex.unlock();
        }
    }

    // BUFFERS
    // -----------------------------------------------
    if (m_update_buffers ||
        m_buffers_total != int(uniforms.buffers.size()) ||
        m_doubleBuffers_total != int(uniforms.doubleBuffers.size()) )
        _updateBuffers();
    
    if (uniforms.buffers.size() > 0 || 
        uniforms.doubleBuffers.size() > 0 ||
        m_pyramid_total > 0 ||
        m_flood_total > 0)
        _renderBuffers();

    // RENDER SHADOW MAP
    // -----------------------------------------------
    if (uniforms.models.size() > 0)
        m_sceneRender.renderShadowMap(uniforms);
    
    // MAIN SCENE
    // ----------------------------------------------- < main scene start
    if (screenshotFile != "" || isRecording() )
        if (!m_record_fbo.isAllocated())
            m_record_fbo.allocate(vera::getWindowWidth(), vera::getWindowHeight(), vera::COLOR_TEXTURE_DEPTH_BUFFER);

    if (uniforms.functions["u_sceneNormal"].present)
        m_sceneRender.renderNormalBuffer(uniforms);

    if (uniforms.functions["u_scenePosition"].present)
        m_sceneRender.renderPositionBuffer(uniforms);

    if (m_sceneRender.getBuffersTotal() != 0)
        m_sceneRender.renderBuffers(uniforms);

    if (m_postprocessing || m_plot == PLOT_LUMA || m_plot == PLOT_RGB || m_plot == PLOT_RED || m_plot == PLOT_GREEN || m_plot == PLOT_BLUE ) {
        m_sceneRender.renderFbo.bind();
    }
    else if (screenshotFile != "") {
        if (vera::haveExt(screenshotFile, "png")) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        m_record_fbo.bind();
    }
    else if (isRecording())
        m_record_fbo.bind();

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GlslViewer::render() {
    // RENDER CONTENT
    if (uniforms.models.size() == 0) {
        TRACK_BEGIN("render:2D_scene")

        // Load main shader
        m_canvas_shader.use();

        if (quilt_resolution >= 0) {
            vera::renderQuilt([&](const vera::QuiltProperties& quilt, glm::vec4& viewport, int &viewIndex) {

                // set up the camera rotation and position for current view
                uniforms.activeCamera->setVirtualOffset(5.0f, viewIndex, quilt.totalViews);
                uniforms.set("u_tile", float(quilt.columns), float(quilt.rows), float(quilt.totalViews));
                uniforms.set("u_viewport", float(viewport.x), float(viewport.y), float(viewport.z), float(viewport.w));

                // Update Uniforms and textures variables
                uniforms.feedTo( &m_canvas_shader );

                // Pass special uniforms
                m_canvas_shader.setUniform("u_model", glm::vec3(1.0f));
                m_canvas_shader.setUniform("u_modelMatrix", glm::mat4(1.0f));
                m_canvas_shader.setUniform("u_viewMatrix", glm::mat4(1.0f));
                m_canvas_shader.setUniform("u_projectionMatrix", glm::mat4(1.0f));
                m_canvas_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.));
                vera::billboard()->render( &m_canvas_shader );
            }, quilt_tile, true);
        }

        else {
            // Update Uniforms and textures variables
            uniforms.feedTo( &m_canvas_shader );

            // Pass special uniforms
            m_canvas_shader.setUniform("u_model", glm::vec3(1.0f));
            m_canvas_shader.setUniform("u_modelMatrix", glm::mat4(1.0f));
            m_canvas_shader.setUniform("u_viewMatrix", glm::mat4(1.0f));
            m_canvas_shader.setUniform("u_projectionMatrix", glm::mat4(1.0f));
            m_canvas_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.));
            vera::billboard()->render( &m_canvas_shader );
        }

        TRACK_END("render:2D_scene")
    }

    else {
        TRACK_BEGIN("render:3D_scene")
        if (quilt_resolution >= 0) {
            vera::renderQuilt([&](const vera::QuiltProperties& quilt, glm::vec4& viewport, int &viewIndex){

                // set up the camera rotation and position for current view
                uniforms.activeCamera->setVirtualOffset(m_sceneRender.getArea() * 0.75, viewIndex, quilt.totalViews);

                uniforms.set("u_tile", float(quilt.columns), float(quilt.rows), float(quilt.totalViews));
                uniforms.set("u_viewport", float(viewport.x), float(viewport.y), float(viewport.z), float(viewport.w));

                m_sceneRender.render(uniforms);

                if (m_sceneRender.showGrid || m_sceneRender.showAxis || m_sceneRender.showBBoxes)
                    m_sceneRender.renderDebug(uniforms);
            }, quilt_tile, true);
        }

        else {
            m_sceneRender.render(uniforms);
            if (m_sceneRender.showGrid || m_sceneRender.showAxis || m_sceneRender.showBBoxes) {
                m_sceneRender.renderDebug(uniforms);
            }
        }
        TRACK_END("render:3D_scene")
    }
}

void GlslViewer::renderPost() {
    // POST PROCESSING
    if (m_postprocessing) {
        TRACK_BEGIN("render:postprocessing")

        m_sceneRender.renderFbo.unbind();

        if (screenshotFile != "") {
            if (vera::haveExt(screenshotFile, "png")) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            }
            m_record_fbo.bind();
        }
        else if (isRecording())
            m_record_fbo.bind();
    
        m_postprocessing_shader.use();
        m_postprocessing_shader.setUniform("u_model", glm::vec3(1.0f));
        m_postprocessing_shader.setUniform("u_modelMatrix", glm::mat4(1.0f));
        m_postprocessing_shader.setUniform("u_viewMatrix", glm::mat4(1.0f));
        m_postprocessing_shader.setUniform("u_projectionMatrix", glm::mat4(1.0f));
        m_postprocessing_shader.setUniform("u_modelViewProjectionMatrix", glm::mat4(1.0f) );//vera::getOrthoMatrix());

        // Update uniforms and textures
        uniforms.feedTo( &m_postprocessing_shader, true, true );

        for (size_t i = 0; i < m_sceneRender.buffersFbo.size(); i++)
            m_postprocessing_shader.setUniformTexture("u_sceneBuffer" + vera::toString(i), m_sceneRender.buffersFbo[i]);//, m_postprocessing_shader.textureIndex++);

        if (lenticular.size() > 0)
            feedLenticularUniforms(m_postprocessing_shader);

        vera::billboard()->render( &m_postprocessing_shader );

        TRACK_END("render:postprocessing")
    }
    else if (m_plot == PLOT_RGB || m_plot == PLOT_RED || m_plot == PLOT_GREEN || m_plot == PLOT_BLUE || m_plot == PLOT_LUMA) {
        m_sceneRender.renderFbo.unbind();

        if (screenshotFile != "") {
            if (vera::haveExt(screenshotFile, "png")) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            }
            m_record_fbo.bind();
        }
        else if (isRecording())
            m_record_fbo.bind();

        vera::image(m_sceneRender.renderFbo);
    }
        
    if (screenshotFile != "" || isRecording()) {
        m_record_fbo.unbind();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);

        vera::image(m_record_fbo);
    }

    TRACK_END("render")
    
    if  (vera::getWindowStyle() != vera::EMBEDDED)
        console_uniforms_refresh();
}


void GlslViewer::renderUI() {
    TRACK_BEGIN("renderUI")

    vera::resetCamera();

    // // IN PUT TEXTURES
    if (m_showTextures) {      

        int nTotal = uniforms.textures.size();
        if (nTotal > 0) {
            vera::setDepthTest(false);
            TRACK_BEGIN("renderUI:textures")
            float w = (float)(vera::getWindowWidth());
            float h = (float)(vera::getWindowHeight());
            float a = h/w;
            float scale = fmin(1.0f / (float)(nTotal), 0.25) * 0.5;
            float xStep = w * scale * a;
            float yStep = h * scale;
            float xOffset = xStep;
            float yOffset = h - yStep;
            
            vera::textAngle(0.0);
            vera::textAlign(vera::ALIGN_TOP);
            vera::textAlign(vera::ALIGN_LEFT);
            vera::textSize(xStep * 0.2f / vera::pixelDensity());

            for (vera::TexturesMap::iterator it = uniforms.textures.begin(); it != uniforms.textures.end(); it++) {
                // if texture names starts width '_' skip it (internal texture)
                if (it->first[0] == '_')
                    continue;
                     
                float imgAspect = ((float)it->second->getWidth()/(float)it->second->getHeight());
                float imgW = xStep * imgAspect;
                vera::TextureStreamsMap::const_iterator slit = uniforms.streams.find(it->first);
                if ( slit != uniforms.streams.end() )
                    vera::image((vera::TextureStream*)slit->second, xStep * 0.5 + imgW * 0.5, yOffset, imgW, yStep, true);
                else 
                    vera::image(it->second, xStep * 0.5 + imgW * 0.5, yOffset, imgW, yStep);

                vera::text(it->first, 0, vera::getWindowHeight() - yOffset - yStep);

                yOffset -= yStep * 2.0;
            }
            TRACK_END("renderUI:textures")
        }
    }

    // RESULTING BUFFERS
    if (m_showPasses) {
        vera::setDepthTest(false);
        TRACK_BEGIN("renderUI:buffers")

        // DEBUG BUFFERS
        int nTotal = uniforms.buffers.size();
        nTotal += uniforms.doubleBuffers.size();
        nTotal += uniforms.pyramids.size();
        nTotal += uniforms.floods.size();
        if (uniforms.models.size() > 0 ) {
            nTotal += 1;
            nTotal += uniforms.functions["u_scene"].present;
            nTotal += uniforms.functions["u_sceneDepth"].present;
        }
        nTotal += uniforms.functions["u_sceneNormal"].present;
        nTotal += uniforms.functions["u_scenePosition"].present;
        nTotal += m_sceneRender.getBuffersTotal();
        
        if (nTotal > 0) {
            float w = (float)(vera::getWindowWidth());
            float h = (float)(vera::getWindowHeight());
            float scale = fmin(1.0f / (float)(nTotal), 0.25) * 0.5;
            float xStep = w * scale;
            float yStep = h * scale;
            float xOffset = w - xStep;
            float yOffset = h - yStep;

            vera::textAngle(0.0);
            vera::textSize(xStep * 0.2f / vera::pixelDensity());
            vera::textAlign(vera::ALIGN_TOP);
            vera::textAlign(vera::ALIGN_LEFT);

            for (size_t i = 0; i < uniforms.buffers.size(); i++) {
                glm::vec2 offset = glm::vec2(xOffset, yOffset);
                glm::vec2 scale = glm::vec2(yStep);
                scale.x *= ((float)uniforms.buffers[i]->getWidth()/(float)uniforms.buffers[i]->getHeight());
                offset.x += xStep - scale.x;

                vera::image(uniforms.buffers[i], offset.x, offset.y, scale.x, scale.y);
                vera::text("u_buffer" + vera::toString(i), xOffset - scale.x, vera::getWindowHeight() - yOffset - yStep);

                yOffset -= yStep * 2.0;
            }

            for (size_t i = 0; i < uniforms.doubleBuffers.size(); i++) {
                glm::vec2 offset = glm::vec2(xOffset, yOffset);
                glm::vec2 scale = glm::vec2(yStep);
                scale.x *= ((float)uniforms.doubleBuffers[i]->src->getWidth()/(float)uniforms.doubleBuffers[i]->src->getHeight());
                offset.x += xStep - scale.x;

                vera::image(uniforms.doubleBuffers[i]->src, offset.x, offset.y, scale.x, scale.y);
                vera::text("u_doubleBuffer" + vera::toString(i), xOffset - scale.x, vera::getWindowHeight() - yOffset - yStep);

                yOffset -= yStep * 2.0;
            }

            for (size_t i = 0; i < uniforms.pyramids.size(); i++) {
                glm::vec2 offset = glm::vec2(xOffset, yOffset);
                glm::vec2 scale = glm::vec2(yStep);
                scale.x *= ((float)uniforms.pyramids[i].getWidth()/(float)uniforms.pyramids[i].getHeight());
                float w = scale.x;
                offset.x += xStep - w;
                for (size_t j = 0; j < uniforms.pyramids[i].getDepth() * 2; j++ ) {

                    if (j < uniforms.pyramids[i].getDepth())
                        vera::image(uniforms.pyramids[i].getResult(j), offset.x, offset.y, scale.x, scale.y);
                    else 
                        vera::image(uniforms.pyramids[i].getResult(j), offset.x + w * 2.0f, offset.y, scale.x, scale.y);

                    offset.x -= scale.x;
                    if (j < uniforms.pyramids[i].getDepth()) {
                        scale *= 0.5;
                        offset.y = yOffset - yStep * 0.5;
                    }
                    else {
                        offset.y = yOffset + yStep * 0.5;
                        scale *= 2.0;
                    }
                    offset.x -= scale.x;

                }

                vera::text("u_pyramid0" + vera::toString(i), xOffset - scale.x * 2.0, vera::getWindowHeight() - yOffset - yStep);
                yOffset -= yStep * 2.0;

            }

            for (size_t i = 0; i < uniforms.floods.size(); i++) {
                glm::vec2 offset = glm::vec2(xOffset, yOffset);
                glm::vec2 scale = glm::vec2(yStep);
                scale.x *= ((float)uniforms.floods[i].dst->getWidth()/(float)uniforms.floods[i].dst->getHeight());
                offset.x += xStep - scale.x;

                vera::image(uniforms.floods[i].dst, offset.x, offset.y, scale.x, scale.y);
                vera::text("u_flood" + vera::toString(i), xOffset - scale.x, vera::getWindowHeight() - yOffset - yStep);

                yOffset -= yStep * 2.0;
            }

            if (uniforms.models.size() > 0) {
                vera::fill(0.0);
                for (vera::LightsMap::iterator it = uniforms.lights.begin(); it != uniforms.lights.end(); ++it ) {
                    if ( it->second->getShadowMap()->isAllocated() && it->second->getShadowMap()->getDepthTextureId() ) {
                        vera::imageDepth(it->second->getShadowMap(), xOffset, yOffset, xStep, yStep, it->second->getShadowMapFar(), it->second->getShadowMapNear());
                        vera::text("u_lightShadowMap", xOffset - xStep, vera::getWindowHeight() - yOffset - yStep);
                        yOffset -= yStep * 2.0;
                    }
                }
                vera::fill(1.0);
            }

            if (uniforms.functions["u_scenePosition"].present) {
                vera::image(&m_sceneRender.positionFbo, xOffset, yOffset, xStep, yStep);
                vera::text("u_scenePosition", xOffset - xStep, vera::getWindowHeight() - yOffset - yStep);
                yOffset -= yStep * 2.0;
            }

            if (uniforms.functions["u_sceneNormal"].present) {
                vera::image(&m_sceneRender.normalFbo, xOffset, yOffset, xStep, yStep);
                vera::text("u_sceneNormal", xOffset - xStep, vera::getWindowHeight() - yOffset - yStep);
                yOffset -= yStep * 2.0;
            }

            for (size_t i = 0; i < m_sceneRender.buffersFbo.size(); i++) {
                vera::image(m_sceneRender.buffersFbo[i], xOffset, yOffset, xStep, yStep);
                vera::text("u_sceneBuffer" + vera::toString(i), xOffset - xStep, vera::getWindowHeight() - yOffset - yStep);
                yOffset -= yStep * 2.0;
            }

            if (uniforms.functions["u_scene"].present) {
                vera::image(&m_sceneRender.renderFbo, xOffset, yOffset, xStep, yStep);
                vera::text("u_scene", xOffset - xStep, vera::getWindowHeight() - yOffset - yStep);
                yOffset -= yStep * 2.0;
            }

            if (uniforms.functions["u_sceneDepth"].present) {
                vera::fill(0.0);
                if (uniforms.activeCamera)
                    vera::imageDepth(&m_sceneRender.renderFbo, xOffset, yOffset, xStep, yStep, uniforms.activeCamera->getFarClip(), uniforms.activeCamera->getNearClip());
                vera::text("u_sceneDepth", xOffset - xStep, vera::getWindowHeight() - yOffset - yStep);
                yOffset -= yStep * 2.0;
                vera::fill(1.0);
            }
        }
        TRACK_END("renderUI:buffers")
    }

    if (m_plot != PLOT_OFF && m_plot_texture ) {
        vera::setDepthTest(false);
        TRACK_BEGIN("renderUI:plot_data")

        float p = vera::pixelDensity();
        float w = 100 * p;
        float h = 30 * p;
        float x = (float)(vera::getWindowWidth()) * 0.5;
        float y = h + 10;

        m_plot_shader.use();
        m_plot_shader.setUniform("u_scale", w, h);
        m_plot_shader.setUniform("u_translate", x, y);
        m_plot_shader.setUniform("u_resolution", (float)vera::getWindowWidth(), (float)vera::getWindowHeight());
        m_plot_shader.setUniform("u_viewport", w, h);
        m_plot_shader.setUniform("u_model", glm::vec3(1.0f));
        m_plot_shader.setUniform("u_modelMatrix", glm::mat4(1.0f));
        m_plot_shader.setUniform("u_viewMatrix", glm::mat4(1.0f));
        m_plot_shader.setUniform("u_projectionMatrix", glm::mat4(1.0f));
        m_plot_shader.setUniform("u_modelViewProjectionMatrix", vera::getOrthoMatrix());
        m_plot_shader.setUniformTexture("u_plotData", m_plot_texture, 0);
        
        vera::billboard()->render(&m_plot_shader);
        TRACK_END("renderUI:plot_data")
    }

    if (cursor && vera::getMouseEntered()) {
        TRACK_BEGIN("renderUI:cursor")
        if (m_cross_vbo == nullptr)
            m_cross_vbo = std::unique_ptr<vera::Vbo>(new vera::Vbo( vera::crossMesh( glm::vec3(0.0f, 0.0f, 0.0f), 10.0f) ));

        vera::Shader* fill = vera::fillShader();
        fill->use();
        fill->setUniform("u_modelViewProjectionMatrix", glm::translate(vera::getOrthoMatrix(), glm::vec3(vera::getMousePositionFlipped(), 0.0f) ) );
        fill->setUniform("u_color", glm::vec4(1.0f));
        m_cross_vbo->render(fill);
        TRACK_END("renderUI:cursor")
    }

    #if !defined(__EMSCRIPTEN__)
    if (frag_index == -1 && vert_index == -1 && geom_index == -1 && vera::getWindowStyle() != vera::EMBEDDED) {
        float w = (float)(vera::getWindowWidth());
        float h = (float)(vera::getWindowHeight());
        float xStep = w * 0.05;
        float yStep = h * 0.05;
        float x = xStep * 2.0f;
        float y = yStep * 3.0f;

        vera::Camera *cam = vera::camera();
        vera::resetCamera();

        vera::fill(0.0f, 0.0f, 0.0f, 0.75f);
        vera::noStroke();
        vera::rect(glm::vec2(w * 0.5f, h * 0.5f), glm::vec2(w - xStep * 2.0f, h - yStep * 2.0f));

        vera::fill(1.0f);

        vera::textAngle(0.0f);
        vera::textAlign(vera::ALIGN_MIDDLE);
        vera::textAlign(vera::ALIGN_CENTER);

        vera::textSize(38.0f);
        vera::text("Drag & Drop", w * 0.5f, h * 0.45f);

        vera::textSize(22.0f);
        vera::text(".vert .frag .ply .lst .obj .gltf .glb .splat", w * 0.5f, h * 0.55f);

        vera::setCamera(cam);
    }
    #endif

    if (help) {
        float w = (float)(vera::getWindowWidth());
        float h = (float)(vera::getWindowHeight());
        float xStep = w * 0.05;
        float yStep = h * 0.05;
        float x = xStep * 2.0f;
        float y = yStep * 3.0f;

        vera::Camera *cam = vera::camera();
        vera::resetCamera();

        vera::fill(0.0f, 0.0f, 0.0f, 0.75f);
        vera::noStroke();
        vera::rect(glm::vec2(w * 0.5f, h * 0.5f), glm::vec2(w - xStep * 2.0f, h - yStep * 2.0f));

        vera::fill(1.0f);
        vera::textAlign(vera::ALIGN_MIDDLE);
        vera::textAlign(vera::ALIGN_LEFT);
        vera::textAngle(0.0f);
        vera::textSize(22.0f);
        yStep = vera::textHeight() * 1.5f;

        if (geom_index != -1) {
            vera::text("a - " + std::string( m_sceneRender.showAxis? "hide" : "show" ) + " axis", x, y);
            y += yStep;
            vera::text("b - " + std::string( m_sceneRender.showBBoxes? "hide" : "show" ) + " bounding boxes", x, y);
            y += yStep;
        }

        vera::text("c - hide/show cursor", x, y);
        y += yStep;

        if (geom_index != -1) {
            vera::text("d - " + std::string( m_sceneRender.dynamicShadows? "disable" : "enable" ) + " dynamic shadows", x, y);
            y += yStep;
            vera::text("f - hide/show floor", x, y);
            y += yStep;
        }
        vera::text("F - " + std::string( vera::isFullscreen() ? "disable" : "enable" ) + " fullscreen", x, y);
        y += yStep;
        if (geom_index != -1) {
            vera::text("g - " + std::string( m_sceneRender.showGrid? "hide" : "show" ) + " grid", x, y);
            y += yStep;
        }

        vera::text("h - " + std::string( help? "hide" : "show" ) + " help", x, y);
        y += yStep;
        vera::text("i - hide/show extra info", x, y);
        y += yStep;
        vera::text("o - open shaders on default editor", x, y);
        y += yStep;
        vera::text("p - hide/show render passes/buffers", x, y);
        y += yStep;

        if (uniforms.streams.size() > 0) {
            vera::text("r - restart stream textures", x, y);
            y += yStep;
        }

        if (geom_index != -1) {
            vera::text("s - hide/show sky", x, y);
            y += yStep;
        }

        vera::text("t - hide/show loaded textures", x, y);
        y += yStep;
        vera::text("v - " + std::string( verbose? "disable" : "enable" ) + " verbose", x, y);
        y += yStep;

        if (uniforms.streams.size() > 0) {
            vera::text("space - start/stop stream textures", x, y);
            y += yStep;
        }
        
        vera::setCamera(cam);
        vera::setDepthTest(false);
    }

    TRACK_END("renderUI")
}

void GlslViewer::renderDone() {
    TRACK_BEGIN("update:post_render")

    // RECORD
    if (isRecording()) {
        onScreenshot( vera::toString( getRecordingCount() , 0, 5, '0') + ".png");
        recordingFrameAdded();
    }
    // SCREENSHOT 
    else if (screenshotFile != "") {
        onScreenshot(screenshotFile);
        screenshotFile = "";
    }

    vera::resetChange();
    uniforms.resetChange();
    m_change_viewport = false;

    if (m_plot != PLOT_OFF)
        onPlot();

    if (!m_initialized) {
        m_initialized = true;
        vera::updateViewport();
        vera::flagChange();
    }

    TRACK_END("update:post_render")
}

// ------------------------------------------------------------------------- ACTIONS
void GlslViewer::printDependencies(ShaderType _type) const {
    if (_type == FRAGMENT)
        for (size_t i = 0; i < m_frag_dependencies.size(); i++)
            std::cout << m_frag_dependencies[i] << std::endl;
    
    else 
        for (size_t i = 0; i < m_vert_dependencies.size(); i++)
            std::cout << m_vert_dependencies[i] << std::endl;
}

// ------------------------------------------------------------------------- EVENTS
namespace {
    template<typename uniform_list_t>
    void reload_uniforms(uniform_list_t& unforms_list, std::string filename, const WatchFile &_file) {
        using uniform_t = typename uniform_list_t::value_type;
        auto uniform_iterator = std::find_if(std::begin(unforms_list), std::end(unforms_list)
                            , [&](uniform_t uniform){return filename == uniform.second->getFilePath();});
        if(uniform_iterator != std::end(unforms_list)) {
            std::cout << "Reloading" << filename << std::endl;
            uniform_iterator->second->load(filename, _file.vFlip);
        }
    }
}

void GlslViewer::onFileChange(WatchFileList &_files, int index) {
    FileType type = _files[index].type;
    std::string filename = _files[index].path;

    const auto reset_shaders = [&](std::string& source, vera::StringList& dependencies){
        source = "";
        dependencies.clear();
        if ( vera::loadGlslFrom(filename, &source, include_folders, &dependencies) )
            resetShaders(_files);
    };

    const auto dependency_matches_filename = [&](const vera::StringList& dependencies) {
        return std::any_of(std::begin(dependencies), std::end(dependencies), [&](const std::string& dependency){ return dependency == filename; });
    };

    // IF the change is on a dependency file, re route to the correct shader that need to be reload
    if (type == GLSL_DEPENDENCY) {
        using kv = std::tuple<FileType, std::string>;
        if (dependency_matches_filename(m_frag_dependencies)){
            std::tie(type, filename) = kv{FRAG_SHADER, _files[frag_index].path};
        } else if (dependency_matches_filename(m_vert_dependencies)){
            std::tie(type, filename) = kv{VERT_SHADER, _files[vert_index].path};
        }
    }
    switch(type) {
    case FRAG_SHADER:
        reset_shaders(m_frag_source, m_frag_dependencies);
        break;
    case VERT_SHADER:
        reset_shaders(m_vert_source, m_vert_dependencies);
        break;
    case GEOMETRY:
        // TODO
        break;
    case IMAGE:
        reload_uniforms(uniforms.textures, filename, _files[index]);
        break;
    case CUBEMAP:
        reload_uniforms(uniforms.cubemaps, filename, _files[index]);
        break;
    default: //'GLSL_DEPENDENCY' and 'IMAGE_BUMPMAP' not handled in switch
        break;
    }

    vera::flagChange();
    uniforms.flagChange();
}

void GlslViewer::onScroll(float _yoffset) {
    // Vertical scroll button zooms u_view2d and view3d.
    /* zoomfactor 2^(1/4): 4 scroll wheel clicks to double in size. */
    constexpr float zoomfactor = 1.1892;
    if (_yoffset != 0) {
        float z = pow(zoomfactor, _yoffset);

        // zoom view2d
        glm::vec2 zoom = glm::vec2(z,z);
        glm::vec2 origin = {vera::getWindowWidth()/2, vera::getWindowHeight()/2};
        m_view2d = glm::translate(m_view2d, origin);
        m_view2d = glm::scale(m_view2d, zoom);
        m_view2d = glm::translate(m_view2d, -origin);
        
        vera::flagChange();
    }

    // zoom view3d
    if (uniforms.activeCamera) {
        if (uniforms.activeCamera != uniforms.cameras["default"]) {
            uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
            uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
            uniforms.cameras["default"]->setTarget(uniforms.activeCamera->getTarget());
            
            uniforms.activeCamera = uniforms.cameras["default"];
        }

        float currentDistance = glm::length(uniforms.activeCamera->getPosition() - uniforms.activeCamera->getTarget());
        
        currentDistance = glm::max(currentDistance, 0.01f);

        // Logarithmic zoom for better control
        currentDistance *= pow(0.9f, _yoffset);
        
        if (currentDistance < 0.0f) {
            m_camera_azimuth += 180.0f;
        }

        uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, currentDistance);
    }

}

void GlslViewer::onMouseMove(float _x, float _y) {
    vera::flagChange();
}

void GlslViewer::onMousePress(float _x, float _y, int _button) {
    if (uniforms.activeCamera == nullptr)
        return;

    // Recalculate orbital parameters from current camera state to prevent jumps
    glm::vec3 v = uniforms.activeCamera->getPosition() - uniforms.activeCamera->getTarget();
    float dist = glm::length(v);

    if (dist > 0.001f) {
        // Re-sync target if we drifted (e.g. via WASD fly mode)
        if (glm::dot(glm::normalize(v), uniforms.activeCamera->getZAxis()) < 0.99f) {
            uniforms.activeCamera->setTarget(uniforms.activeCamera->getPosition() - uniforms.activeCamera->getZAxis() * dist);
            v = uniforms.activeCamera->getPosition() - uniforms.activeCamera->getTarget();
        }

        m_camera_elevation = -glm::degrees(asinf(glm::clamp(v.y / dist, -1.0f, 1.0f)));
        m_camera_azimuth = glm::degrees(atan2f(v.x, v.z));
    }
}

void GlslViewer::onMouseDrag(float _x, float _y, int _button) {
     // If no active camera, do nothing
    if (uniforms.activeCamera == nullptr)
        return;

    if (quilt_resolution < 0) {
        // If it's not playing on the HOLOPLAY
        // produce continue draging like blender
        //
        float x = _x;
        float y = _y;
        float pd = vera::getDisplayPixelRatio();

        if (x <= 0) x = vera::getWindowWidth() - 2;
        else if (x >= vera::getWindowWidth()) x = 2; 

        if (y <= 0) y = vera::getWindowHeight() - 2;
        else if (y >= vera::getWindowHeight()) y = 2;

        if (x != _x || y != _y) vera::setMousePosition(x, y);
    }

    bool shiftPressed = vera::isShiftPressed();
    bool ctrlPressed = vera::isControlPressed();

    // Get current camera state
    float currentDistance = glm::length(uniforms.cameras["default"]->getPosition() - uniforms.cameras["default"]->getTarget());
    
    if (uniforms.activeCamera != uniforms.cameras["default"]) {
        uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
        uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
        uniforms.cameras["default"]->setTarget(uniforms.activeCamera->getTarget());
        uniforms.activeCamera = uniforms.cameras["default"];
    }

    float vel_x = vera::getMouseVelX();
    float vel_y = vera::getMouseVelY();

    // Clamp velocity to prevent jumps
    vel_x = glm::clamp(vel_x, -50.0f, 50.0f);
    vel_y = glm::clamp(vel_y, -50.0f, 50.0f);

    if (uniforms.activeCamera->bFlipped) {
        vel_x = -vel_x;
        vel_y = -vel_y;
    }
    
    if (_button == 1 && !shiftPressed && !ctrlPressed) {
        // Left-button drag is used to pan u_view2d.
        m_view2d = glm::translate(m_view2d, glm::vec2(-vera::getMouseVelX(),-vera::getMouseVelY()) );
        
         // Update orbital angles
        m_camera_azimuth -= vel_x * 0.5f;
        m_camera_elevation -= vel_y * 0.5f;
        
        // Clamp elevation to prevent gimbal lock
        m_camera_elevation = glm::clamp(m_camera_elevation, -89.0f, 89.0f);

        uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, currentDistance);
    } 
    else if (_button == 1 && shiftPressed) {
        uniforms.activeCamera->moveTarget(-vel_x * 0.01f, vel_y * 0.01f);
        
    }
    else if (_button == 1 && ctrlPressed) {
        uniforms.activeCamera->move(-vel_x * 0.01f, 0.0f, vel_y * 0.01f);
    }
    else if (_button == 2) {
        // Right-button or Shift+Ctrl+Left: Zoom (change distance)
        float zoomSpeed = currentDistance * 0.01f; // Zoom speed relative to distance
        currentDistance += vel_y * zoomSpeed;
        
        // Clamp distance to reasonable bounds
        currentDistance = glm::clamp(currentDistance, 0.1f, 1000.0f);

        uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, currentDistance);
    }
    else if (_button == 3) {
        uniforms.activeCamera->pan(-vel_x * 0.05f);
        uniforms.activeCamera->tilt(-vel_y * 0.05f);
    }
    
}

void GlslViewer::onWindowResize(int _newWidth, int _newHeight) {
    if (uniforms.activeCamera) {
        if (uniforms.activeCamera != uniforms.cameras["default"]) {
                uniforms.cameras["default"]->setTransformMatrix(uniforms.activeCamera->getTransformMatrix());
                uniforms.cameras["default"]->setProjection(uniforms.activeCamera->getProjectionMatrix());
                
                uniforms.activeCamera = uniforms.cameras["default"];
            }
        uniforms.activeCamera->setViewport(_newWidth, _newHeight);
    }
    
    for (size_t i = 0; i < uniforms.buffers.size(); i++) 
        if (uniforms.buffers[i]->scale > 0.0)
            uniforms.buffers[i]->allocate(  _newWidth * uniforms.buffers[i]->scale, 
                                            _newHeight * uniforms.buffers[i]->scale, 
                                            vera::COLOR_FLOAT_TEXTURE);

    for (size_t i = 0; i < uniforms.doubleBuffers.size(); i++) {
        if (uniforms.doubleBuffers[i]->buffer(0).scale > 0.0 || uniforms.doubleBuffers[i]->buffer(1).scale > 0.0) {
            uniforms.doubleBuffers[i]->buffer(0).allocate(  _newWidth * uniforms.doubleBuffers[i]->buffer(0).scale, 
                                                            _newHeight * uniforms.doubleBuffers[i]->buffer(0).scale, 
                                                            vera::COLOR_FLOAT_TEXTURE);
            uniforms.doubleBuffers[i]->buffer(1).allocate(  _newWidth * uniforms.doubleBuffers[i]->buffer(1).scale, 
                                                            _newHeight * uniforms.doubleBuffers[i]->buffer(1).scale, 
                                                            vera::COLOR_FLOAT_TEXTURE);
        }
    }

    for (size_t i = 0; i < uniforms.pyramids.size(); i++) {
        if (m_pyramid_fbos[i].scale > 0.0) {
            m_pyramid_fbos[i].allocate( _newWidth * m_pyramid_fbos[i].scale, 
                                        _newHeight * m_pyramid_fbos[i].scale, 
                                        vera::COLOR_FLOAT_TEXTURE);

            uniforms.pyramids[i].allocate(  _newWidth * m_pyramid_fbos[i].scale, 
                                            _newHeight * m_pyramid_fbos[i].scale);
        }
    }

    for (size_t i = 0; i < uniforms.floods.size(); i++) {
        if (uniforms.floods[i].scale > 0.0) {
            uniforms.floods[i].allocate( _newWidth * uniforms.floods[i].scale, 
                                         _newHeight * uniforms.floods[i].scale, 
                                         vera::COLOR_FLOAT_TEXTURE);
        }
    }

    if (quilt_resolution >= 0)
        m_sceneRender.updateBuffers(uniforms, vera::getQuiltWidth(), vera::getQuiltHeight());
    else 
        m_sceneRender.updateBuffers(uniforms, _newWidth, _newHeight);

    if (screenshotFile != "" || isRecording()) 
        m_record_fbo.allocate(_newWidth, _newHeight, vera::COLOR_TEXTURE_DEPTH_BUFFER);

    vera::flagChange();
    m_change_viewport = true;
}

void GlslViewer::onScreenshot(std::string _file) {
    #if defined(PYTHON_RENDER)
    if (_file != "") {
    #else
    if (_file != "" && vera::isGL()) {
    #endif
        TRACK_BEGIN("screenshot")

        glBindFramebuffer(GL_FRAMEBUFFER, m_record_fbo.getId());

        if (vera::getExt(_file) == "hdr") {
            float* pixels = new float[vera::getWindowWidth() * vera::getWindowHeight()*4];
            glReadPixels(0, 0, vera::getWindowWidth(), vera::getWindowHeight(), GL_RGBA, GL_FLOAT, pixels);
            vera::savePixelsFloat(_file, pixels, vera::getWindowWidth(), vera::getWindowHeight());
        }
        #if defined(SUPPORT_LIBAV) && !defined(PLATFORM_RPI)
        else if (recordingPipe()) {
            int width = vera::getWindowWidth();
            int height = vera::getWindowHeight();
            auto pixels = std::unique_ptr<unsigned char[]>(new unsigned char [width * height * 3]);
            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.get());
            recordingPipeFrame( std::move(pixels) );
        }
        #endif
        else {
            int width = vera::getWindowWidth();
            int height = vera::getWindowHeight();
            auto pixels = std::unique_ptr<unsigned char[]>(new unsigned char [width * height * 4]);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

            #if defined(SUPPORT_MULTITHREAD_RECORDING) && !defined(PYTHON_RENDER)
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
                auto func = [saverPtr]() {
                    Job& saver = *saverPtr;
                    saver();
                };
                m_save_threads.Submit(std::move(func));
            }
            #else

            vera::savePixels(_file, pixels.get(), width, height);
            if (vera::getExt(_file) == "png" || 
                vera::getExt(_file) == "jpg" || vera::getExt(_file) == "jpeg")
                m_postprocessing_shader.addDefinesTo(_file);
            
            #endif

        }
    
        if ( !isRecording() )
            std::cout << "Screenshot saved to " << _file << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        TRACK_END("screenshot")

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    }
}

void GlslViewer::onPlot() {
    // if ( !vera::isGL() )
    //     return;
    if (!m_sceneRender.renderFbo.isAllocated())
        return;

    if ( (m_plot == PLOT_LUMA || m_plot == PLOT_RGB || m_plot == PLOT_RED || m_plot == PLOT_GREEN || m_plot == PLOT_BLUE ) && haveChange() ) {
        TRACK_BEGIN("plot::histogram")


        // Extract pixels
        glBindFramebuffer(GL_FRAMEBUFFER, m_sceneRender.renderFbo.getId());
        int w = vera::getWindowWidth();
        int h = vera::getWindowHeight();
        int c = 3;
        int total = w * h * c;
        unsigned char* pixels = new unsigned char[total];
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Count frequencies of appearances 
        float max_rgb_freq = 0;
        float max_luma_freq = 0;
        glm::vec4 freqs[256];
        for (int i = 0; i < total; i += c) {
            m_plot_values[pixels[i]].r++;
            if (m_plot_values[pixels[i]].r > max_rgb_freq)
                max_rgb_freq = m_plot_values[pixels[i]].r;

            m_plot_values[pixels[i+1]].g++;
            if (m_plot_values[pixels[i+1]].g > max_rgb_freq)
                max_rgb_freq = m_plot_values[pixels[i+1]].g;

            m_plot_values[pixels[i+2]].b++;
            if (m_plot_values[pixels[i+2]].b > max_rgb_freq)
                max_rgb_freq = m_plot_values[pixels[i+2]].b;

            int luma = 0.299 * pixels[i] + 0.587 * pixels[i+1] + 0.114 * pixels[i+2];
            m_plot_values[luma].a++;
            if (m_plot_values[luma].a > max_luma_freq)
                max_luma_freq = m_plot_values[luma].a;
        }
        delete[] pixels;

        // Normalize frequencies
        for (int i = 0; i < 256; i ++)
            m_plot_values[i] = m_plot_values[i] / glm::vec4(max_rgb_freq, max_rgb_freq, max_rgb_freq, max_luma_freq);

        if (m_plot_texture == nullptr)
            m_plot_texture = new vera::Texture();
        m_plot_texture->load(256, 1, 4, 32, &m_plot_values[0], vera::NEAREST, vera::CLAMP);

        uniforms.textures["u_histogram"] = m_plot_texture;
        uniforms.flagChange();
        TRACK_END("plot::histogram")
    }

    else if (m_plot == PLOT_FPS ) {
        TRACK_BEGIN("plot::fps")

        // Push back values one position
        for (int i = 0; i < 255; i ++)
            m_plot_values[i] = m_plot_values[i+1];
        m_plot_values[255] = glm::vec4( vera::getFps()/60.0f, 0.0f, 0.0f, 1.0f);

        if (m_plot_texture == nullptr)
            m_plot_texture = new vera::Texture();

        m_plot_texture->load(256, 1, 4, 32, &m_plot_values[0], vera::NEAREST, vera::CLAMP);
        // uniforms.textures["u_sceneFps"] = m_plot_texture;

        TRACK_END("plot::fps")
    }

    else if (m_plot == PLOT_MS ) {
        TRACK_BEGIN("plot::ms")

        // Push back values one position
        for (int i = 0; i < 255; i ++)
            m_plot_values[i] = m_plot_values[i+1];
        m_plot_values[255] = glm::vec4( vera::getDelta(), 0.0f, 0.0f, 1.0f);

        if (m_plot_texture == nullptr)
            m_plot_texture = new vera::Texture();

        m_plot_texture->load(256, 1, 4, 32, &m_plot_values[0], vera::NEAREST, vera::CLAMP);

        // uniforms.textures["u_sceneMs"] = m_plot_texture;

        TRACK_END("plot::ms")
    }
}
