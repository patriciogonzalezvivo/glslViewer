#pragma once

#include "gl/shader.h"
#include "gl/vbo.h"
#include "gl/texture.h"
#include "gl/textureCube.h"
#include "gl/fbo.h"
#include "gl/uniform.h"

#include "3d/camera.h"

#include "tools/fs.h"

typedef std::vector<std::string> List;
typedef std::map<std::string,Texture*> TextureList;

class Sandbox {
public:
    Sandbox();
    
    // Main stages
    void        setup(WatchFileList &_files);
    void        draw();
    void        clean();
    bool        reload();

    void        record(float _start, float _end);
    int         getRecordedPorcentage();

    void        addDefines(const std::string &_define);
    void        delDefines(const std::string &_define);

    void        setCubeMap(TextureCube* _cubemap) { m_cubemap = _cubemap; }

    // Getting some data out of Sandbox
    std::string getVertexSource() const { return m_vertSource; } 
    std::string getFragmentSource() const { return m_fragSource; }
    std::string get3DView() const;
    Camera&     getCamera() { return m_cam; }
    int         getTotalBuffers() const { return m_buffers.size(); }
    
    // Some events
    void        onFileChange(WatchFileList &_files, int _index);
    void        onScroll(float _yoffset);
    void        onScreenshot(std::string _file);
    void        onMouseDrag(float _x, float _y, int _button);
    void        onViewportResize(int _newWidth, int _newHeight);
   
    // Uniforms
    UniformList uniforms;

    // Textures
    TextureList textures;

    // Defines
    List        defines;

    // Include folders
    FileList    include_folders;

    // Dependencies
    FileList    vert_dependencies;
    FileList    frag_dependencies;

    // Screenshot file
    std::string screenshotFile;

    // States
    int         iFrag;
    int         iVert;
    int         iGeom;

    bool        verbose;
    bool        vFlip;

private:
    void _updateBackground(); 
    void _updateBuffers();
    void _updateUniforms( Shader &_shader );
    void _updateTextures( Shader &_shader, int &_textureIndex  );
    void _updateDependencies(WatchFileList &_files);

    // Main Shader
    std::string         m_fragSource;
    const std::string*  m_fragPath;
    
    std::string         m_vertSource;
    const std::string*  m_vertPath;

    Shader              m_shader;

    // Geometry
    Vbo*                m_vbo;
    glm::mat4           m_vbo_matrix;

    // Camera
    Camera              m_cam;
    glm::mat3           m_view2d;
    glm::vec3           m_centre3d;
    glm::vec3           m_eye3d;
    glm::vec3           m_up3d;
    float               m_lat;
    float               m_lon;

    // Buffers
    std::vector<Fbo>    m_buffers;
    std::vector<Shader> m_buffers_shaders;
    Vbo*                m_billboard_vbo;
    std::string         m_billboard_vert;
    
    // Background
    Shader              m_background_shader;
    bool                m_background_enabled;

    // CubeMap
    Shader              m_cubemap_shader;
    Vbo*                m_cubemap_vbo;
    TextureCube*        m_cubemap;

    // Recording
    float               m_record_start;
    float               m_record_head;
    float               m_record_end;
    int                 m_record_counter;
    bool                m_record;
};
