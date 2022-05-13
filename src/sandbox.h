#pragma once

#ifdef SUPPORT_MULTITHREAD_RECORDING 
#include <atomic>
#include "thread_pool/thread_pool.hpp"
#endif

#include "scene.h"
#include "types/files.h"
#include "ada/tools/list.h"

enum ShaderType {
    FRAGMENT = 0,
    VERTEX = 1
};

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

    void                render();
    void                renderUI();
    void                renderDone();

    void                clear();
    
    bool                isReady();

    void                addDefine( const std::string &_define, const std::string &_value = "");
    void                delDefine( const std::string &_define );

    // Getting some data out of Sandbox
    const std::string&  getSource( ShaderType _type ) const;
    Scene&              getScene() { return m_scene; }

    void                printDependencies( ShaderType _type ) const;
    
    // Some events
    void                onScroll( float _yoffset );
    void                onMouseDrag( float _x, float _y, int _button );
    void                onViewportResize( int _newWidth, int _newHeight );
    void                onFileChange( WatchFileList &_files, int _index );
    void                onScreenshot( std::string _file );
    void                onPlot();
   
    // Include folders
    ada::List           include_folders;

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
    void                _updateConvolutionPyramids();
    void                _updateBuffers();
    void                _renderConvolutionPyramids();
    void                _renderBuffers();

    // Main Shader
    std::string         m_frag_source;
    std::string         m_vert_source;

    // Dependencies
    ada::List           m_vert_dependencies;
    ada::List           m_frag_dependencies;

    // Buffers
    std::vector<ada::Shader>    m_buffers_shaders;
    int                         m_buffers_total;

    // Buffers
    std::vector<ada::Shader>    m_doubleBuffers_shaders;
    int                         m_doubleBuffers_total;

    // A. CANVAS
    ada::Shader         m_canvas_shader;

    // B. SCENE
    Scene               m_scene;
    ada::Fbo            m_scene_fbo;

    // Pyramid Convolution
    std::vector<ada::Fbo>       m_convolution_pyramid_fbos;
    std::vector<ada::Shader>    m_convolution_pyramid_subshaders;
    ada::Shader                 m_convolution_pyramid_shader;
    int                         m_convolution_pyramid_total;

    // Postprocessing
    ada::Shader         m_postprocessing_shader;
    bool                m_postprocessing;

    // Billboard
    ada::Shader         m_billboard_shader;
    ada::Vbo*           m_billboard_vbo;
    
    // Cursor
    ada::Shader         m_wireframe2D_shader;
    ada::Vbo*           m_cross_vbo;

    // debug plot texture and shader for histogram or fps plots
    ada::Shader         m_plot_shader;
    ada::Texture*       m_plot_texture;
    glm::vec4           m_plot_values[256];

    // Recording
    ada::Fbo            m_record_fbo;
    #ifdef SUPPORT_MULTITHREAD_RECORDING 
    std::atomic<int>        m_task_count {0};
    std::atomic<long long>  m_max_mem_in_queue {0};
    thread_pool::ThreadPool m_save_threads;
    #endif

    // Other state properties
    glm::mat3           m_view2d;
    float               m_time_offset;
    float               m_lat;
    float               m_lon;
    size_t              m_frame;
    bool                m_change;
    bool                m_initialized;
    bool                m_error_screen;

    //  Debug
    size_t              m_plot;
    bool                m_showTextures;
    bool                m_showPasses;
    
};
