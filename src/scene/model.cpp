#include "model.h"

#include "tools/geom.h"
#include "tools/shapes.h"

Model::Model():
    m_model_vbo(nullptr), m_bbox_vbo(nullptr), 
    m_area(0.0f) {
}

Model::~Model() {
    clear();
}

void Model::clear() {
    if (m_model_vbo) {
        delete m_model_vbo;
        m_model_vbo = nullptr;
    }

    if (m_bbox_vbo) {
        delete m_bbox_vbo;
        m_bbox_vbo = nullptr;
    }
}

void Model::load(const std::string &_path, List &_defines) {
    Mesh mesh;
    mesh.load( _path );
    m_model_vbo = mesh.getVbo();
    setPosition( -getCentroid( mesh.getVertices() ) );

    glm::vec3 min_v;
    glm::vec3 max_v;
    getBoundingBox( mesh.getVertices(), min_v, max_v);
    m_area = glm::min(glm::length(min_v), glm::length(max_v));
    m_bbox_vbo = cubeCorners( min_v, max_v, 0.25 ).getVbo();

    if (mesh.hasColors()) {
        _defines.push_back("MODEL_HAS_COLORS");
    }

    if (mesh.hasNormals()) {
        _defines.push_back("MODEL_HAS_NORMALS");
    }

    if (mesh.hasTexCoords()) {
        _defines.push_back("MODEL_HAS_TEXCOORDS");
    }

    if (mesh.hasTangents()) {
        _defines.push_back("MODEL_HAS_TANGENTS");
    }

}

void Model::draw(){

}
