#pragma once

#include <memory>
#include "uniforms.h"
#include "types/command.h"

#include "ada/gl/gl.h"
#include "ada/gl/vbo.h"
#include "ada/gl/shader.h"
#include "ada/gl/textureCube.h"
#include "ada/scene/model.h"

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

    void            setBlend(ada::BlendMode _blend) { m_blend = _blend; }
    ada::BlendMode  getBlend() { return m_blend; }

    void            setCulling(ada::CullingMode _culling) { m_culling = _culling; }
    ada::CullingMode getCulling() { return m_culling; }

    void            setCubeMap(ada::SkyData* _skybox );
    void            setSun(const glm::vec3& _vec);

    void            flagChange();
    void            unflagChange();
    bool            haveChange() const;
    
    float           getArea() const { return m_area; }

    void            render(Uniforms& _uniforms);
    void            renderFloor(Uniforms& _uniforms, const glm::mat4& _mvp, bool _lights = true);
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
    float               m_area;

    // Camera
    ada::BlendMode      m_blend;
    ada::CullingMode    m_culling;
    bool                m_depth_test;
    
    // Ligth
    ada::Vbo*           m_lightUI_vbo;
    ada::Shader         m_lightUI_shader;
    bool                m_dynamicShadows;
    bool                m_shadows;

    // Background
    ada::Shader         m_background_shader;
    ada::Vbo*           m_background_vbo;
    bool                m_background;

    // CubeMap
    ada::Shader         m_cubemap_shader;
    ada::Vbo*           m_cubemap_vbo;
    ada::SkyData*       m_cubemap_skybox;

    ada::SkyData        m_skybox;

    ada::Shader         m_floor_shader;
    ada::Vbo*           m_floor_vbo;
    float               m_floor_height;
    int                 m_floor_subd_target;
    int                 m_floor_subd;

    // UI Grid
    std::unique_ptr<ada::Vbo>          m_grid_vbo;
    ada::Vbo*           m_axis_vbo;
};
