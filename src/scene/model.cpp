#include "model.h"

#include "tools/text.h"
#include "tools/geom.h"
#include "tools/shapes.h"

Model::Model():
    m_model_vbo(nullptr), m_bbox_vbo(nullptr), 
    m_bbmin(100000.0), m_bbmax(-1000000.),
    m_name(""), m_area(0.0f) {
}

Model::Model(const std::string& _name, Mesh &_mesh, const Material &_mat):
    m_model_vbo(nullptr), m_bbox_vbo(nullptr), 
    m_area(0.0f) {
    setName(_name);
    loadGeom(_mesh);
    loadMaterial(_mat);
}

void Model::setName(const std::string& _str) {
    if (!m_name.empty())
        delDefine( "MODEL_" + toUpper( toUnderscore( purifyString(m_name) ) ) );

    if (!_str.empty()) {
        m_name = toLower( toUnderscore( purifyString(_str) ) );
        addDefine( "MODEL_" + toUpper( m_name ) );
    }
}

void Model::addDefine(const std::string& _define, const std::string& _value) { 
    m_shader.addDefine(_define, _value); 
}

void Model::delDefine(const std::string& _define) { 
    m_shader.delDefine(_define); 
};

void Model::printDefines() {
    m_shader.printDefines();
}

void Model::printVboInfo() {
    if (m_model_vbo)
        m_model_vbo->printInfo();
}

bool Model::loadGeom(Mesh& _mesh) {
    // Load Geometry VBO
    m_model_vbo = _mesh.getVbo();

    getBoundingBox( _mesh.getVertices(), m_bbmin, m_bbmax);
    m_area = glm::min(glm::length(m_bbmin), glm::length(m_bbmax));
    m_bbox_vbo = cubeCorners( m_bbmin, m_bbmax, 0.25 ).getVbo();

    // Setup Shader and GEOMETRY DEFINE FLAGS
    if (_mesh.hasColors())
        addDefine("MODEL_HAS_COLORS");

    if (_mesh.hasNormals())
        addDefine("MODEL_HAS_NORMALS");

    if (_mesh.hasTexCoords())
        addDefine("MODEL_HAS_TEXCOORDS");

    if (_mesh.hasTangents())
        addDefine("MODEL_HAS_TANGENTS");

    addDefine("SHADOW_MAP", "u_ligthShadowMap");
#ifdef PLATFORM_RPI
    addDefine("SHADOW_MAP_SIZE", "512.0");
#else
    addDefine("SHADOW_MAP_SIZE", "1024.0");
#endif

    return true;
}

bool Model::loadMaterial(const Material &_material) {
    return _material.feedProperties(m_shader);
}

bool Model::loadShader(const std::string& _fragStr, const std::string& _vertStr, bool verbose) {
    if (m_shader.isLoaded())
        m_shader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

    return m_shader.load( _fragStr, _vertStr, verbose);
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

void Model::draw(Uniforms& _uniforms, const glm::mat4& _viewProjectionMatrix) {

    // If the model and the shader are loaded
    if ( m_model_vbo && m_shader.isLoaded() ) {

        // bind the shader
        m_shader.use();

        // Update Uniforms and textures variables to the shader
        _uniforms.feedTo( m_shader);

        // Pass special uniforms
        m_shader.setUniform( "u_modelViewProjectionMatrix", _viewProjectionMatrix );
        m_model_vbo->render( &m_shader );
    }
}