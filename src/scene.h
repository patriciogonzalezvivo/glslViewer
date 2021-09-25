#pragma once


#include "uniforms.h"
#include "io/command.h"

#include "ada/gl/vbo.h"
#include "ada/gl/shader.h"
#include "ada/gl/textureCube.h"
#include "ada/scene/model.h"

enum CullingMode {
    NONE = 0,
    FRONT = 1,
    BACK = 2,
    BOTH = 3
};

enum BlendMode {
    ALPHA = 0,       // Alpha is the default
    ADD = 1,
    MULTIPLY = 2,
    SCREEN = 3,
    SUBSTRACT = 4,
};

class Scene {
public:

    Scene();
    virtual ~Scene();

    void            setup(CommandList& _commands, Uniforms& _uniforms);
    void            clear();

    bool            loadGeometry(Uniforms& _uniforms, WatchFileList& _files, int _index, bool _verbose);
    bool            loadShaders(const std::string& _fragmentShader, const std::string& _vertexShader, bool _verbose);


    void            addDefine(const std::string& _define, const std::string& _value);
    void            delDefine(const std::string& _define);
    void            printDefines();

    void            setBlend(BlendMode _blend) { m_blend = _blend; }
    BlendMode       getBlend() { return m_blend; }

    void            setCulling(CullingMode _culling) { m_culling = _culling; }
    CullingMode     getCulling() { return m_culling; }

    void            setCubeMap(ada::SkyBox* _skybox );

    void            flagChange();
    void            unflagChange();
    bool            haveChange() const;
    
    float           getArea() const { return m_area; }

    void            render(Uniforms& _uniforms);
    void            renderFloor(Uniforms& _uniforms, const glm::mat4& _mvp);
    void            renderGeometry(Uniforms& _uniforms);
    void            renderBackground(Uniforms& _uniforms);
    void            renderDebug(Uniforms& _uniforms);

    void            renderShadowMap(Uniforms& _uniforms);

    bool            showGrid;
    bool            showAxis;
    bool            showBBoxes;
    bool            showCubebox;

protected:
     // Geometry
    std::vector<ada::Model*>             m_models;
    std::map<std::string,ada::Material>  m_materials;

    ada::Node           m_origin;
    glm::mat4           m_mvp;
    float               m_area;

    // Camera
    BlendMode           m_blend;
    CullingMode         m_culling;
    bool                m_depth_test;
    
    // Ligth
    ada::Vbo*           m_lightUI_vbo;
    ada::Shader         m_lightUI_shader;
    bool                m_dynamicShadows;

    // Background
    ada::Shader         m_background_shader;
    ada::Vbo*           m_background_vbo;
    bool                m_background;

    // CubeMap
    ada::Shader         m_cubemap_shader;
    ada::Vbo*           m_cubemap_vbo;
    ada::SkyBox*        m_cubemap_skybox;

    ada::SkyBox         m_skybox;

    ada::Shader         m_floor_shader;
    ada::Vbo*           m_floor_vbo;
    float               m_floor_height;
    int                 m_floor_subd_target;
    int                 m_floor_subd;

    // UI Grid
    ada::Vbo*           m_grid_vbo;
    ada::Vbo*           m_axis_vbo;
    
    // UI Bbox
    ada::Shader         m_wireframe3D_shader;
};