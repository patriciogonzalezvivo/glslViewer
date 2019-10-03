#include "gltf.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "../gl/vbo.h"
#include "../tools/fs.h"
#include "../tools/geom.h"
#include "../tools/text.h"

#define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION
// #define JSON_NOEXCEPTION
#include "tinygltf/tiny_gltf.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

bool loadModel(tinygltf::Model& _model, const std::string& _filename) {
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    std::string ext = getExt(_filename);

    bool res = false;

    // assume binary glTF.
    if (ext == "glb" || ext == "GLB")
        res = loader.LoadBinaryFromFile(&_model, &err, &warn, _filename.c_str());

    // assume ascii glTF.
    else
        res = loader.LoadASCIIFromFile(&_model, &err, &warn, _filename.c_str());

    if (!warn.empty())
        std::cout << "Warn: " << warn.c_str() << std::endl;

    if (!err.empty())
        std::cout << "ERR: " << err.c_str() << std::endl;

    return res;
}

GLenum extractMode(const tinygltf::Primitive& _primitive) {
    if (_primitive.mode == TINYGLTF_MODE_TRIANGLES) {
      return GL_TRIANGLES;
    } else if (_primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
      return GL_TRIANGLE_STRIP;
    } else if (_primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) {
      return GL_TRIANGLE_FAN;
    } else if (_primitive.mode == TINYGLTF_MODE_POINTS) {
      return GL_POINTS;
    } else if (_primitive.mode == TINYGLTF_MODE_LINE) {
      return GL_LINES;
    } else if (_primitive.mode == TINYGLTF_MODE_LINE_LOOP) {
      return GL_LINE_LOOP;
    } else {
      return 0;
    }
}

void extractIndices(const tinygltf::Model& _model, const tinygltf::Accessor& _indexAccessor, Mesh& _mesh) {
    const tinygltf::BufferView &buffer_view = _model.bufferViews[_indexAccessor.bufferView];
    const tinygltf::Buffer &buffer = _model.buffers[buffer_view.buffer];
    const uint8_t* base = &buffer.data.at(buffer_view.byteOffset + _indexAccessor.byteOffset);

    switch (_indexAccessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            const uint32_t *p = (uint32_t*) base;
            for (size_t i = 0; i < _indexAccessor.count; ++i) {
                _mesh.addIndex( p[i] );
            }
        }; break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            const uint16_t *p = (uint16_t*) base;
            for (size_t i = 0; i < _indexAccessor.count; ++i) {
                _mesh.addIndex( p[i] );
            }
        }; break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            const uint8_t *p = (uint8_t*) base;
            for (size_t i = 0; i < _indexAccessor.count; ++i) {
                _mesh.addIndex( p[i] );
            }
        }; break;
    }
}

void extractVertexData(uint32_t v_pos, const uint8_t *base, int accesor_componentType, int accesor_type, bool accesor_normalized, uint32_t byteStride, float *output, uint8_t max_num_comp) {
    float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    uint32_t ncomp = 1;
    switch (accesor_type) {
        case TINYGLTF_TYPE_SCALAR: ncomp = 1; break;
        case TINYGLTF_TYPE_VEC2:   ncomp = 2; break;
        case TINYGLTF_TYPE_VEC3:   ncomp = 3; break;
        case TINYGLTF_TYPE_VEC4:   ncomp = 4; break;
        default:
            assert(!"invalid type");
    }
    switch (accesor_componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
            const float *data = (float*)(base+byteStride*v_pos);
            for (uint32_t i = 0; (i < ncomp); ++i) {
                v[i] = data[i];
            }
        }
        // TODO SUPPORT OTHER FORMATS
        break;
        default:
            assert(!"Conversion Type from float to -> ??? not implemented yet");
            break;
    }
    for (uint32_t i = 0; i < max_num_comp; ++i) {
        output[i] = v[i];
    }
}

Material extractMaterial(tinygltf::Material& _material) {
    Material mat;
    mat.name = toLower( toUnderscore( purifyString( _material.name ) ) );

    mat.addDefine("MATERIAL_NAME_" + toUpper(mat.name) );
    mat.addDefine("MATERIAL_EMISSIVE", (double*)_material.emissiveFactor.data(), 3);
    mat.addDefine("MATERIAL_BASECOLOR", (double*)_material.pbrMetallicRoughness.baseColorFactor.data(), 4);
    mat.addDefine("MATERIAL_ROUGHNESS", _material.pbrMetallicRoughness.roughnessFactor);
    mat.addDefine("MATERIAL_METALLIC", _material.pbrMetallicRoughness.metallicFactor);

    return mat;
}

void extractMesh(tinygltf::Model& _model, tinygltf::Mesh& _mesh, Models& _models, bool _verbose) {
    std::cout << "Mesh " << _mesh.name << std::endl;
    for (size_t i = 0; i < _mesh.primitives.size(); ++i) {
        tinygltf::Primitive primitive = _mesh.primitives[i];

        Mesh mesh;
        extractIndices(_model, _model.accessors[primitive.indices], mesh);
        mesh.setDrawMode(extractMode(primitive));

        // Extract Vertex Data
        for (auto &attrib : primitive.attributes) {
            tinygltf::Accessor accessor = _model.accessors[attrib.second];
            const tinygltf::BufferView &bufferView = _model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = _model.buffers[bufferView.buffer];
            int byteStride = accessor.ByteStride(bufferView);

            if (attrib.first.compare("POSITION") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec3 pos;
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &pos[0], 3);
                    mesh.addVertex(pos);
                }
            }

            else if (attrib.first.compare("COLOR") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec4 col = glm::vec4(1.0f);
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &col[0], 4);
                    mesh.addColor(col);
                }
            }

            else if (attrib.first.compare("NORMAL") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec3 nor;
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &nor[0], 3);
                    mesh.addNormal(nor);
                }
            }

            else if (attrib.first.compare("TEXCOORD_0") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec2 uv;
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &uv[0], 2);
                    mesh.addTexCoord(uv);
                }
            }

            else if (attrib.first.compare("TANGENT") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec4 tan;
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &tan[0], 4);
                    mesh.addTangent(tan);
                }
            }

            else {
                std::cout << " " << std::endl;
                std::cout << "Undeclared attribute: " << attrib.first << std::endl;
                std::cout << "  type        :" << accessor.type << std::endl;
                std::cout << "  component   :" << accessor.componentType << std::endl;
                std::cout << "  normalize   :" << accessor.normalized << std::endl;
                std::cout << "  bufferView  :" << accessor.bufferView << std::endl;
                std::cout << "  byteOffset  :" << accessor.byteOffset << std::endl;
                std::cout << "  count       :" << accessor.count << std::endl;
                std::cout << "  byteStride  :" << byteStride << std::endl;
                std::cout << " "<< std::endl;
            }
        }

        if (_verbose) {
            std::cout << "    vertices = " << mesh.getVertices().size() << std::endl;
            std::cout << "    colors   = " << mesh.getColors().size() << std::endl;
            std::cout << "    normals  = " << mesh.getNormals().size() << std::endl;
            std::cout << "    uvs      = " << mesh.getTexCoords().size() << std::endl;
            std::cout << "    indices  = " << mesh.getIndices().size() << std::endl;

            if (mesh.getDrawMode() == GL_TRIANGLES) {
                std::cout << "    triang.  = " << mesh.getIndices().size()/3 << std::endl;
            }
            else if (mesh.getDrawMode() == GL_LINES ) {
                std::cout << "    lines    = " << mesh.getIndices().size()/2 << std::endl;
            }
        }

        if ( !mesh.hasNormals() )
            if ( mesh.computeNormals() )
                if ( _verbose )
                    std::cout << "    . Compute normals" << std::endl;

        if ( mesh.computeTangents() )
            if ( _verbose )
                std::cout << "    . Compute tangents" << std::endl;

        Material mat = extractMaterial( _model.materials[primitive.material] );

        _models.push_back( new Model(_mesh.name, mesh, mat) );
    }
};

// bind models
void extractNodes(tinygltf::Model& _model, tinygltf::Node& _node, Models& _models, bool _verbose) {
    std::cout << "Node " << _node.name << std::endl;
    extractMesh(_model, _model.meshes[ _node.mesh ], _models, _verbose);
    
    for (size_t i = 0; i < _node.children.size(); i++) {
        extractNodes(_model, _model.nodes[ _node.children[i] ], _models, _verbose);
    }
};

bool loadGLTF(Uniforms& _uniforms, WatchFileList& _files, Materials& _materials, Models& _models, int _index, bool _verbose) {
    tinygltf::Model model;
    std::string filename = _files[_index].path;

    if (! loadModel(model, filename)) {
        std::cout << "Failed to load .glTF : " << filename << std::endl;
        return false;
    }
    std::cout << "Load Model" << std::endl;

    
    const tinygltf::Scene &scene = model.scenes[model.defaultScene];
    std::cout << " tinygltf::Scene &scene" << std::endl;
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        extractNodes(model, model.nodes[scene.nodes[i]], _models, _verbose);
    }

    return true;
}
