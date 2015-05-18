#pragma once

#include <vector>
#include <string>
#include <map>

#include "gl.h"
#include "shader.h"

enum AttrType {
    POSITION_ATTRIBUTE,
    COLOR_ATTRIBUTE,
    NORMAL_ATTRIBUTE,
    TEXCOORD_ATTRIBUTE,
    OTHER_ATTRIBUTE
};

class VertexLayout {
public:

    struct VertexAttrib {
        std::string name;
        GLint size;
        GLenum type;
        AttrType attrType;
        GLboolean normalized;
        GLvoid* offset; // Can be left as zero; value is overwritten in constructor of VertexLayout
    };

    VertexLayout(std::vector<VertexAttrib> _attribs);
    virtual ~VertexLayout();

    void        enable(const Shader* _program);

    GLint       getStride() const { return m_stride; };

    std::string getDefaultVertShader();
    std::string getDefaultFragShader();

private:

    static std::map<GLint, GLuint> s_enabledAttribs; // Map from attrib locations to bound shader program

    std::vector<VertexAttrib> m_attribs;
    GLint m_stride;

    int m_positionAttribIndex;
    int m_colorAttribIndex;
    int m_normalAttribIndex;
    int m_texCoordAttribIndex;

};
