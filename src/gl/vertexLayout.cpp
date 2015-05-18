#include "vertexLayout.h"
#include "utils.h"

std::map<GLint, GLuint> VertexLayout::s_enabledAttribs = std::map<GLint, GLuint>();

VertexLayout::VertexLayout(std::vector<VertexAttrib> _attribs) : m_attribs(_attribs), m_stride(0), m_positionAttribIndex(-1), m_colorAttribIndex(-1), m_normalAttribIndex(-1), m_texCoordAttribIndex(-1) {

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

        if ( m_attribs[i].attrType == POSITION_ATTRIBUTE ){
            m_positionAttribIndex = i;
        } else if ( m_attribs[i].attrType == COLOR_ATTRIBUTE ){
            m_colorAttribIndex = i;
        } else if ( m_attribs[i].attrType == NORMAL_ATTRIBUTE ){
            m_normalAttribIndex = i;
        } else if ( m_attribs[i].attrType == TEXCOORD_ATTRIBUTE ){
            m_texCoordAttribIndex = i;
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
        const GLint location = _program->getAttribLocation("a_"+m_attribs[i].name);
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

std::string VertexLayout::getDefaultVertShader() {
    std::string rta = 
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n"
"\n"
"uniform mat4 u_modelViewProjectionMatrix;\n"
"uniform mat4 u_modelMatrix;\n"
"uniform mat4 u_viewMatrix;\n"
"uniform mat4 u_projectionMatrix;\n"
"uniform mat4 u_normalMatrix;\n"
"\n"
"uniform float u_time;\n"
"uniform vec2 u_mouse;\n"
"uniform vec2 u_resolution;\n"
"\n";

    for ( uint i = 0; i < m_attribs.size(); i++ ){
        int size = m_attribs[i].size;
        if (m_positionAttribIndex == i){
            size = 4;
        }
        rta += "attribute vec" + getString(size) + " a_" + m_attribs[i].name + ";\n";
        rta += "varying vec" + getString(size) + " v_" + m_attribs[i].name + ";\n";
    }

    rta += "\n"
"void main(void) {\n"
"\n";

    for ( uint i = 0; i < m_attribs.size(); i++ ){
        rta += "    v_" + m_attribs[i].name + " = a_" + m_attribs[i].name + ";\n";
    }

    if (m_positionAttribIndex != -1 && m_positionAttribIndex < m_attribs.size()){
        rta += "    gl_Position = u_modelViewProjectionMatrix * v_" + m_attribs[m_positionAttribIndex].name + ";\n";
    }
    
    rta +=  "}\n";

    return rta;
}

std::string VertexLayout::getDefaultFragShader() {
    std::string rta = 
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n"
"\n"
"uniform mat4 u_modelViewProjectionMatrix;\n"
"uniform mat4 u_modelMatrix;\n"
"uniform mat4 u_viewMatrix;\n"
"uniform mat4 u_projectionMatrix;\n"
"uniform mat4 u_normalMatrix;\n"
"\n"
"uniform float u_time;\n"
"uniform vec2 u_mouse;\n"
"uniform vec2 u_resolution;\n"
"\n";

    for ( uint i = 0; i < m_attribs.size(); i++ ){
        int size = m_attribs[i].size;
        if (m_positionAttribIndex == i){
            size = 4;
        }
        rta += "varying vec" + getString(size) + " v_" + m_attribs[i].name + ";\n";
    }

    rta += "\n"
"void main(void) {\n"
"\n";

    if ( m_colorAttribIndex != -1 ){
        rta += "    gl_FragColor = v_" + m_attribs[m_colorAttribIndex].name + ";\n";
    } else if ( m_texCoordAttribIndex != -1 ){
        rta += "    gl_FragColor = vec4(vec3(v_" + m_attribs[m_texCoordAttribIndex].name + ",1.0),1.0);\n";
    } else if ( m_normalAttribIndex != -1 ){
        rta += "    gl_FragColor = vec4(0.5+v_" + m_attribs[m_normalAttribIndex].name + "*0.5,1.0);\n";
    } else {
        rta += "    gl_FragColor = vec4(1.0);\n";
    }
    
    rta +=  "}\n";

    return rta;
}
