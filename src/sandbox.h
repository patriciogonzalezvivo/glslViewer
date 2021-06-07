#pragma once

#include "scene/scene.h"
#include "types/list.h"

#include "thread_pool/thread_pool.hpp"

#include <atomic>

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
    bool                reloadShaders(WatchFileList &_files);

    void                flagChange();
    void                unflagChange(); 
    bool                haveChange();

    void                render();
    void                renderUI();
    void                renderDone();

    void                clear();
    
    bool                isReady();

    void                record( float _start, float _end, float fps = 24.0 );
    int                 getRecordedPercentage();

    void                addDefine( const std::string &_define, const std::string &_value = "");
    void                delDefine( const std::string &_define );

    // Getting some data out of Sandbox
    std::string         getSource( ShaderType _type ) const;

    void                printDependencies( ShaderType _type ) const;

    Scene&              getScene() { return m_scene; }
    
    // Some events
    void                onScroll( float _yoffset );
    void                onMouseDrag( float _x, float _y, int _button );
    void                onViewportResize( int _newWidth, int _newHeight );
    void                onFileChange( WatchFileList &_files, int _index );
    void                onScreenshot( std::string _file );
    void                onHistogram();
   
    // Include folders
    List                include_folders;

    // Uniforms
    Uniforms            uniforms;

    // Screenshot file
    std::string         screenshotFile;

    // States
    int                 frag_index;
    int                 vert_index;
    int                 geom_index;

    int                 holoplay;
    
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
    List                m_vert_dependencies;
    List                m_frag_dependencies;

    // Buffers
    std::vector<Shader> m_buffers_shaders;
    int                 m_buffers_total;

    // A. CANVAS
    Shader              m_canvas_shader;

    // B. SCENE
    Scene               m_scene;
    Fbo                 m_scene_fbo;

    // Pyramid Convolution
    Shader              m_convolution_pyramid_subshader;
    Shader              m_convolution_pyramid_shader;
    Fbo                 m_convolution_pyramid_fbo;
    bool                m_convolution_pyramid;

    // Postprocessing
    Shader              m_postprocessing_shader;
    bool                m_postprocessing;

    // Billboard
    Shader              m_billboard_shader;
    Vbo*                m_billboard_vbo;
    
    // Cursor
    Shader              m_wireframe2D_shader;
    Vbo*                m_cross_vbo;

    // Recording
    Fbo                 m_record_fbo;
    float               m_record_fdelta;
    float               m_record_start;
    float               m_record_head;
    float               m_record_end;
    int                 m_record_counter;
    bool                m_record;

    // Histogram
    Shader              m_histogram_shader;
    Texture*            m_histogram_texture;
    bool                m_histogram;

    // Other state properties
    glm::mat3           m_view2d;
    float               m_lat;
    float               m_lon;
    unsigned int        m_frame;
    bool                m_change;
    bool                m_initialized;
    bool                m_error_screen;

    //  Debug
    bool                m_showTextures;
    bool                m_showPasses;
    
    std::atomic<int>        m_task_count {0};
    std::atomic<long long> m_max_mem_in_queue {0};
    thread_pool::ThreadPool m_save_threads;
};
