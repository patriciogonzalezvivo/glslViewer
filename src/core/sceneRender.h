#pragma once

#include <memory>
#include "uniforms.h"
#include "tools/command.h"

#include "vera/gl/gl.h"
#include "vera/gl/vbo.h"
#include "vera/gl/shader.h"
#include "vera/gl/textureCube.h"
#include "vera/types/model.h"

class SceneRender {
public:

    SceneRender();
    virtual ~SceneRender();

    void            commandsInit(CommandList& _commands, Uniforms& _uniforms);
    void            uniformsInit(Uniforms& _uniforms);

    bool            loadScene(Uniforms& _uniforms);
    bool            clearScene();
    void            setShaders(Uniforms& _uniforms, const std::string& _fragmentShader, const std::string& _vertexShader);

    void            addDefine(const std::string& _define, const std::string& _value);
    void            delDefine(const std::string& _define);
    void            printDefines();

    void            setBlend(vera::BlendMode _blend) { m_blend = _blend; }
    vera::BlendMode getBlend() const { return m_blend; }

    void            setCulling(vera::CullingMode _culling) { m_culling = _culling; }
    vera::CullingMode getCulling() { return m_culling; }

    float           getArea() const { return m_area; }
    
    size_t          getBuffersTotal() const { return m_buffers_total; }
    void            updateBuffers(Uniforms& _uniforms, int _width, int _height);
    void            printBuffers();

    void            render(Uniforms& _uniforms);
    void            renderFloor(Uniforms& _uniforms);
    void            renderDevLook(Uniforms& _uniforms);
    void            renderBackground(Uniforms& _uniforms);
    void            renderDebug(Uniforms& _uniforms);
    void            renderShadowMap(Uniforms& _uniforms);
    void            renderNormalBuffer(Uniforms& _uniforms);
    void            renderPositionBuffer(Uniforms& _uniforms);
    void            renderBuffers(Uniforms& _uniforms);

    bool            showGrid;
    bool            showAxis;
    bool            showBBoxes;
    bool            showCubebox;

    vera::Fbo       renderFbo;
    vera::Fbo       normalFbo;
    vera::Fbo       positionFbo;
    BuffersList     buffersFbo;

    bool            dynamicShadows;

protected:
    vera::Node                  m_origin;
    float                       m_area;

    // Camera
    vera::BlendMode             m_blend;
    vera::CullingMode           m_culling;
    bool                        m_depth_test;
    
    // Light
    std::unique_ptr<vera::Vbo>  m_lightUI_vbo;
    vera::Shader                m_lightUI_shader;
    bool                        m_shadows;

    // Background
    vera::Shader                m_background_shader;
    bool                        m_background;

    // CubeMap
    vera::Shader                m_cubemap_shader;
    std::unique_ptr<vera::Vbo>  m_cubemap_vbo;

    // Floor
    vera::Model                 m_floor;
    float                       m_floor_height;
    int                         m_floor_subd_target;
    int                         m_floor_subd;

    // DevLook
    std::vector<vera::Model*>   m_devlook_spheres;
    std::vector<vera::Model*>   m_devlook_billboards;

    // UI Grid
    std::unique_ptr<vera::Vbo>  m_grid_vbo;
    std::unique_ptr<vera::Vbo>  m_axis_vbo;

    glm::vec3                   m_ssaoSamples[64];
    glm::vec3                   m_ssaoNoise[16];

    size_t                      m_buffers_total;

    bool                        m_commands_loaded;
    bool                        m_uniforms_loaded;
};
