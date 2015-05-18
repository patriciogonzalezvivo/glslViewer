#include "vbo.h"
#include <iostream>

Vbo::Vbo(VertexLayout* _vertexLayout, GLenum _drawMode) : m_vertexLayout(_vertexLayout), m_glVertexBuffer(0), m_nVertices(0), m_glIndexBuffer(0), m_nIndices(0), m_isUploaded(false) {
    setDrawMode(_drawMode);
}

Vbo::Vbo() : m_vertexLayout(NULL), m_glVertexBuffer(0), m_nVertices(0), m_glIndexBuffer(0), m_nIndices(0), m_isUploaded(false) {
}

Vbo::~Vbo() {
    glDeleteBuffers(1, &m_glVertexBuffer);
    glDeleteBuffers(1, &m_glIndexBuffer);
    
    m_vertexData.clear();
    m_indices.clear();

    if (m_vertexLayout != NULL){
        delete m_vertexLayout;
    }
}

void Vbo::setVertexLayout(VertexLayout* _vertexLayout) {
    if (m_vertexLayout != NULL){
        delete m_vertexLayout;
    }
    m_vertexLayout = _vertexLayout;
}

void Vbo::setDrawMode(GLenum _drawMode) {
    switch (_drawMode) {
        case GL_POINTS:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        case GL_LINES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_TRIANGLES:
            m_drawMode = _drawMode;
            break;
        default:
            std::cout << "Invalid draw mode for mesh! Defaulting to GL_TRIANGLES" << std::endl;
            m_drawMode = GL_TRIANGLES;
    }
}

void Vbo::addVertex(GLbyte* _vertex) {
    addVertices(_vertex, 1);
}

void Vbo::addVertices(GLbyte* _vertices, int _nVertices) {
    if (m_isUploaded) {
        std::cout << "Vbo cannot add vertices after upload!" << std::endl;
        return;
    }
    
    // Only add up to 65535 vertices, any more will overflow our 16-bit indices
    int indexSpace = MAX_INDEX_VALUE - m_nVertices;
    if (_nVertices > indexSpace) {
        _nVertices = indexSpace;
        std::cout << "WARNING: Tried to add more vertices than available in index space" << std::endl;
    }

    int vertexBytes = m_vertexLayout->getStride() * _nVertices;
    m_vertexData.insert(m_vertexData.end(), _vertices, _vertices + vertexBytes);
    m_nVertices += _nVertices;
}

void Vbo::addIndex(GLushort* _index) {
    addIndices(_index, 1);
}

void Vbo::addIndices(GLushort* _indices, int _nIndices) {
    if (m_isUploaded) {
        std::cout << "Vbo cannot add indices after upload!" << std::endl;
        return;
    }
    
    if (m_nVertices >= MAX_INDEX_VALUE) {
        std::cout << "WARNING: Vertex buffer full, not adding indices" << std::endl;
        return;
    }

    m_indices.insert(m_indices.end(), _indices, _indices + _nIndices);
    m_nIndices += _nIndices;
}

void Vbo::upload() {
    if (m_nVertices > 0) {
        // Generate vertex buffer, if needed
        if (m_glVertexBuffer == 0) {
            glGenBuffers(1, &m_glVertexBuffer);
        }
        
        // Buffer vertex data
        glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, m_vertexData.size(), m_vertexData.data(), GL_STATIC_DRAW);
    }

    if (m_nIndices > 0) {
        // Generate index buffer, if needed
        if (m_glIndexBuffer == 0) {
            glGenBuffers(1, &m_glIndexBuffer);
        }

        // Buffer element index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLushort), m_indices.data(), GL_STATIC_DRAW);
    }

    m_vertexData.clear();
    m_indices.clear();

    m_isUploaded = true;
}

void Vbo::draw(const Shader* _shader) {

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        upload();
    }
    
    // Bind buffers for drawing
    if (m_nVertices > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
    }

    if (m_nIndices > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
    }

    // Enable shader program
    _shader->use();

    // Enable vertex attribs via vertex layout object
    m_vertexLayout->enable(_shader);

    // Draw as elements or arrays
    if (m_nIndices > 0) {
        glDrawElements(m_drawMode, m_nIndices, GL_UNSIGNED_SHORT, 0);
    } else if (m_nVertices > 0) {
        glDrawArrays(m_drawMode, 0, m_nVertices);
    }
}
