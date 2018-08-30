#pragma once

#include <vector>
#include <string>

#include "glm/glm.hpp"

#include "../gl/vbo.h"

class Mesh {
public:

    Mesh();
    Mesh(const Mesh &_mother);
    virtual ~Mesh();

    bool    load(const std::string& _file);
    bool    save(const std::string& _file, bool _useBinary = false);

    void    setDrawMode(GLenum _drawMode = GL_TRIANGLES);

    void    setColor(const glm::vec4 &_color);
    void    addColor(const glm::vec4 &_color);
    void    addColors(const std::vector<glm::vec4> &_colors);

    void    addVertex(const glm::vec3 &_point);
    void    addVertices(const std::vector<glm::vec3>& _verts);
    void    addVertices(const glm::vec3* _verts, int _amt);

    void    addNormal(const glm::vec3 &_normal);
    void    addNormals(const std::vector<glm::vec3> &_normals );

    void    addTangent(const glm::vec4 &_tangent);

    void    addTexCoord(const glm::vec2 &_uv);
    void    addTexCoords(const std::vector<glm::vec2> &_uvs);

    void    addIndex(uint16_t _i);
    void    addIndices(const std::vector<uint16_t>& _inds);
    void    addIndices(const uint16_t* _inds, int _amt);

    void    addTriangle(uint16_t index1, uint16_t index2, uint16_t index3);

    void    add(const Mesh &_mesh);

    const bool    hasColors() const { return m_colors.size() > 0; }
    const bool    hasNormals() const { return m_normals.size() > 0; }
    const bool    hasTexCoords() const { return m_texCoords.size() > 0; }
    const bool    hasTangents() const { return m_tangents.size() > 0; }
    const bool    hasIndices() const { return m_indices.size() > 0; }

    GLenum  getDrawMode() const;
    std::vector<glm::ivec3>  getTriangles() const ;

    const std::vector<glm::vec4> & getColors() const;
    const std::vector<glm::vec4> & getTangents() const;
    const std::vector<glm::vec3> & getVertices() const;
    const std::vector<glm::vec3> & getNormals() const;
    const std::vector<glm::vec2> & getTexCoords() const;
    const std::vector<uint16_t>  & getIndices() const;

    Vbo*    getVbo();

    void    computeNormals();
    void    computeTangents();
    void    clear();

private:
    std::vector<glm::vec4>  m_colors;
    std::vector<glm::vec4>  m_tangents;
    std::vector<glm::vec3>  m_vertices;
    std::vector<glm::vec3>  m_normals;
    std::vector<glm::vec2>  m_texCoords;
    std::vector<uint16_t>   m_indices;

    GLenum    m_drawMode;
};
