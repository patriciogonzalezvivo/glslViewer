#pragma once

#if defined(SUPPORT_MULTITHREAD_RECORDING)
#include <atomic>
#include "thread_pool/thread_pool.hpp"
#endif

#include <mutex>
#include <vector>

#include "sceneRender.h"
#include "tools/files.h"
#include "vera/ops/string.h"

enum ShaderType {
    FRAGMENT = 0,
    VERTEX = 1
};

enum PlotType {
    PLOT_OFF = 0,
    PLOT_LUMA, PLOT_RED, PLOT_GREEN, PLOT_BLUE, PLOT_RGB,
    PLOT_FPS, PLOT_MS
};

const std::string plot_options[] = { "off", "luma", "red", "green", "blue", "rgb", "fps", "ms" };

typedef std::vector<vera::Fbo>       FboList;
typedef std::vector<vera::Shader>    ShaderList;

class GlslViewer {
public:
    GlslViewer();
    virtual ~GlslViewer();
    
    // Main stages
    void                loadAssets(WatchFileList &_files);
    void                loadModel(vera::Model* _model);
    void                commandsInit(CommandList &_commands);

    // Switches to a loaded named camera (e.g. one loaded from a COLMAP
    // camera.csv) by id, syncing "default" for orbiting. Returns false if
    // no camera with that id is loaded. Shared by the "camera" console
    // command and the auto-select-first-camera-on-load behavior. Animates
    // the transition (position/orientation/projection) over
    // m_camera_transition_duration seconds unless _animate is false (used
    // for the very first, load-time selection, which should be instant).
    bool                selectCamera(const std::string& _id, bool _animate = true);

    void                setFrame(size_t _frame);
    void                setSource(ShaderType _type, const std::string& _source);
    void                resetShaders(WatchFileList &_files);

    bool                haveChange();

    void                renderPrep();
    void                render();
    void                renderPost();
    void                renderUI();
    void                renderDone();

    bool                isReady();

    void                addDefine( const std::string &_define, const std::string &_value = "");
    void                delDefine( const std::string &_define );

    // Getting some data out of Sandbox
    const std::string&  getSource( ShaderType _type ) const;
    SceneRender&        getSceneRender() { return m_sceneRender; }

    void                printDependencies( ShaderType _type ) const;
    
    // Some events
    void                onScroll( float _yoffset );
    void                onMousePress( float _x, float _y, int _button );
    void                onMouseMove( float _x, float _y );
    void                onMouseDrag( float _x, float _y, int _button );
    void                onWindowResize( int _newWidth, int _newHeight );
    void                onFileChange( WatchFileList &_files, int _index );
    void                onScreenshot( std::string _file );
    void                onPlot();
   
    // Include folders
    vera::StringList    include_folders;

    // Uniforms
    Uniforms            uniforms;

    // Screenshot file
    std::string         screenshotFile;

    // Quilt/Lenticular
    std::string         lenticular;
    int                 quilt_resolution;
    int                 quilt_tile;

    // States
    int                 frag_index;
    int                 vert_index;
    int                 geom_index;             // index (into the WatchFileList) of the first geometry file, or -1. Kept for the Python API and single-geometry commands.
    std::vector<int>    geom_indices;           // indices of ALL loaded geometry files (may combine meshes, point clouds and splats)
    bool                hasGeometry() const { return !geom_indices.empty(); }
    bool                verbose;
    bool                cursor;
    bool                help;
    bool                fxaa;

protected:
    void                _updateBuffers();
    void                _renderBuffers();

    // Geometry files whose reload was requested from the file-watcher thread.
    // GL resources (VBOs, shaders, textures) can only be touched on the render
    // thread, so onFileChange() just enqueues here and renderPrep() drains it.
    std::vector<std::string> m_geom_reload_queue;
    std::mutex               m_geom_reload_mutex;

    // Main Shader
    std::string         m_frag_source;
    std::string         m_vert_source;

    // Dependencies
    vera::StringList    m_vert_dependencies;
    vera::StringList    m_frag_dependencies;

    // Buffers
    ShaderList          m_buffers_shaders;
    int                 m_buffers_total;

    // Double Buffers
    ShaderList          m_doubleBuffers_shaders;
    int                 m_doubleBuffers_total;

    // Pyramids
    FboList             m_pyramid_fbos;
    ShaderList          m_pyramid_subshaders;
    vera::Shader        m_pyramid_shader;
    int                 m_pyramid_total;

    // Floods
    ShaderList          m_flood_subshaders;
    vera::Shader        m_flood_shader;
    int                 m_flood_total;

    // A. CANVAS
    vera::Shader        m_canvas_shader;

    // B. SCENE
    SceneRender         m_sceneRender;
    
    // Postprocessing
    vera::Shader        m_postprocessing_shader;
    bool                m_postprocessing;
    
    // Cursor
    std::unique_ptr<vera::Vbo>      m_cross_vbo;

    // debug plot texture and shader for histogram or fps plots
    vera::Shader                    m_plot_shader;
    vera::Texture*                  m_plot_texture;
    glm::vec4                       m_plot_values[256];
    PlotType                        m_plot;

    // Recording
    vera::Fbo                       m_record_fbo;
    #if defined(SUPPORT_MULTITHREAD_RECORDING)
    std::atomic<int>                m_task_count {0};
    std::atomic<long long>          m_max_mem_in_queue {0};
    thread_pool::ThreadPool         m_save_threads;
    #endif

    // Other state properties
    glm::mat3                       m_view2d;
    float                           m_time_offset;
    
    glm::vec3                       m_camera_target;
    float                           m_camera_azimuth;
    float                           m_camera_elevation;
    std::string                     m_camera_id;

    // Animated transition between named cameras (see selectCamera()),
    // advanced once per frame in updateCameraTransition().
    void                            updateCameraTransition();
    void                            finishCameraSelection(const std::string& _id);
    void                            applyCameraMatrixUniforms(vera::Camera* _cam);
    bool                            m_camera_transitioning;
    float                           m_camera_transition_time;
    float                           m_camera_transition_duration;
    std::string                     m_camera_transition_target_id;
    glm::vec3                       m_camera_transition_from_pos;
    glm::quat                       m_camera_transition_from_rot;
    glm::mat4                       m_camera_transition_from_proj;

    // Camera animation (camera,orbit / arc / dolly / truck / pedestal / pan /
    // tilt / roll). Plays every frame in updateCameraAnimation() until the user
    // does a mouse gesture (see onMousePress/onMouseDrag/onScroll), which sets
    // m_cam_anim = CAM_NONE. m_cam_anim_speed is a global multiplier shared with
    // the named-camera transition above.
    enum CameraAnim { CAM_NONE, CAM_ORBIT, CAM_ARC, CAM_DOLLY,
                      CAM_TRUCK, CAM_PEDESTAL, CAM_PAN, CAM_TILT, CAM_ROLL };
    void                            startCameraAnimation(CameraAnim _mode, float _a = 0.0f, float _b = 0.0f);
    void                            updateCameraAnimation();
    CameraAnim                      m_cam_anim;
    float                           m_cam_anim_phase;       // advances by getDelta()*speed
    float                           m_cam_anim_amp;         // ping-pong amplitude (deg or dist)
    float                           m_cam_anim_min;         // dolly min distance
    float                           m_cam_anim_max;         // dolly max distance
    float                           m_cam_anim_speed;        // global animation speed multiplier
    // Base pose captured when an animation starts (offsets are applied relative
    // to this every frame, so the oscillation never drifts and stops cleanly).
    glm::vec3                       m_cam_base_pos;
    glm::vec3                       m_cam_base_target;
    glm::quat                       m_cam_base_rot;
    float                           m_cam_base_az;
    float                           m_cam_base_el;
    float                           m_cam_base_dist;

    vera::ShaderErrorResolve        m_error_screen;
    bool                            m_change_viewport;
    bool                            m_update_buffers;

    bool                            m_initialized;

    //  Debug
    bool                            m_showTextures;
    bool                            m_showPasses;
};
