#pragma once

#include <vector>
#include <string>

#include "glm/glm.hpp"

#include "../gl/vbo.h"

#ifdef PLATFORM_RPI
#define INDEX_TYPE uint16_t
#else
#define INDEX_TYPE uint32_t
#endif

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

    void    addIndex(INDEX_TYPE _i);
    void    addIndices(const std::vector<INDEX_TYPE>& _inds);
    void    addIndices(const INDEX_TYPE* _inds, int _amt);

    void    addTriangle(INDEX_TYPE index1, INDEX_TYPE index2, INDEX_TYPE index3);

    void    add(const Mesh &_mesh);

    const bool    hasColors() const { return m_colors.size() > 0; }
    const bool    hasNormals() const { return m_normals.size() > 0; }
    const bool    hasTexCoords() const { return m_texCoords.size() > 0; }
    const bool    hasTangents() const { return m_tangents.size() > 0; }
    const bool    hasIndices() const { return m_indices.size() > 0; }

    Vbo*    getVbo();
    GLenum  getDrawMode() const;
    std::vector<glm::ivec3>  getTriangles() const ;

    const std::vector<glm::vec4> & getColors() const;
    const std::vector<glm::vec4> & getTangents() const;
    const std::vector<glm::vec3> & getVertices() const;
    const std::vector<glm::vec3> & getNormals() const;
    const std::vector<glm::vec2> & getTexCoords() const;
    const std::vector<INDEX_TYPE>  & getIndices() const;

    void    computeNormals();
    void    computeTangents();
    void    clear();

private:
    std::vector<glm::vec4>  m_colors;
    std::vector<glm::vec4>  m_tangents;
    std::vector<glm::vec3>  m_vertices;
    std::vector<glm::vec3>  m_normals;
    std::vector<glm::vec2>  m_texCoords;
    std::vector<INDEX_TYPE>   m_indices;

    GLenum    m_drawMode;
};
