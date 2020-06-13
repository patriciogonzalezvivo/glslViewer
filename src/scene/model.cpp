#include "model.h"

#include "tools/text.h"
#include "tools/geom.h"

Model::Model():
    m_model_vbo(nullptr), m_bbox_vbo(nullptr), 
    m_bbmin(100000.0), m_bbmax(-1000000.),
    m_name(""), m_area(0.0f) {

#if defined(PLATFORM_RPI) || defined(PLATFORM_RPI4) 
    addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
    addDefine("LIGHT_SHADOWMAP_SIZE", "512.0");
#else
    addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
    addDefine("LIGHT_SHADOWMAP_SIZE", "1024.0");
#endif
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
        delDefine( "MODEL_NAME_" + toUpper( toUnderscore( purifyString(m_name) ) ) );

    if (!_str.empty()) {
        m_name = toLower( toUnderscore( purifyString(_str) ) );
        addDefine( "MODEL_NAME_" + toUpper( m_name ) );
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
        addDefine("MODEL_VERTEX_COLOR", "v_color");

    if (_mesh.hasNormals())
        addDefine("MODEL_VERTEX_NORMAL", "v_normal");

    if (_mesh.hasTexCoords())
        addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");

    if (_mesh.hasTangents())
        addDefine("MODEL_VERTEX_TANGENT", "v_tangent");

    if (_mesh.getDrawMode() == GL_POINTS)
        addDefine("MODEL_PRIMITIVE_POINTS");
    else if (_mesh.getDrawMode() == GL_LINES)
        addDefine("MODEL_PRIMITIVE_LINES");
    else if (_mesh.getDrawMode() == GL_LINE_STRIP)
        addDefine("MODEL_PRIMITIVE_LINE_STRIP");
    else if (_mesh.getDrawMode() == GL_TRIANGLES)
        addDefine("MODEL_PRIMITIVE_TRIANGLES");
    else if (_mesh.getDrawMode() == GL_TRIANGLE_FAN)
        addDefine("MODEL_PRIMITIVE_TRIANGLE_FAN");

    addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
    addDefine("LIGHT_SHADOWMAP_SIZE", "1024.0");

    return true;
}

bool Model::loadMaterial(const Material &_material) {
    m_shader.mergeDefines(&_material);
    return true;
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

void Model::render(Uniforms& _uniforms, const glm::mat4& _viewProjectionMatrix) {

    // If the model and the shader are loaded
    if ( m_model_vbo && m_shader.isLoaded() ) {

        // bind the shader
        m_shader.use();

        // Update Uniforms and textures variables to the shader
        _uniforms.feedTo( m_shader);

        // Pass special uniforms
        m_shader.setUniform( "u_modelViewProjectionMatrix", _viewProjectionMatrix);
        render( &m_shader );
    }
}

void Model::render(Shader* _shader) {
    m_model_vbo->render(_shader);
}

void Model::renderBbox(Shader* _shader) {
    if (m_bbox_vbo)
        m_model_vbo->render(_shader);
}
