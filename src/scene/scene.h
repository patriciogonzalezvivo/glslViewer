#pragma once

#include "camera.h"
#include "light.h"
#include "model.h"

#include "gl/vbo.h"
#include "gl/fbo.h"
#include "gl/shader.h"
#include "gl/textureCube.h"

#include "tools/skybox.h"
#include "tools/uniform.h"
#include "tools/command.h"
#include "tools/skybox.h"

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

    void            flagChange() { m_change = true; }
    void            unfalgChange() { m_change = false; }
    bool            haveChange() { return m_change; }

    void            loadModel( const std::string &_path, List &_defines);
    Model&          getModel() { return m_model; }

    void            setCamera( glm::vec3 _position, glm::vec2 _viewport );
    // Camera&         getCamera() { return m_camera; }

    void            setCulling(CullingMode _culling) { m_culling = _culling; }
    CullingMode     getCulling() { return m_culling; }

    void            setLight( glm::vec3 _position,  glm::vec3 _color );
    // Light&          getLight() { return m_light; }
    Fbo&            getLightMap() { return m_light_depthfbo; }

    void            setDynamicShadows(bool _dynamic) { m_dynamicShadows = _dynamic; }
    bool            getDynamicShadows() { return m_dynamicShadows; }

    void            setCubeMap( SkyBox* _skybox ) { m_cubemap_skybox = _skybox; m_cubemap_skybox->change = true; }
    void            setCubeMap( TextureCube* _cubemap ) {  m_cubemap = _cubemap; }
    TextureCube*    getCubeMap() { return m_cubemap; }
    void            setCubeMapVisible( bool _draw ) {  m_cubemap_draw = _draw; }
    bool            getCubeMapVisible() { return m_cubemap_draw; }

    void            render(Shader &_shader, Uniforms &_uniforms);
    void            renderDebug();
    void            renderCubeMap();
    void            renderGeometry(Shader &_shader, Uniforms &_uniforms);
    void            renderShadowMap(Shader &_shader, Uniforms &_uniforms);

    void            onMouseDrag(float _x, float _y, int _button);
    void            onViewportResize(int _newWidth, int _newHeight);

protected:

    // Defines
    List            m_defines;

     // Geometry
    Model           m_model;

    // Camera
    Camera          m_camera;
    CullingMode     m_culling;
    glm::mat4       m_mvp;
    float           m_lat;
    float           m_lon;

    // Ligth
    Light           m_light;
    Vbo*            m_light_vbo;
    Shader          m_light_shader;
    Fbo             m_light_depthfbo;
    bool            m_dynamicShadows;

    // CubeMap
    Shader          m_cubemap_shader;
    Vbo*            m_cubemap_vbo;
    TextureCube*    m_cubemap;
    SkyBox*         m_cubemap_skybox;
    bool            m_cubemap_draw;

    SkyBox          m_skybox;

    // UI Grid
    Vbo*            m_grid_vbo;
    Vbo*            m_axis_vbo;
    
    // UI Bbox
    Shader          m_wireframe3D_shader;
    
    bool            m_change;
};