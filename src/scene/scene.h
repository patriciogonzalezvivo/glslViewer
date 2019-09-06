#pragma once

#include "light.h"
#include "model.h"
#include "camera.h"
#include "sceneLoader.h"

#include "gl/vbo.h"
#include "gl/fbo.h"
#include "gl/shader.h"
#include "gl/textureCube.h"

#include "tools/skybox.h"
#include "tools/command.h"

#include "shaders/billboard.h"
#include "shaders/default_error.h"

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

    void            setLight( glm::vec3 _position,  glm::vec3 _color );
    Fbo&            getLightMap() { return m_light_depthfbo; }

    void            setDynamicShadows(bool _dynamic) { m_dynamicShadows = _dynamic; }
    bool            getDynamicShadows() { return m_dynamicShadows; }

    void            setCubeMap( SkyBox* _skybox );
    void            setCubeMap( TextureCube* _cubemap );
    void            setCubeMap( const std::string& _filename, WatchFileList &_files, bool _visible, bool _verbose = true);
    TextureCube*    getCubeMap() { return m_cubemap; }
    void            setCubeMapVisible( bool _draw ) {  m_cubemap_draw = _draw; }
    bool            getCubeMapVisible() { return m_cubemap_draw; }

    void            unflagChange();
    bool            haveChange() const;

    void            render(Uniforms &_uniforms);
    void            renderGeometry(Uniforms &_uniforms);
    void            renderShadowMap(Uniforms &_uniforms);
    void            renderBackground(Uniforms &_uniforms);
    void            renderDebug();

    void            onMouseDrag(float _x, float _y, int _button);
    void            onViewportResize(int _newWidth, int _newHeight);

protected:
     // Geometry
    std::vector<Model*>             m_models;
    std::map<std::string,Material>  m_materials;

    Node            m_origin;
    glm::mat4       m_mvp;
    float           m_area;
    float           m_floor;

    // Camera
    Camera          m_camera;
    CullingMode     m_culling;
    float           m_lat;
    float           m_lon;

    // Ligth
    Light           m_light;
    Vbo*            m_light_vbo;
    Shader          m_light_shader;
    Fbo             m_light_depthfbo;
    bool            m_dynamicShadows;

    // Background
    Shader          m_background_shader;
    Vbo*            m_background_vbo;
    bool            m_background_draw;

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
};