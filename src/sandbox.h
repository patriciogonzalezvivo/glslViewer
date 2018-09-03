#pragma once

#include "gl/shader.h"
#include "gl/vbo.h"
#include "gl/texture.h"
#include "gl/textureCube.h"
#include "gl/fbo.h"

#include "3d/camera.h"
#include "3d/light.h"

#include "tools/fs.h"
#include "tools/uniform.h"

enum ShaderType {
    FRAGMENT = 0,
    VERTEX = 1
};

enum CullingMode {
    NONE = 0,
    FRONT = 1,
    BACK = 2,
    BOTH = 3
};

typedef std::vector<std::string> List;
typedef std::map<std::string,Texture*> TextureList;

class Sandbox {
public:
    Sandbox();
    
    // Main stages
    void                    setup(WatchFileList &_files);
    bool                    reload();
    void                    draw();
    void                    drawDebug2D();
    void                    drawDebug3D();
    void                    drawDone();
    void                    clean();
    

    void                    flagChange() { m_change = true; }
    bool                    haveChange();
    bool                    isReady();

    void                    record(float _start, float _end);
    int                     getRecordedPorcentage();

    void                    addDefines(const std::string &_define);
    void                    delDefines(const std::string &_define);

    void                    setCubeMap(TextureCube* _cubemap) { m_cubemap = _cubemap; }
    void                    setCulling(CullingMode _culling) { m_culling = _culling; }
    CullingMode             getCulling() { return m_culling; }

    // Getting some data out of Sandbox
    std::string             getSource(ShaderType _type) const;
    int                     getTotalBuffers() const { return m_buffers.size(); }

    Camera&                 getCamera() { return m_cam; }
    Light&                  getLight() { return m_light; }

    void                    printDependencies(ShaderType _type) const;
    
    // Some events
    void                    onFileChange(WatchFileList &_files, int _index);
    void                    onScroll(float _yoffset);
    void                    onScreenshot(std::string _file);
    void                    onMouseDrag(float _x, float _y, int _button);
    void                    onViewportResize(int _newWidth, int _newHeight);
   
    // User defined Uniforms 
    UniformDataList         uniforms_data;

    // Native Uniforms
    UniformFunctionsList    uniforms_functions;

    // Textures
    TextureList             textures;

    // Defines
    List                    defines;

    // Include folders
    FileList                include_folders;

    // Screenshot file
    std::string             screenshotFile;

    // States
    int                     frag_index;
    int                     vert_index;
    int                     geom_index;

    bool                    verbose;
    bool                    cursor;
    bool                    debug;

private:
    void                _updateBuffers();
    void                _updateUniforms(Shader &_shader);
    void                _updateTextures(Shader &_shader, int &_textureIndex);
    void                _updateDependencies(WatchFileList &_files);

    // Main Shader
    std::string         m_frag_source;
    std::string         m_vert_source;

    // Dependencies
    FileList            m_vert_dependencies;
    FileList            m_frag_dependencies;
    Shader              m_shader;

    // Geometry
    Vbo*                m_model_vbo;
    Node                m_model_node;

    // Camera
    Camera              m_cam;
    glm::mat4           m_mvp;
    glm::mat3           m_view2d;
    float               m_lat;
    float               m_lon;

    // Ligth
    Light               m_light;

    // CubeMap
    Shader              m_cubemap_shader;
    Vbo*                m_cubemap_vbo;
    TextureCube*        m_cubemap;
    
    // Background
    Shader              m_background_shader;
    bool                m_background_enabled;

    // Buffers
    std::vector<Fbo>    m_buffers;
    std::vector<Shader> m_buffers_shaders;
    int                 m_buffers_total;

    // Postprocessing
    Fbo                 m_scene_fbo;
    Shader              m_postprocessing_shader;
    bool                m_postprocessing_enabled;

    // Billboard
    Shader              m_billboard_shader;
    Vbo*                m_billboard_vbo;
    
    // UI
    Shader              m_light_shader;
    Shader              m_wireframe2D_shader;
    Vbo*                m_cross_vbo;
    
    // Bbox
    Shader              m_wireframe3D_shader;
    Vbo*                m_bbox_vbo;
    
    // Recording
    float               m_record_start;
    float               m_record_head;
    float               m_record_end;
    int                 m_record_counter;
    bool                m_record;

    // Scene
    CullingMode         m_culling;
    int                 m_textureIndex;
    bool                m_change;
    bool                m_ready;
};
