#pragma once

#include "node.h"

#include "gl/vbo.h"
#include "gl/shader.h"

#include "types/mesh.h"

#include "tools/uniform.h"

class Model : public Node {
public:
    Model();
    Model(Mesh &_mesh);
    Model(Mesh &_mesh, const std::string &_fragStr, const std::string &_vertStr, bool verbose);
    virtual ~Model();

    bool        loadGeom(Mesh &_mesh);
    bool        loadShader(const std::string &_fragStr, const std::string &_vertStr, bool verbose);

    bool        loaded() const { return m_model_vbo != nullptr; }
    void        clear();

    void        addDefine(const std::string &_define, const std::string &_value = "");
    void        delDefine(const std::string &_define);
    void        printDefines();

    Vbo*        getVbo() { return m_model_vbo; }
    Vbo*        getBboxVbo() { return m_bbox_vbo; }

    float       getArea() const { return m_area; }
    glm::vec3   getMinBoundingBox() const { return m_bbmin; }
    glm::vec3   getMaxBoundingBox() const { return m_bbmax; }
    
    void        draw(Uniforms &_uniforms, const glm::mat4 &_viewProjectionMatrix );

protected:
    Shader      m_shader;
    
    Vbo*        m_model_vbo;
    Vbo*        m_bbox_vbo;

    glm::vec3   m_bbmin;
    glm::vec3   m_bbmax;

    float       m_area;
};