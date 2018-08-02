#pragma once

#include "gl/shader.h"
#include "gl/vbo.h"
#include "gl/texture.h"
#include "gl/pingpong.h"
#include "gl/uniform.h"

#include "3d/camera.h"

#include "tools/fs.h"

class Sandbox {
public:
    Sandbox();
    
    // Main stages
    void setup(WatchFileList &_files);
    void draw();
    void clean();

    // Getting some data out of Sandbox
    std::string getVertexSource() const { return m_vertSource; } 
    std::string getFragmentSource() const { return m_fragSource; }
    std::string get3DView() const;
    int         getTotalBuffers() const { return m_buffers.size(); }
    
    // Some events
    void onFileChange(WatchFileList &_files, int _index);
    void onScroll(float _yoffset);
    void onScreenshot(std::string _file);
    void onMouseDrag(float _x, float _y, int _button);
    void onViewportResize(int _newWidth, int _newHeight);
   
    // Uniforms
    UniformList                     uniforms;

    // Textures
    std::map<std::string,Texture*>  textures;

    // Defines
    std::vector<std::string>        defines;

    // Include folders
    std::vector<std::string>        include_folders;

    // Screenshot file
    std::string screenshotFile;

    // States
    int iFrag;
    int iVert;
    int iGeom;

    bool verbose;
    bool vFlip;

private:
    void _updateBuffers();
    void _updateUniforms( Shader &_shader );
    void _updateTextures( Shader &_shader, uint &_textureIndex  );
    
    //  Shader
    Shader              m_shader;

    std::string         m_fragSource;
    const std::string*  m_fragPath;
    
    std::string         m_vertSource;
    const std::string*  m_vertPath;

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
};
