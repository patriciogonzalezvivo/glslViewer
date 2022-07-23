#pragma once

#if defined(SUPPORT_MULTITHREAD_RECORDING)
#include <atomic>
#include "thread_pool/thread_pool.hpp"
#endif

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

class Sandbox {
public:
    Sandbox();
    virtual ~Sandbox();
    
    // Main stages
    void                setup(WatchFileList &_files, CommandList &_commands);

    bool                setSource(ShaderType _type, const std::string& _source);
    bool                reloadShaders(WatchFileList &_files);

    void                flagChange();
    void                unflagChange(); 
    bool                haveChange();

    void                renderPrep();
    void                render();
    void                renderPost();
    void                renderUI();
    void                renderDone();

    void                clear();
    
    bool                isReady();

    void                addDefine( const std::string &_define, const std::string &_value = "");
    void                delDefine( const std::string &_define );

    // Getting some data out of Sandbox
    const std::string&  getSource( ShaderType _type ) const;
    SceneRender&        getSceneRender() { return m_sceneRender; }

    void                printDependencies( ShaderType _type ) const;
    
    // Some events
    void                onScroll( float _yoffset );
    void                onMouseDrag( float _x, float _y, int _button );
    void                onViewportResize( int _newWidth, int _newHeight );
    void                onFileChange( WatchFileList &_files, int _index );
    void                onScreenshot( std::string _file );
    void                onPlot();
   
    // Include folders
    vera::StringList     include_folders;

    // Uniforms
    Uniforms            uniforms;

    // Screenshot file
    std::string         screenshotFile;

    // States
    int                 frag_index;
    int                 vert_index;
    int                 geom_index;

    std::string         lenticular;
    int                 quilt;
    
    bool                verbose;
    bool                cursor;
    bool                fxaa;

private:
    void                _updateSceneBuffer(int _width, int _height);
    void                _updateBuffers();
    void                _renderBuffers();

    // Main Shader
    std::string         m_frag_source;
    std::string         m_vert_source;

    // Dependencies
    vera::StringList    m_vert_dependencies;
    vera::StringList    m_frag_dependencies;

    // Buffers
    ShaderList          m_buffers_shaders;
    int                 m_buffers_total;

    // Buffers
    ShaderList          m_doubleBuffers_shaders;
    int                 m_doubleBuffers_total;

    // A. CANVAS
    vera::Shader        m_canvas_shader;

    // B. SCENE
    SceneRender         m_sceneRender;
    vera::Fbo           m_sceneRender_fbo;

    // Pyramid Convolution
    FboList             m_pyramid_fbos;
    ShaderList          m_pyramid_subshaders;
    vera::Shader        m_pyramid_shader;
    int                 m_pyramid_total;

    // Postprocessing
    vera::Shader        m_postprocessing_shader;
    bool                m_postprocessing;
    
    // Cursor
    vera::Vbo*          m_cross_vbo;

    // debug plot texture and shader for histogram or fps plots
    vera::Shader        m_plot_shader;
    vera::Texture*      m_plot_texture;
    glm::vec4           m_plot_values[256];
    PlotType            m_plot;

    // Recording
    vera::Fbo           m_record_fbo;
    #if defined(SUPPORT_MULTITHREAD_RECORDING)
    std::atomic<int>    m_task_count {0};
    std::atomic<long long>      m_max_mem_in_queue {0};
    thread_pool::ThreadPool     m_save_threads;
    #endif

    // Other state properties
    glm::mat3           m_view2d;
    float               m_time_offset;
    float               m_camera_azimuth;
    float               m_camera_elevation;
    size_t              m_frame;
    vera::ShaderErrorResolve    m_error_screen;
    bool                m_change;
    bool                m_initialized;

    //  Debug
    bool                m_showTextures;
    bool                m_showPasses;
};
