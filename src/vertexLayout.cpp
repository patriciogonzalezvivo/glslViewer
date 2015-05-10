#include "vertexLayout.h"
#include "utils.h"

std::map<GLint, GLuint> VertexLayout::s_enabledAttribs = std::map<GLint, GLuint>();

VertexLayout::VertexLayout(std::vector<VertexAttrib> _attribs) : m_attribs(_attribs) {

    m_stride = 0; 

    for (unsigned int i = 0; i < m_attribs.size(); i++) {

        // Set the offset of this vertex attribute: The stride at this point denotes the number
        // of bytes into the vertex by which this attribute is offset, but we must cast the number
        // as a void* to use with glVertexAttribPointer; We use reinterpret_cast to avoid warnings
        m_attribs[i].offset = reinterpret_cast<void*>(m_stride);

        GLint byteSize = m_attribs[i].size;

        switch (m_attribs[i].type) {
            case GL_FLOAT:
            case GL_INT:
            case GL_UNSIGNED_INT:
                byteSize *= 4; // 4 bytes for floats, ints, and uints
                break;
            case GL_SHORT:
            case GL_UNSIGNED_SHORT:
                byteSize *= 2; // 2 bytes for shorts and ushorts
                break;
        }

        m_stride += byteSize;

        // TODO: Automatically add padding or warn if attributes are not byte-aligned

    }
}

VertexLayout::~VertexLayout() {
    m_attribs.clear();
}

void VertexLayout::enable(const Shader* _program) {
    GLuint glProgram = _program->getProgram();

    // Enable all attributes for this layout
    for (unsigned int i = 0; i < m_attribs.size(); i++) {
        const GLint location = _program->getAttribLocation(m_attribs[i].name);
        if (location != -1) {
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, m_attribs[i].size, m_attribs[i].type, m_attribs[i].normalized, m_stride, m_attribs[i].offset);
            s_enabledAttribs[location] = glProgram; // Track currently enabled attribs by the program to which they are bound
        }
    }

    // Disable previously bound and now-unneeded attributes
    for (std::map<GLint, GLuint>::iterator it=s_enabledAttribs.begin(); it!=s_enabledAttribs.end(); ++it){
        const GLint& location = it->first;
        GLuint& boundProgram = it->second;

        if (boundProgram != glProgram && boundProgram != 0) {
            glDisableVertexAttribArray(location);
            boundProgram = 0;
        }
    }
}

std::string VertexLayout::getDefaultVertShader(){
    std::string rta = 
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

    rta +=
"uniform mat4 u_modelViewProjectionMatrix;\n"
"uniform float u_time;\n"
"uniform vec2 u_mouse;\n"
"uniform vec2 u_resolution;\n";

    for ( uint i = 0; i < m_attribs.size(); i++ ){
        rta += "attribute vec" + getString(m_attribs[i].size) + " " + m_attribs[i].name + ";\n";
    }

    rta +=
"varying vec4 v_position;\n"
"varying vec4 v_color;\n"
"varying vec3 v_normal;\n"
"varying vec2 v_texcoord;\n"
"void main(void) {\n";

    rta +=
"    v_position = u_modelViewProjectionMatrix * a_position;\n"
"    gl_Position = v_position;\n"
"    v_color = a_color;\n"
"    v_normal = a_normal;\n"
"    v_texcoord = a_texcoord;\n";

    rta +=  "}\n";

    return rta;
}

std::string VertexLayout::getDefaultFragShader(){
    std::string rta;

    return rta;
}
