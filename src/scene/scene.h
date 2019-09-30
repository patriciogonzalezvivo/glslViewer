#pragma once

#include "light.h"
#include "model.h"
#include "camera.h"


#include "../gl/vbo.h"
#include "../gl/shader.h"
#include "../gl/textureCube.h"

#include "../tools/skybox.h"
#include "../tools/command.h"

#include "../shaders/billboard.h"
#include "../shaders/default_scene.h"

#include "../loaders/ply.h"
#include "../loaders/obj.h"

enum CullingMode {
    NONE = 0,
    FRONT = 1,
    BACK = 2,
    BOTH = 3
};

class Scene {
public:

    Scene();
    virtual ~Scene();

    void            setup(CommandList &_commands, Uniforms &_uniforms);
    void            clear();

    bool            loadGeometry(Uniforms& _uniforms, WatchFileList &_files, int _index, bool _verbose);
    bool            loadShaders(const std::string &_fragmentShader, const std::string &_vertexShader, bool _verbose);

    void            addDefine(const std::string &_define, const std::string &_value);
    void            delDefine(const std::string &_define);
    void            printDefines();

    void            setCulling(CullingMode _culling) { m_culling = _culling; }
    CullingMode     getCulling() { return m_culling; }

    const Fbo*      getShadowMap() { return m_light.getShadowMap(); }

    void            setCubeMap( SkyBox* _skybox );
    void            setCubeMap( TextureCube* _cubemap );
    void            setCubeMap( const std::string& _filename, WatchFileList &_files, bool _visible, bool _verbose = true);
    TextureCube*    getCubeMap() { return m_cubemap; }

    void            flagChange();
    void            unflagChange();
    bool            haveChange() const;

    void            render(Uniforms &_uniforms);
    void            renderFloor(Uniforms &_uniforms, const glm::mat4& _mvp);
    void            renderGeometry(Uniforms &_uniforms);
    void            renderShadowMap(Uniforms &_uniforms);
    void            renderBackground(Uniforms &_uniforms);
    void            renderDebug();

    void            onMouseDrag(float _x, float _y, int _button);
    void            onViewportResize(int _newWidth, int _newHeight);

    bool            showGrid;
    bool            showAxis;
    bool            showBBoxes;
    bool            showCubebox;

protected:
     // Geometry
    std::vector<Model*>             m_models;
    std::map<std::string,Material>  m_materials;

    Node            m_origin;
    glm::mat4       m_mvp;
    float           m_area;

    // Camera
    Camera          m_camera;
    CullingMode     m_culling;
    float           m_lat;
    float           m_lon;

    // Ligth
    Light           m_light;
    Vbo*            m_light_vbo;
    Shader          m_light_shader;
    bool            m_dynamicShadows;

    // Background
    Shader          m_background_shader;
    Vbo*            m_background_vbo;
    bool            m_background;

    // CubeMap
    Shader          m_cubemap_shader;
    Vbo*            m_cubemap_vbo;
    TextureCube*    m_cubemap;
    SkyBox*         m_cubemap_skybox;

    SkyBox          m_skybox;

    Shader          m_floor_shader;
    Vbo*            m_floor_vbo;
    float           m_floor_height;
    int             m_floor_subd_target;
    int             m_floor_subd;

    // UI Grid
    Vbo*            m_grid_vbo;
    Vbo*            m_axis_vbo;
    
    // UI Bbox
    Shader          m_wireframe3D_shader;
};