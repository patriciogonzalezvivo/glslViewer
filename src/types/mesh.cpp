#include "mesh.h"

#include <iostream>

#include "gl/vertexLayout.h"

Mesh::Mesh():m_drawMode(GL_TRIANGLES) {

}

Mesh::Mesh(const Mesh &_mother):m_drawMode(_mother.getDrawMode()) {
    add(_mother);
}

Mesh::~Mesh() {

}

void Mesh::setDrawMode(GLenum _drawMode) {
    m_drawMode = _drawMode;
}

void Mesh::setColor(const glm::vec4 &_color) {
    m_colors.clear();
    for (uint32_t i = 0; i < m_vertices.size(); i++) {
        m_colors.push_back(_color);
    }
}

void Mesh::addColor(const glm::vec4 &_color) {
    m_colors.push_back(_color);
}

void Mesh::addColors(const std::vector<glm::vec4> &_colors) {
    m_colors.insert(m_colors.end(), _colors.begin(), _colors.end());
}

void Mesh::addVertex(const glm::vec3 &_point) {
   m_vertices.push_back(_point);
}

void Mesh::addVertices(const std::vector<glm::vec3>& _verts) {
   m_vertices.insert(m_vertices.end(),_verts.begin(),_verts.end());
}

void Mesh::addVertices(const glm::vec3* verts, int amt) {
   m_vertices.insert(m_vertices.end(),verts,verts+amt);
}

void Mesh::addNormal(const glm::vec3 &_normal) {
    m_normals.push_back(_normal);
}

void Mesh::addNormals(const std::vector<glm::vec3> &_normals ) {
    m_normals.insert(m_normals.end(), _normals.begin(), _normals.end());
}

void  Mesh::addTangent(const glm::vec4 &_tangent) {
    m_tangents.push_back(_tangent);
}

void Mesh::addTexCoord(const glm::vec2 &_uv) {
    m_texCoords.push_back(_uv);
}

void Mesh::addTexCoords(const std::vector<glm::vec2> &_uvs) {
    m_texCoords.insert(m_texCoords.end(), _uvs.begin(), _uvs.end());
}

void Mesh::addIndex(INDEX_TYPE _i) {
    m_indices.push_back(_i);
}

void Mesh::addIndices(const std::vector<INDEX_TYPE>& inds) {
    m_indices.insert(m_indices.end(),inds.begin(),inds.end());
}

void Mesh::addIndices(const INDEX_TYPE* inds, int amt) {
    m_indices.insert(m_indices.end(),inds,inds+amt);
}

void Mesh::addTriangle(INDEX_TYPE index1, INDEX_TYPE index2, INDEX_TYPE index3) {
    addIndex(index1);
    addIndex(index2);
    addIndex(index3);
}

void Mesh::add(const Mesh &_mesh) {

    if (_mesh.getDrawMode() != m_drawMode) {
        std::cout << "INCOMPATIBLE DRAW MODES" << std::endl;
        return;
    }

    INDEX_TYPE indexOffset = (INDEX_TYPE)getVertices().size();

    addColors(_mesh.getColors());
    addVertices(_mesh.getVertices());
    addNormals(_mesh.getNormals());
    addTexCoords(_mesh.getTexCoords());

    for (uint32_t i = 0; i < _mesh.getIndices().size(); i++) {
        addIndex(indexOffset+_mesh.getIndices()[i]);
    }
}

GLenum Mesh::getDrawMode() const{
    return m_drawMode;
}

const std::vector<glm::vec4> & Mesh::getColors() const{
    return m_colors;
}

const std::vector<glm::vec4> & Mesh::getTangents() const {
    return m_tangents;
}

const std::vector<glm::vec3> & Mesh::getVertices() const{
    return m_vertices;
}

const std::vector<glm::vec3> & Mesh::getNormals() const{
    return m_normals;
}

const std::vector<glm::vec2> & Mesh::getTexCoords() const{
    return m_texCoords;
}

const std::vector<INDEX_TYPE> & Mesh::getIndices() const{
    return m_indices;
}

std::vector<glm::ivec3> Mesh::getTriangles() const {
    std::vector<glm::ivec3> faces;

    if (getDrawMode() == GL_TRIANGLES) {
        if (hasIndices()) {
            for(unsigned int j = 0; j < m_indices.size(); j += 3) {
                glm::ivec3 tri;
                for(int k = 0; k < 3; k++) {
                    tri[k] = m_indices[j+k];
                }
                faces.push_back(tri);
            }
        }
        else {
            for( unsigned int j = 0; j < m_vertices.size(); j += 3) {
                glm::ivec3 tri;
                for(int k = 0; k < 3; k++) {
                    tri[k] = j+k;
                }
                faces.push_back(tri);
            }
        }
    }
    else {
        //  TODO
        //
        std::cout << "ERROR: getTriangles(): Mesh only add GL_TRIANGLES for NOW !!" << std::endl;
    }

    return faces;
}

void Mesh::clear() {
    if (!m_vertices.empty()) {
        m_vertices.clear();
    }
    if (hasColors()) {
        m_colors.clear();
    }
    if (hasNormals()) {
        m_normals.clear();
    }
    if (hasTexCoords()) {
        m_texCoords.clear();
    }
    if (hasTangents()) {
        m_tangents.clear();
    }
    if (hasIndices()) {
        m_indices.clear();
    }
}

bool Mesh::computeNormals() {
    if (getDrawMode() != GL_TRIANGLES) 
        return false;

    //The number of the vertices
    int nV = m_vertices.size();

    //The number of the triangles
    int nT = m_indices.size() / 3;

    std::vector<glm::vec3> norm( nV ); //Array for the normals

    //Scan all the triangles. For each triangle add its
    //normal to norm's vectors of triangle's vertices
    for (int t=0; t<nT; t++) {

        //Get indices of the triangle t
        int i1 = m_indices[ 3 * t ];
        int i2 = m_indices[ 3 * t + 1 ];
        int i3 = m_indices[ 3 * t + 2 ];

        //Get vertices of the triangle
        const glm::vec3 &v1 = m_vertices[ i1 ];
        const glm::vec3 &v2 = m_vertices[ i2 ];
        const glm::vec3 &v3 = m_vertices[ i3 ];

        //Compute the triangle's normal
        glm::vec3 dir = glm::normalize(glm::cross(v2-v1,v3-v1));

        //Accumulate it to norm array for i1, i2, i3
        norm[ i1 ] += dir;
        norm[ i2 ] += dir;
        norm[ i3 ] += dir;
    }

    //Normalize the normal's length and add it.
    m_normals.clear();
    for (int i=0; i<nV; i++) {
        addNormal( glm::normalize(norm[i]) );
    }

    return true;
}

// http://www.terathon.com/code/tangent.html
bool Mesh::computeTangents() {
    //The number of the vertices
    size_t nV = m_vertices.size();

    if (m_texCoords.size() != nV || 
        m_normals.size() != nV || 
        getDrawMode() != GL_TRIANGLES)
        return false;

    //The number of the triangles
    size_t nT = m_indices.size() / 3;

    std::vector<glm::vec3> tan1( nV );
    std::vector<glm::vec3> tan2( nV );

    //Scan all the triangles. For each triangle add its
    //normal to norm's vectors of triangle's vertices
    for (size_t t = 0; t < nT; t++) {

        //Get indices of the triangle t
        int i1 = m_indices[ 3 * t ];
        int i2 = m_indices[ 3 * t + 1 ];
        int i3 = m_indices[ 3 * t + 2 ];

        //Get vertices of the triangle
        const glm::vec3 &v1 = m_vertices[ i1 ];
        const glm::vec3 &v2 = m_vertices[ i2 ];
        const glm::vec3 &v3 = m_vertices[ i3 ];

        const glm::vec2 &w1 = m_texCoords[i1];
        const glm::vec2 &w2 = m_texCoords[i2];
        const glm::vec2 &w3 = m_texCoords[i3];

        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;
        
        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;
        
        float r = 1.0f / (s1 * t2 - s2 * t1);
        glm::vec3 sdir( (t2 * x1 - t1 * x2) * r, 
                        (t2 * y1 - t1 * y2) * r, 
                        (t2 * z1 - t1 * z2) * r);
        glm::vec3 tdir( (s1 * x2 - s2 * x1) * r, 
                        (s1 * y2 - s2 * y1) * r, 
                        (s1 * z2 - s2 * z1) * r);
        
        tan1[i1] += sdir;
        tan1[i2] += sdir;
        tan1[i3] += sdir;
        
        tan2[i1] += tdir;
        tan2[i2] += tdir;
        tan2[i3] += tdir;
    }

    //Normalize the normal's length and add it.
    m_tangents.clear();
    for (size_t i = 0; i < nV; i++) {
        const glm::vec3 &n = m_normals[i];
        const glm::vec3 &t = tan1[i];
        
        // Gram-Schmidt orthogonalize
        glm::vec3 tangent = t - n * glm::dot(n, t);

        // Calculate handedness
        float hardedness = (glm::dot( glm::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;

        addTangent(glm::vec4(tangent, hardedness));
    }

    return true;
}

Vbo* Mesh::getVbo() {

    // Create Vertex Layout
    //
    std::vector<VertexAttrib> attribs;
    attribs.push_back({"position", 3, GL_FLOAT, false, 0});
    int  nBits = 3;

    bool bColor = false;
    if (hasColors() && getColors().size() == m_vertices.size()) {
        attribs.push_back({"color", 4, GL_FLOAT, false, 0});
        bColor = true;
        nBits += 4;
    }

    bool bNormals = false;
    if (hasNormals() && getNormals().size() == m_vertices.size()) {
        attribs.push_back({"normal", 3, GL_FLOAT, false, 0});
        bNormals = true;
        nBits += 3;
    }

    bool bTexCoords = false;
    if (hasTexCoords()  && getTexCoords().size() == m_vertices.size()) {
        attribs.push_back({"texcoord", 2, GL_FLOAT, false, 0});
        bTexCoords = true;
        nBits += 2;
    }

    bool bTangents = false;
    if (hasTangents() && getTangents().size() == m_vertices.size()) {
        attribs.push_back({"tangent", 4, GL_FLOAT, false, 0});
        bTangents = true;
        nBits += 4;
    }

    VertexLayout* vertexLayout = new VertexLayout(attribs);
    Vbo* tmpMesh = new Vbo(vertexLayout);
    tmpMesh->setDrawMode(getDrawMode());

    std::vector<GLfloat> data;
    for (unsigned int i = 0; i < m_vertices.size(); i++) {
        data.push_back(m_vertices[i].x);
        data.push_back(m_vertices[i].y);
        data.push_back(m_vertices[i].z);
        if (bColor) {
            data.push_back(m_colors[i].r);
            data.push_back(m_colors[i].g);
            data.push_back(m_colors[i].b);
            data.push_back(m_colors[i].a);
        }
        if (bNormals) {
            data.push_back(m_normals[i].x);
            data.push_back(m_normals[i].y);
            data.push_back(m_normals[i].z);
        }
        if (bTexCoords) {
            data.push_back(m_texCoords[i].x);
            data.push_back(m_texCoords[i].y);
        }
        if (bTangents) {
            data.push_back(m_tangents[i].x);
            data.push_back(m_tangents[i].y);
            data.push_back(m_tangents[i].z);
            data.push_back(m_tangents[i].w);
        }
    }

    tmpMesh->addVertices((GLbyte*)data.data(), m_vertices.size());
    
    if (!hasIndices()) {
        if ( getDrawMode() == GL_LINES ) {
            for (uint32_t i = 0; i < getVertices().size(); i++) {
                addIndex(i);
            }
        }
        else if ( getDrawMode() == GL_LINE_STRIP ) {
            for (uint32_t i = 1; i < getVertices().size(); i++) {
                addIndex(i-1);
                addIndex(i);
            }
        }
    }

    tmpMesh->addIndices(m_indices.data(), m_indices.size());    

    return tmpMesh;
}
