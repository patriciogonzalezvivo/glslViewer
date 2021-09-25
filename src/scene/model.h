#pragma once

#include <vector>

#include "ada/gl/vbo.h"
#include "ada/gl/mesh.h"
#include "ada/gl/shader.h"
#include "ada/scene/node.h"

#include "material.h"


#include "../uniforms.h"

class Model : public ada::Node {
public:
    Model();
    Model(const std::string& _name, ada::Mesh& _mesh, const Material& _mat);
    virtual ~Model();

    bool        loadGeom(ada::Mesh& _mesh);
    bool        loadShader(const std::string& _fragStr, const std::string& _vertStr, bool verbose);
    bool        loadMaterial(const Material& _material);

    bool        loaded() const { return m_model_vbo != nullptr; }
    void        clear();

    void        setName(const std::string& _str);
    std::string getName() { return m_name; }

    void        addDefine(const std::string& _define, const std::string& _value = "");
    void        delDefine(const std::string& _define);
    void        printDefines();
    void        printVboInfo();

    float       getArea() const { return m_area; }
    glm::vec3   getMinBoundingBox() const { return m_bbmin; }
    glm::vec3   getMaxBoundingBox() const { return m_bbmax; }
    
    void        render(Uniforms& _uniforms, const glm::mat4& _viewProjectionMatrix );
    void        render(ada::Shader* _shader);
    void        renderBbox(ada::Shader* _shader);

protected:
    ada::Shader m_shader;
    
    ada::Vbo*   m_model_vbo;
    ada::Vbo*   m_bbox_vbo;

    glm::vec3   m_bbmin;
    glm::vec3   m_bbmax;

    std::string m_name;
    float       m_area;
};

typedef std::vector<Model*>  Models;
