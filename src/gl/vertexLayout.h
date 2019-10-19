#pragma once

#include <vector>
#include <string>
#include <map>

#include "gl.h"
#include "shader.h"

struct VertexAttrib {
    std::string name;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLvoid* offset; // Can be left as zero; value is overwritten in constructor of VertexLayout
};

class VertexLayout {
public:

    VertexLayout(const std::vector<VertexAttrib>& _attribs);
    VertexLayout(const std::vector<VertexAttrib>& _attribs, GLint _stride);
    virtual ~VertexLayout();

    void        enable(const Shader* _program);

    GLint       getStride() const { return m_stride; };

    void        printAttrib();

private:
    static std::map<GLint, GLuint> s_enabledAttribs; // Map from attrib locations to bound shader program

    std::vector<VertexAttrib> m_attribs;
    GLint m_stride;
};
