#include "sandbox.h"

#include <sys/stat.h>   // stat
#include <algorithm>    // std::find
#include <numeric>
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
Sandbox::Sandbox(): 
    screenshotFile(""), lenticular(""), quilt(-1), 
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
    m_view2d(1.0), m_time_offset(0.0), m_camera_elevation(1.0), m_camera_azimuth(180.0), m_error_screen(vera::SHOW_MAGENTA_SHADER), 
    m_change(true), m_change_viewport(true), m_update_buffers(true), m_initialized(false), 

    // Debug
    m_showTextures(false), m_showPasses(false)
{
    // set vera scene values to uniforms
    vera::setScene( (vera::Scene*)&uniforms );

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
        _shader.setUniform("u_mouse", float(vera::getMouseX()), float(vera::getMouseY()));
    },
    []() { return vera::toString(vera::getMouseX(),1) + "," + vera::toString(vera::getMouseY(),1); } );

    // VIEWPORT
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

Sandbox::~Sandbox() {
    #if defined(SUPPORT_MULTITHREAD_RECORDING)
    /** make sure every frame is saved before exiting **/
    if (m_task_count > 0)
        std::cout << "saving remaining frames to disk, this might take a while ..." << std::endl;
    
    while (m_task_count > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    
    #endif
}

// ------------------------------------------------------------------------- SET

void Sandbox::commandsInit(CommandList &_commands ) {

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
            std::cout << vera::getVersion() << std::endl;
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
            std::cout << "plot," << plot_options[m_plot] << std::endl; 
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
    "textures[,on|off]", "return a list of textures as their uniform name and path. Or show/hide textures on viewport.", false));

    _commands.push_back(Command("buffer", [&](const std::string& _line){ 
        if (_line == "buffers") {
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
    "buffers[,show|hide]", "return a list of buffers as their uniform name. Or show/hide buffer on viewport.", false));

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
        if (_line == "cubemaps") {
            uniforms.printCubemaps();
            return true;
        } else if (_line == "cubemap") {
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
            std::string rta = m_sceneRender.showCubebox ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = vera::split(_line,',');
            if (values.size() == 2) {
                if (values[1] == "toggle")
                    values[1] = m_sceneRender.showCubebox ? "off" : "on";

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
            uniforms.activeCamera->setDistance(vera::toFloat(values[1]));
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
            uniforms.activeCamera->setFOV( vera::toFloat(values[1]) );
            return true;
        }
        else {
            std::cout << uniforms.activeCamera->getFOV() << std::endl;
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
            uniforms.activeCamera->setPosition( -glm::vec3(vera::toFloat(values[1]), vera::toFloat(values[2]), vera::toFloat(values[3])));
            uniforms.activeCamera->lookAt( uniforms.activeCamera->getTarget() );
            glm::vec3 v = uniforms.activeCamera->getPosition();
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
    "streams[,stop|play|restart|speed|prevs[,<value>]]", "print all streams or get/set streams speed and previous frames"));

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

void Sandbox::loadAssets(WatchFileList &_files) {
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
        else
            m_frag_source = vera::getDefaultSrc(vera::FRAG_DEFAULT_SCENE);
    }

    if (vert_index != -1) {
        // If there is a Vertex shader load it
        m_vert_source = "";
        m_vert_dependencies.clear();

        vera::loadGlslFrom(_files[vert_index].path, &m_vert_source, include_folders, &m_vert_dependencies);
    }
    else {
        // If there is no use the default one
        if (geom_index == -1)
            m_vert_source = vera::getDefaultSrc(vera::VERT_DEFAULT);
        else
            m_vert_source = vera::getDefaultSrc(vera::VERT_DEFAULT_SCENE);
    }

    // LOAD GEOMETRY
    // -----------------------------------------------
    if (geom_index != -1) {
        uniforms.load(_files[geom_index].path, verbose);
        m_sceneRender.loadScene(uniforms);
        uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, m_sceneRender.getArea() * 2.0);
    }
    else {
        m_canvas_shader.addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");
        uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, 2.0);
    }

    uniforms.activeCamera->lookAt( uniforms.activeCamera->getTarget() );

    // FINISH SCENE SETUP
    // -------------------------------------------------
    uniforms.activeCamera->setViewport(vera::getWindowWidth(), vera::getWindowHeight());
    
    if (lenticular.size() > 0)
        vera::setLenticularProperties(lenticular);

    if (quilt >= 0) {
        vera::setQuiltProperties(quilt);
        addDefine("QUILT", vera::toString(quilt));
        addDefine("QUILT_WIDTH", vera::toString( vera::getQuiltWidth() ));
        addDefine("QUILT_HEIGHT", vera::toString( vera::getQuiltHeight() ));
        addDefine("QUILT_COLUMNS", vera::toString( vera::getQuiltColumns() ));
        addDefine("QUILT_ROWS", vera::toString( vera::getQuiltRows() ));
        addDefine("QUILT_TOTALVIEWS", vera::toString( vera::getQuiltTotalViews() ));

        uniforms.activeCamera->setFOV(glm::radians(14.0f));
        uniforms.activeCamera->setProjection(vera::ProjectionType::PERSPECTIVE_VIRTUAL_OFFSET);
        // uniforms.activeCamera->setClipping(0.01, 100.0);

        if (geom_index != -1)
            uniforms.activeCamera->orbit(m_camera_elevation, m_camera_azimuth, m_sceneRender.getArea() * 8.5);

        if (lenticular.size() == 0)
            vera::setWindowSize(vera::getQuiltWidth(), vera::getQuiltHeight());
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

    addDefine("GLSLVIEWER", vera::toString(GLSLVIEWER_VERSION_MAJOR) + vera::toString(GLSLVIEWER_VERSION_MINOR) + vera::toString(GLSLVIEWER_VERSION_PATCH) );
    if (uniforms.activeCubemap) {
        addDefine("SCENE_SH_ARRAY", "u_SH");
        addDefine("SCENE_CUBEMAP", "u_cubeMap");
    }

    // LOAD SHADERS
    resetShaders( _files );
    flagChange();
}

void Sandbox::loadModel(vera::Model* _model) {
    _model->setShader(m_frag_source, m_vert_source);

    uniforms.models[_model->getName()] = _model;
    m_sceneRender.loadScene(uniforms);
    uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, m_sceneRender.getArea() * 2.0);
    uniforms.activeCamera->lookAt(uniforms.activeCamera->getTarget());
}

void Sandbox::addDefine(const std::string &_define, const std::string &_value) {
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
}

void Sandbox::delDefine(const std::string &_define) {
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
    m_change_viewport = false;
    m_sceneRender.unflagChange();
    uniforms.unflagChange();
}

bool Sandbox::haveChange() { 
    return  m_change ||
            isRecording() ||
            screenshotFile != "" ||
            m_sceneRender.haveChange() ||
            uniforms.haveChange();
}

const std::string& Sandbox::getSource(ShaderType _type) const {
    return (_type == FRAGMENT)? m_frag_source : m_vert_source;
}

void Sandbox::setFrame(size_t _frame) { 
    uniforms.setFrame(_frame);
}

// ------------------------------------------------------------------------- RELOAD SHADER

void Sandbox::setSource(ShaderType _type, const std::string& _source) {
    if (_type == FRAGMENT) {
        m_frag_dependencies.clear();
        m_frag_source = vera::resolveGlsl(_source, include_folders, &m_frag_dependencies);
    }
    else {
        m_vert_dependencies.clear();
        m_vert_source = vera::resolveGlsl(_source, include_folders, &m_vert_dependencies);;
    }
};

void Sandbox::resetShaders( WatchFileList &_files ) {

    if (vera::getWindowStyle() != vera::EMBEDDED)
        console_clear();
        
    flagChange();

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
    bool havePostprocessing = checkPostprocessing( getSource(FRAGMENT) );
    if (havePostprocessing) {
        // Specific defines for this buffer
        m_postprocessing_shader.addDefine("POSTPROCESSING");
        m_postprocessing_shader.setSource(m_frag_source, vera::getDefaultSrc(vera::VERT_BILLBOARD));
        uniforms.functions["u_scene"].present = havePostprocessing;
        m_postprocessing = havePostprocessing;
    }
    else if (lenticular.size() > 0) {
        m_postprocessing_shader.setSource(vera::getLenticularFragShader(vera::getVersion()), vera::getDefaultSrc(vera::VERT_BILLBOARD));
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
void Sandbox::_updateBuffers() {
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

                vera::getBillboard()->render( &m_pyramid_shader );
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

                vera::getBillboard()->render( &m_flood_shader );
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
        if (quilt >= 0)
            m_sceneRender.updateBuffers(uniforms, vera::getQuiltWidth(), vera::getQuiltHeight());
        else 
            m_sceneRender.updateBuffers(uniforms, vera::getWindowWidth(), vera::getWindowHeight());
    }

}

// ------------------------------------------------------------------------- DRAW
void Sandbox::_renderBuffers() {
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

        vera::getBillboard()->render( &m_buffers_shaders[i] );
        
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

        vera::getBillboard()->render( &m_doubleBuffers_shaders[i] );
        
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

        vera::getBillboard()->render( &m_pyramid_subshaders[i] );

        m_pyramid_fbos[i].unbind();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

        vera::getBillboard()->render( &m_flood_subshaders[i] );

        uniforms.floods[i].dst->unbind();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Sandbox::renderPrep() {
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

void Sandbox::render() {
    // RENDER CONTENT
    if (uniforms.models.size() == 0) {
        TRACK_BEGIN("render:2D_scene")

        // Load main shader
        m_canvas_shader.use();

        if (quilt >= 0) {
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
                vera::getBillboard()->render( &m_canvas_shader );
            }, true);
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
            vera::getBillboard()->render( &m_canvas_shader );
        }

        TRACK_END("render:2D_scene")
    }

    else {
        TRACK_BEGIN("render:3D_scene")
        if (quilt >= 0) {
            vera::renderQuilt([&](const vera::QuiltProperties& quilt, glm::vec4& viewport, int &viewIndex){

                // set up the camera rotation and position for current view
                uniforms.activeCamera->setVirtualOffset(m_sceneRender.getArea() * 0.75, viewIndex, quilt.totalViews);
                // uniforms.activeCamera->setVirtualOffset(5.0f, viewIndex, quilt.totalViews);
                // uniforms.activeCamera->setVirtualOffset(10.0f, viewIndex, quilt.totalViews);

                uniforms.set("u_tile", float(quilt.columns), float(quilt.rows), float(quilt.totalViews));
                uniforms.set("u_viewport", float(viewport.x), float(viewport.y), float(viewport.z), float(viewport.w));

                m_sceneRender.render(uniforms);

                if (m_sceneRender.showGrid || m_sceneRender.showAxis || m_sceneRender.showBBoxes)
                    m_sceneRender.renderDebug(uniforms);
            }, true);
        }

        else {
            m_sceneRender.render(uniforms);
            if (m_sceneRender.showGrid || m_sceneRender.showAxis || m_sceneRender.showBBoxes)
                m_sceneRender.renderDebug(uniforms);
        }
        TRACK_END("render:3D_scene")
    }
}

void Sandbox::renderPost() {
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

        vera::getBillboard()->render( &m_postprocessing_shader );

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

namespace {
namespace renderable_objects {
struct render_ui_t {
    glm::vec2 dimensions = {vera::getWindowWidth(), vera::getWindowHeight()};
    float scale = 1;
    glm::vec2 step = dimensions * scale;
    glm::vec2 offset = dimensions - step;
    float p = vera::getPixelDensity();
    glm::vec2 pos = dimensions * glm::vec2{0.5, 1} + glm::vec2{0, 10};
};

void print_text(const std::string& prompt, const float offsetx, render_ui_t& uio) {
    vera::text(prompt, offsetx, vera::getWindowHeight() - uio.offset.y + uio.step.y);
    uio.offset.y -= uio.step.y * 2.0;
}

void print_fbo_text(const vera::Fbo& lala, const std::string& prompt, render_ui_t& uio) {
    vera::image(&lala, uio.offset.x, uio.offset.y, uio.step.x, uio.step.y);
    print_text(prompt, uio.offset.x - uio.step.x, uio);
}

void print_buffers_text(const vera::Fbo& uniforms_buffer, size_t i, const std::string& prompt, render_ui_t& uio) {
    glm::vec2 scale = uio.step.y * glm::vec2{((float)uniforms_buffer.getWidth()/(float)uniforms_buffer.getHeight()), 1};
    glm::vec2 offset = uio.offset + glm::vec2{uio.step.x - scale.x, 0};
    vera::image(uniforms_buffer, offset.x, offset.y, scale.x, scale.y);
    print_text(prompt + vera::toString(i), uio.offset.x - scale.x, uio);
}

namespace render_pass_actions {
struct render_pass_args_t {
    const vera::Fbo* const fbo;
    const BuffersList* const fbolist;
};

void do_pass_scene(const std::string& prompt_id, Uniforms& uniforms, const render_pass_args_t& abc, render_ui_t& uio) {
    if (uniforms.functions[prompt_id].present)
        print_fbo_text(*abc.fbo, prompt_id, uio);
}

void do_pass_scenebuffer(const std::string& prompt_id, Uniforms& , const render_pass_args_t& abc, render_ui_t& uio) {
    for (size_t i = 0; i < abc.fbolist->size(); i++)
        print_fbo_text(*(*abc.fbolist)[i], prompt_id, uio);
}

void do_pass_scenedepth(const std::string& prompt_id, Uniforms& uniforms, const render_pass_args_t& abc, render_ui_t& uio) {
    if (uniforms.functions[prompt_id].present) {
        if (uniforms.activeCamera)
            vera::imageDepth(*abc.fbo, uio.offset.x, uio.offset.y, uio.step.x, uio.step.y, uniforms.activeCamera->getFarClip(), uniforms.activeCamera->getNearClip());
        print_text(prompt_id, uio.offset.x - uio.step.x, uio);
    }
}

void do_pass_lightmap(const std::string& prompt_id, Uniforms& uniforms, const render_pass_args_t&, render_ui_t& uio) {
    if (uniforms.models.size() > 0) {
        for (vera::LightsMap::iterator it = uniforms.lights.begin(); it != uniforms.lights.end(); ++it ) {
            if ( it->second->getShadowMap()->getDepthTextureId() ) {
                vera::imageDepth(it->second->getShadowMap(), uio.offset.x, uio.offset.y, uio.step.x, uio.step.y, it->second->getShadowMapFar(), it->second->getShadowMapNear());
                // vera::image(it->second->getShadowMap(), xOffset, yOffset, xStep, yStep);
                print_text(prompt_id, uio.offset.x - uio.step.x, uio);
            }
        }
    }
}

void do_pass_pyramid(const std::string&, Uniforms& uniforms, const render_pass_args_t&, render_ui_t& uio) {
    for (size_t i = 0; i < uniforms.pyramids.size(); i++) {
        glm::vec2 scale = uio.step.y * glm::vec2{((float)uniforms.pyramids[i].getWidth()/(float)uniforms.pyramids[i].getHeight()), 1};
        glm::vec2 offset = uio.offset  + glm::vec2{uio.step.x - scale.x, 0};
        const auto w = scale.x;
        for (size_t j = 0; j < uniforms.pyramids[i].getDepth() * 2; j++ ) {
            const auto is_lower_depth = (j < uniforms.pyramids[i].getDepth());
            const auto delta_offset = is_lower_depth ? offset.x : offset.x + w * 2.0f;
            vera::image(uniforms.pyramids[i].getResult(j), delta_offset, offset.y, scale.x, scale.y);
            offset.x -= scale.x;
            std::tie(scale, offset.y) = is_lower_depth
                                        ? std::pair<glm::vec2, float>{scale *= 0.5, uio.offset.y - uio.step.y * 0.5}
                                        : std::pair<glm::vec2, float>{scale *= 2.0, uio.offset.y + uio.step.y * 0.5};

            offset.x -= scale.x;
        }
        // vera::text("u_pyramid0" + vera::toString(i), xOffset - scale.x * 2.0, vera::getWindowHeight() - yOffset + yStep);
        uio.offset.y -= uio.step.y * 2.0;
    }
}

void do_pass_doublebuffers(const std::string& prompt_id, Uniforms& uniforms, const render_pass_args_t&, render_ui_t& uio) {
    for (size_t i = 0; i < uniforms.doubleBuffers.size(); i++)
        print_buffers_text(*uniforms.doubleBuffers[i]->src, i, prompt_id, uio);
}

void do_pass_singlebuffer(const std::string& prompt_id, Uniforms& uniforms, const render_pass_args_t&, render_ui_t& uio) {
    for (size_t i = 0; i < uniforms.buffers.size(); i++)
        print_buffers_text(*uniforms.buffers[i], i, prompt_id, uio);
}
} // namespace [render_pass_actions]

void set_common_text_attributes(float textangle, float textsize, vera::VerticalAlign v, vera::HorizontalAlign h){
    vera::textAngle(textangle);
    vera::textSize(textsize);
    vera::textAlign(v);
    vera::textAlign(h);
}

void process_render_passes(Uniforms& uniforms, const SceneRender& m_sceneRender, int nTotal){
    using namespace render_pass_actions;
    render_ui_t uio;
    uio.scale = fmin(1.0f / (float)(nTotal), 0.25) * 0.5;
    uio.step = uio.dimensions * uio.scale;
    uio.offset = uio.dimensions - uio.step;

    set_common_text_attributes(-HALF_PI, uio.step.y * 0.2f / vera::getPixelDensity(false), vera::ALIGN_BOTTOM, vera::ALIGN_LEFT);

    struct vtable_render_pass_t{
        using func_sig_t = auto (*)(const std::string&, Uniforms&, const render_pass_args_t&, render_ui_t&)-> void;
        const std::string prompt_id;
        const render_pass_args_t process_info;
        const func_sig_t process_renderer;
    };
    const std::array<vtable_render_pass_t, 9> somelist { vtable_render_pass_t
        {"u_buffer", {nullptr, nullptr}, do_pass_singlebuffer}
        , {"u_doubleBuffer", {nullptr, nullptr}, do_pass_doublebuffers}
        , {"u_pyramid0", {nullptr, nullptr}, do_pass_pyramid}
        , {"u_lightShadowMap", {nullptr, nullptr}, do_pass_lightmap}
        , {"u_scenePosition", {&m_sceneRender.positionFbo, nullptr}, do_pass_scene}
        , {"u_sceneNormal", {&m_sceneRender.normalFbo, nullptr}, do_pass_scene}
        , {"u_sceneBuffer", {nullptr, &m_sceneRender.buffersFbo}, do_pass_scenebuffer}
        , {"u_scene", {&m_sceneRender.renderFbo, nullptr}, do_pass_scene}
        , {"u_sceneDepth", {&m_sceneRender.renderFbo, nullptr}, do_pass_scenedepth}
    };
    for(const auto& _ : somelist) { _.process_renderer(_.prompt_id, uniforms, _.process_info, uio); }
}

namespace overlay_actions {
struct overlay_fn_args_t {
    Uniforms * uniforms;
    const SceneRender* m_sceneRender;
    const bool* m_postprocessing;
    vera::Shader* m_plot_shader;
    vera::Texture** m_plot_texture;
    std::unique_ptr<vera::Vbo>* m_cross_vbo;
    const int* geom_index;
    const bool* help;
    const bool* verbose;
};

void overlay_m_showTextures(const overlay_fn_args_t& o) {
    if (o.uniforms->textures.size() > 0) {
        glDisable(GL_DEPTH_TEST);
        TRACK_BEGIN("renderUI:textures")
        render_ui_t uio;
        uio.scale = fmin(1.0f / (float)(o.uniforms->textures.size()), 0.25) * 0.5;
        uio.step = uio.dimensions * uio.scale;
        uio.offset = {uio.step.x, uio.dimensions.y - uio.step.y};

        set_common_text_attributes(-HALF_PI, uio.step.y * 0.2f / vera::getPixelDensity(false), vera::ALIGN_TOP, vera::ALIGN_LEFT);

        for(const auto& texture : o.uniforms->textures) {
            const auto textureStream_match = std::find_if(std::begin(o.uniforms->streams), std::end(o.uniforms->streams)
                                           , [&](vera::TextureStreamsMap::value_type stream){return texture.first == stream.first;});
            if ( textureStream_match != std::end(o.uniforms->streams) )
                vera::image((vera::TextureStream*)textureStream_match->second, uio.offset.x, uio.offset.y, uio.step.x, uio.step.y, true);
            else
                vera::image(texture.second, uio.offset.x, uio.offset.y, uio.step.x, uio.step.y);
            print_text(texture.first, uio.offset.x + uio.step.x, uio);
        }
        TRACK_END("renderUI:textures")
    }
}

void overlay_m_showPasses(const overlay_fn_args_t& o) {
    glDisable(GL_DEPTH_TEST);
    TRACK_BEGIN("renderUI:buffers")

    // DEBUG BUFFERS
    const auto is_postprocessing_with_uniforms = o.m_postprocessing
                                                 && o.uniforms->models.size() > 0;
    using num_of_passes_t = std::pair<bool, size_t>;
    const auto nTotalArray = { num_of_passes_t
        {true, o.uniforms->buffers.size()} //buffer
        , {true, o.uniforms->doubleBuffers.size()}    // doublebuffer
        , {true, o.uniforms->pyramids.size()} //pyramid
        , {is_postprocessing_with_uniforms, 1}  // lightmap
        , {true, o.uniforms->functions["u_scenePosition"].present}
        , {true, o.uniforms->functions["u_sceneNormal"].present}
        , {true, o.m_sceneRender->getBuffersTotal()}
        , {is_postprocessing_with_uniforms, o.uniforms->functions["u_scene"].present}
        , {is_postprocessing_with_uniforms, o.uniforms->functions["u_sceneDepth"].present}
    };
    const auto nTotal = std::accumulate(std::begin(nTotalArray), std::end(nTotalArray), int{}
                                        , [](const int acc, const num_of_passes_t& kv) { return acc + ((kv.first) ? kv.second : 0); });
    if (nTotal > 0) {
        process_render_passes(*o.uniforms, *o.m_sceneRender, nTotal);
    }
    TRACK_END("renderUI:buffers")
};

void overlay_m_plot(const overlay_fn_args_t& o) {
    glDisable(GL_DEPTH_TEST);
    TRACK_BEGIN("renderUI:plot_data")

    render_ui_t uio;
    uio.dimensions = glm::vec2{100, 30} * uio.p;
    uio.pos = {(float)(vera::getWindowWidth()) * 0.5, uio.dimensions.y + 10};

    o.m_plot_shader->use();
    o.m_plot_shader->setUniform("u_scale", uio.dimensions);
    o.m_plot_shader->setUniform("u_translate", uio.pos);
    o.m_plot_shader->setUniform("u_resolution", {vera::getWindowWidth(), vera::getWindowHeight()});
    o.m_plot_shader->setUniform("u_viewport", uio.dimensions);
    o.m_plot_shader->setUniform("u_model", glm::vec3(1.0f));
    o.m_plot_shader->setUniform("u_modelMatrix", glm::mat4(1.0f));
    o.m_plot_shader->setUniform("u_viewMatrix", glm::mat4(1.0f));
    o.m_plot_shader->setUniform("u_projectionMatrix", glm::mat4(1.0f));
    o.m_plot_shader->setUniform("u_modelViewProjectionMatrix", vera::getOrthoMatrix());
    o.m_plot_shader->setUniformTexture("u_plotData", *o.m_plot_texture, 0);

    vera::getBillboard()->render(&*o.m_plot_shader);
    TRACK_END("renderUI:plot_data")
}

void overlay_cursor(const overlay_fn_args_t& o) {
    TRACK_BEGIN("renderUI:cursor")
    if ((*o.m_cross_vbo) == nullptr)
        (*o.m_cross_vbo) = std::unique_ptr<vera::Vbo>(new vera::Vbo( vera::crossMesh( glm::vec3(0.0f, 0.0f, 0.0f), 10.0f) ));

    vera::Shader* fill = vera::getFillShader();
    fill->use();
    fill->setUniform("u_modelViewProjectionMatrix", glm::translate(vera::getOrthoMatrix(), glm::vec3(vera::getMouseX(), vera::getMouseY(), 0.0f) ) );
    fill->setUniform("u_color", glm::vec4(1.0f));
    (*o.m_cross_vbo)->render(fill);
    TRACK_END("renderUI:cursor")
}

vera::Camera* overlay_black_box(float textangle, float textsize, vera::VerticalAlign v, vera::HorizontalAlign h, render_ui_t& uio) {
    uio.step = uio.dimensions * 0.05f;
    uio.pos = uio.step * glm::vec2{2.0f, 3.0f};

    vera::Camera *cam = vera::getCamera();
    vera::resetCamera();

    vera::fill(0.0f, 0.0f, 0.0f, 0.75f);
    vera::noStroke();
    vera::rect(uio.dimensions * 0.5f, uio.dimensions - uio.step * 2.0f);
    vera::fill(1.0f);
    set_common_text_attributes(textangle, textsize, v, h);
    return cam;
}

void overlay_prompt_drag_and_drop(const overlay_fn_args_t&) {
    render_ui_t uio;
    vera::Camera *cam = overlay_black_box(0.0f, 38.0f, vera::ALIGN_MIDDLE, vera::ALIGN_CENTER, uio);

    vera::text("Drag & Drop", uio.dimensions.x * 0.5f, uio.dimensions.y * 0.45f);
    vera::textSize(22.0f);
    vera::text(".vert .frag .ply .lst .obj .gltf .glb", uio.dimensions.x * 0.5f, uio.dimensions.y * 0.55f);

    vera::setCamera(cam);
}

void overlay_prompt_help(const overlay_fn_args_t& o) {
    render_ui_t uio;
    vera::Camera *cam = overlay_black_box(0.0f, 22.0f, vera::ALIGN_MIDDLE, vera::ALIGN_LEFT, uio);
    uio.step.y = vera::getFontHeight() * 1.5f;

    const auto print_text = [&](const std::string& prompt){
        vera::text(prompt, uio.pos);
        uio.pos.y += uio.step.y;
    };

    const auto geometry_available = *o.geom_index != -1;
    const auto uniform_streams_available = o.uniforms->streams.size() > 0;

    struct help_prompt_t {
        bool predicate;
        std::string message;
    };
    const auto help_prompts = { help_prompt_t
        {geometry_available, "a - " + std::string( o.m_sceneRender->showAxis? "hide" : "show" ) + " axis"}
        , {geometry_available, "b - " + std::string( o.m_sceneRender->showBBoxes? "hide" : "show" ) + " bounding boxes"}
        , {true, "c - hide/show cursor"}
        , {geometry_available, "d - " + std::string( o.m_sceneRender->dynamicShadows? "disable" : "enable" ) + " dynamic shadows"}
        , {geometry_available, "f - hide/show floor"}
        , {true, "F - " + std::string( vera::isFullscreen() ? "disable" : "enable" ) + " fullscreen"}
        , {geometry_available, "g - " + std::string( o.m_sceneRender->showGrid? "hide" : "show" ) + " grid"}
        , {true, "h - " + std::string( o.help? "hide" : "show" ) + " help"}
        , {true, "i - hide/show extra info"}
        , {true, "o - open shaders on default editor"}
        , {true, "p - hide/show render passes/buffers"}
        , {uniform_streams_available, "r - restart stream textures"}
        , {geometry_available, "s - hide/show sky"}
        , {true, "t - hide/show loaded textures"}
        , {true, "v - " + std::string( o.verbose? "disable" : "enable" ) + " verbose"}
        , {uniform_streams_available, "space - start/stop stream textures"}
    };
    for(const auto& _ : help_prompts) { if(_.predicate) print_text(_.message); }

    vera::setCamera(cam);
    glDisable(GL_DEPTH_TEST);
}
} // namespace [overlay_actions]
} // namespace [renderable_objects]
} // namespace [_]

void Sandbox::renderUI() {
    TRACK_BEGIN("renderUI")
    using namespace renderable_objects::overlay_actions;

    const auto display_m_plots = m_plot != PLOT_OFF && m_plot_texture ;
    const auto diplay_cursor = cursor && vera::getMouseEntered();
    const auto no_geometry_available = frag_index == -1 && vert_index == -1 && geom_index == -1;

    struct vtable_overlay_fn_args_with_pred_t {
        using function_sig_t = auto (*)(const overlay_fn_args_t&) -> void;
        const bool predicate;
        const function_sig_t do_overlay_action;
        const overlay_fn_args_t parameters;
    };
    const std::array<vtable_overlay_fn_args_with_pred_t, 6> lala = { vtable_overlay_fn_args_with_pred_t
        {m_showTextures, &overlay_m_showTextures, {&uniforms, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}}
        , {m_showPasses, &overlay_m_showPasses, {&uniforms, &m_sceneRender, &m_postprocessing, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}}
        , {display_m_plots, &overlay_m_plot, {nullptr, nullptr, nullptr, &m_plot_shader, &m_plot_texture, nullptr, nullptr, nullptr, nullptr}}
        , {diplay_cursor, &overlay_cursor, {nullptr, nullptr, nullptr, nullptr, nullptr, &m_cross_vbo, nullptr, nullptr, nullptr}}
        , {no_geometry_available, &overlay_prompt_drag_and_drop, {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}}
        , {help, &overlay_prompt_help, {&uniforms, &m_sceneRender, nullptr, nullptr, nullptr, nullptr, &geom_index, &help, &verbose}}
    };
    for(const auto& _: lala) { if(_.predicate) _.do_overlay_action(_.parameters);  }

    TRACK_END("renderUI")
}

void Sandbox::renderDone() {
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

    unflagChange();

    if (m_plot != PLOT_OFF)
        onPlot();

    if (!m_initialized) {
        m_initialized = true;
        vera::updateViewport();
        flagChange();
    }

    TRACK_END("update:post_render")
}

// ------------------------------------------------------------------------- ACTIONS
void Sandbox::printDependencies(ShaderType _type) const {
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

void Sandbox::onFileChange(WatchFileList &_files, int index) {
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
        glm::vec2 origin = {vera::getWindowWidth()/2, vera::getWindowHeight()/2};
        m_view2d = glm::translate(m_view2d, origin);
        m_view2d = glm::scale(m_view2d, zoom);
        m_view2d = glm::translate(m_view2d, -origin);
        
        flagChange();
    }
}

void Sandbox::onMouseDrag(float _x, float _y, int _button) {
    if (uniforms.activeCamera == nullptr)
        return;

    if (quilt < 0) {
        // If it's not playing on the HOLOPLAY
        // produce continue draging like blender
        //
        float x = _x;
        float y = _y;
        float pd = vera::getPixelDensity();

        if (x <= 0) x = vera::getWindowWidth() - 2;
        else if (x >= vera::getWindowWidth()) x = 2; 

        if (y <= 0) y = vera::getWindowHeight() - 2;
        else if (y >= vera::getWindowHeight()) y = 2;

        if (x != _x || y != _y) vera::setMousePosition(x, y);
    }

    if (_button == 1) {
        // Left-button drag is used to pan u_view2d.
        m_view2d = glm::translate(m_view2d, glm::vec2(-vera::getMouseVelX(),-vera::getMouseVelY()) );

        // Left-button drag is used to rotate geometry.
        float dist = uniforms.activeCamera->getDistance();

        float vel_x = vera::getMouseVelX();
        float vel_y = vera::getMouseVelY();

        if (fabs(vel_x) < 50.0 && fabs(vel_y) < 50.0) {
            m_camera_azimuth -= vel_x;
            m_camera_elevation -= vel_y * 0.5;
            uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, dist);
            uniforms.activeCamera->lookAt(glm::vec3(0.0));
        }
    } 
    else {
        // Right-button drag is used to zoom geometry.
        float dist = uniforms.activeCamera->getDistance();
        dist += (-.008f * vera::getMouseVelY());
        if (dist > 0.0f) {
            uniforms.activeCamera->orbit(m_camera_azimuth, m_camera_elevation, dist);
            uniforms.activeCamera->lookAt(glm::vec3(0.0));
        }
    }
}

void Sandbox::onViewportResize(int _newWidth, int _newHeight) {
    m_change_viewport = true;

    if (uniforms.activeCamera)
        uniforms.activeCamera->setViewport(_newWidth, _newHeight);
    
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

    if (m_postprocessing || m_plot == PLOT_LUMA || m_plot == PLOT_RGB || m_plot == PLOT_RED || m_plot == PLOT_GREEN || m_plot == PLOT_BLUE ) {
        if (quilt >= 0)
            m_sceneRender.updateBuffers(uniforms, vera::getQuiltWidth(), vera::getQuiltHeight());
        else 
            m_sceneRender.updateBuffers(uniforms, _newWidth, _newHeight);
    }

    if (screenshotFile != "" || isRecording()) 
        m_record_fbo.allocate(_newWidth, _newHeight, vera::COLOR_TEXTURE_DEPTH_BUFFER);

    flagChange();
}

void Sandbox::onScreenshot(std::string _file) {
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

void Sandbox::onPlot() {
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
