#include "obj.h"

#include <iostream>
#include <fstream>
#include <string>

#include "../tools/text.h"
#include "ada/string.h"

#include "ada/geom/ops.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

void addModel (std::vector<ada::Model*>& _models, const std::string& _name, ada::Mesh& _mesh, ada::Material& _mat, bool _verbose) {
    if (_verbose) {
        std::cout << "    vertices = " << _mesh.getVertices().size() << std::endl;
        std::cout << "    colors   = " << _mesh.getColors().size() << std::endl;
        std::cout << "    normals  = " << _mesh.getNormals().size() << std::endl;
        std::cout << "    uvs      = " << _mesh.getTexCoords().size() << std::endl;
        std::cout << "    indices  = " << _mesh.getIndices().size() << std::endl;

        if (_mesh.getDrawMode() == GL_TRIANGLES) {
            std::cout << "    triang.  = " << _mesh.getIndices().size()/3 << std::endl;
        }
        else if (_mesh.getDrawMode() == GL_LINES ) {
            std::cout << "    lines    = " << _mesh.getIndices().size()/2 << std::endl;
        }
    }

    if ( !_mesh.haveNormals() )
        if ( _mesh.computeNormals() )
            if ( _verbose )
                std::cout << "    . Compute normals" << std::endl;

    if ( _mesh.computeTangents() )
        if ( _verbose )
            std::cout << "    . Compute tangents" << std::endl;

    _models.push_back( new ada::Model(_name, _mesh, _mat) );
}

glm::vec3 getVertex(const tinyobj::attrib_t& _attrib, int _index) {
    return glm::vec3(   _attrib.vertices[3 * _index + 0],
                        _attrib.vertices[3 * _index + 1],
                        _attrib.vertices[3 * _index + 2]);
}

glm::vec4 getColor(const tinyobj::attrib_t& _attrib, int _index) {
    return glm::vec4(   _attrib.colors[3 * _index + 0],
                        _attrib.colors[3 * _index + 1],
                        _attrib.colors[3 * _index + 2],
                        1.0);
}

glm::vec3 getNormal(const tinyobj::attrib_t& _attrib, int _index) {
    return glm::vec3(   _attrib.normals[3 * _index + 0],
                        _attrib.normals[3 * _index + 1],
                        _attrib.normals[3 * _index + 2]);
}

// Check if `mesh_t` contains smoothing group id.
bool hasSmoothingGroup(const tinyobj::shape_t& shape) {
    for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++)
        if (shape.mesh.smoothing_group_ids[i] > 0)
            return true;
        
    return false;
}

void computeSmoothingNormals(const tinyobj::attrib_t& _attrib, const tinyobj::shape_t& _shape, std::map<int, glm::vec3>& smoothVertexNormals) {
    smoothVertexNormals.clear();

    std::map<int, glm::vec3>::iterator iter;

    for (size_t f = 0; f < _shape.mesh.indices.size() / 3; f++) {
        // Get the three indexes of the face (all faces are triangular)
        tinyobj::index_t idx0 = _shape.mesh.indices[3 * f + 0];
        tinyobj::index_t idx1 = _shape.mesh.indices[3 * f + 1];
        tinyobj::index_t idx2 = _shape.mesh.indices[3 * f + 2];

        // Get the three vertex indexes and coordinates
        int vi[3];      // indexes
        vi[0] = idx0.vertex_index;
        vi[1] = idx1.vertex_index;
        vi[2] = idx2.vertex_index;

        glm::vec3 v[3];  // coordinates
        for (size_t i = 0; i < 3; i++)
            v[i] = getVertex(_attrib, vi[i]);

        // Compute the normal of the face
        glm::vec3 normal;
        ada::calcNormal(v[0], v[1], v[2], normal);

        // Add the normal to the three vertexes
        for (size_t i = 0; i < 3; ++i) {
            iter = smoothVertexNormals.find(vi[i]);
            // add
            if (iter != smoothVertexNormals.end())
                iter->second += normal;
            else
                smoothVertexNormals[vi[i]] = normal;
        }
    }  // f

    // Normalize the normals, that is, make them unit vectors
    for (iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end(); iter++) {
        iter->second = glm::normalize(iter->second);
    }
}

glm::vec2 getTexCoords(const tinyobj::attrib_t& _attrib, int _index) {
    return glm::vec2(   _attrib.texcoords[2 * _index], 
                        1.0f - _attrib.texcoords[2 * _index + 1]);
}

ada::Material InitMaterial (const tinyobj::material_t& _material, Uniforms& _uniforms, WatchFileList& _files, const std::string& _folder ) {
    ada::Material mat;
    mat.name = ada::toLower( ada::toUnderscore( ada::purifyString( _material.name ) ) );

    mat.addDefine("MATERIAL_NAME_" + ada::toUpper(mat.name) );

    mat.addDefine("MATERIAL_BASECOLOR", glm::vec4(_material.diffuse[0], _material.diffuse[1], _material.diffuse[2], 1.0));
    if (!_material.diffuse_texname.empty()) {
        std::string name = getUniformName(_material.diffuse_texname);
        _uniforms.addTexture(name, _folder + _material.diffuse_texname, _files);
        mat.addDefine("MATERIAL_BASECOLORMAP", name);

        if (_material.diffuse_texopt.origin_offset[0] != 0.0 ||
            _material.diffuse_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_BASECOLORMAP_OFFSET", (float*)_material.diffuse_texopt.origin_offset, 3);
        }

        if (_material.diffuse_texopt.scale[0] != 1.0 ||
            _material.diffuse_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_BASECOLORMAP_SCALE", (float*)_material.diffuse_texopt.scale, 3);
        }
    }

    mat.addDefine("MATERIAL_SPECULAR", (float*)_material.specular, 3);
    if (!_material.specular_texname.empty()) {
        std::string name = getUniformName(_material.specular_texname);
        _uniforms.addTexture(name, _folder + _material.specular_texname, _files);
        mat.addDefine("MATERIAL_SPECULARMAP", name);

        if (_material.specular_texopt.origin_offset[0] != 0.0 ||
            _material.specular_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_SPECULARMAP_OFFSET", (float*)_material.specular_texopt.origin_offset, 3);
        }

        if (_material.specular_texopt.scale[0] != 1.0 ||
            _material.specular_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_SPECULARMAP_SCALE", (float*)_material.specular_texopt.scale, 3);
        }
    }

    mat.addDefine("MATERIAL_EMISSIVE", (float*)_material.emission, 3);
    if (!_material.emissive_texname.empty()) {
        std::string name = getUniformName(_material.emissive_texname);
        _uniforms.addTexture(name, _folder + _material.emissive_texname, _files);
        mat.addDefine("MATERIAL_EMISSIVEMAP", name);

        if (_material.emissive_texopt.origin_offset[0] != 0.0 ||
            _material.emissive_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_EMISSIVEMAP_OFFSET", (float*)_material.emissive_texopt.origin_offset, 3);
        }

        if (_material.emissive_texopt.scale[0] != 1.0 ||
            _material.emissive_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_EMISSIVEMAP_SCALE", (float*)_material.emissive_texopt.scale, 3);
        }
    }

    mat.addDefine("MATERIAL_ROUGHNESS", _material.roughness);
    if (!_material.roughness_texname.empty()) {
        std::string name = getUniformName(_material.roughness_texname);
        _uniforms.addTexture(name, _folder + _material.roughness_texname, _files);
        mat.addDefine("MATERIAL_ROUGHNESSMAP", name);

        if (_material.roughness_texopt.origin_offset[0] != 0.0 ||
            _material.roughness_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_ROUGHNESSMAP_OFFSET", (float*)_material.roughness_texopt.origin_offset, 3);
        }

        if (_material.roughness_texopt.scale[0] != 1.0 ||
            _material.roughness_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_ROUGHNESSMAP_SCALE", (float*)_material.roughness_texopt.scale, 3);
        }
    }

    mat.addDefine("MATERIAL_METALLIC", _material.metallic);
    if (!_material.metallic_texname.empty()) {
        std::string name = getUniformName(_material.metallic_texname);
        _uniforms.addTexture(name, _folder + _material.metallic_texname, _files);
        mat.addDefine("MATERIAL_METALLICMAP", name);

        if (_material.metallic_texopt.origin_offset[0] != 0.0 ||
            _material.metallic_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_METALLICMAP_OFFSET", (float*)_material.metallic_texopt.origin_offset, 3);
        }

        if (_material.metallic_texopt.scale[0] != 1.0 ||
            _material.metallic_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_METALLICMAP_SCALE", (float*)_material.metallic_texopt.scale, 3);
        }
    }

    if (!_material.normal_texname.empty()) {
        std::string name = getUniformName(_material.normal_texname);
        _uniforms.addTexture(name, _folder + _material.normal_texname, _files);
        mat.addDefine("MATERIAL_NORMALMAP", name);

        if (_material.normal_texopt.origin_offset[0] != 0.0 ||
            _material.normal_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_NORMALMAP_OFFSET", (float*)_material.normal_texopt.origin_offset, 3);
        }

        if (_material.normal_texopt.scale[0] != 1.0 ||
            _material.normal_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_NORMALMAP_SCALE", (float*)_material.normal_texopt.scale, 3);
        }
    }

    if (!_material.bump_texname.empty()) {
        std::string name = getUniformName(_material.bump_texname);
        _uniforms.addTexture(name, _folder + _material.bump_texname, _files);
        mat.addDefine("MATERIAL_BUMPMAP", name);
        _uniforms.addBumpTexture(name + "_normalmap", _folder + _material.bump_texname, _files);
        mat.addDefine("MATERIAL_BUMPMAP_NORMALMAP", name + "_normalmap");

        if (_material.bump_texopt.origin_offset[0] != 0.0 ||
            _material.bump_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_BUMPMAP_OFFSET", (float*)_material.bump_texopt.origin_offset, 3);
            
        }

        if (_material.bump_texopt.scale[0] != 1.0 ||
            _material.bump_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_BUMPMAP_SCALE", (float*)_material.bump_texopt.scale, 3);
        }
    }

    if (!_material.displacement_texname.empty()) {
        std::string name = getUniformName(_material.displacement_texname);
        _uniforms.addTexture(name, _folder + _material.displacement_texname, _files);
        mat.addDefine("MATERIAL_DISPLACEMENTMAP", name);

        if (_material.displacement_texopt.origin_offset[0] != 0.0 ||
            _material.displacement_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_DISPLACEMENTMAP_OFFSET", (float*)_material.displacement_texopt.origin_offset, 3);
        }

        if (_material.displacement_texopt.scale[0] != 1.0 ||
            _material.displacement_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_DISPLACEMENTMAP_SCALE", (float*)_material.displacement_texopt.scale, 3);
        }
    }

    if (!_material.alpha_texname.empty()) {
        std::string name = getUniformName(_material.alpha_texname);
        _uniforms.addTexture(name, _folder + _material.alpha_texname, _files);
        mat.addDefine("MATERIAL_ALPHAMAP", name);

        if (_material.alpha_texopt.origin_offset[0] != 0.0 ||
            _material.alpha_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_ALPHAMAP_OFFSET", (float*)_material.alpha_texopt.origin_offset, 3);
        }

        if (_material.alpha_texopt.scale[0] != 1.0 ||
            _material.alpha_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_ALPHAMAP_SCALE", (float*)_material.alpha_texopt.scale, 3);
        }
    }
    
    // EXTRA 
     mat.addDefine("MATERIAL_SHEEN", _material.sheen);
    if (!_material.sheen_texname.empty()) {
        std::string name = getUniformName(_material.sheen_texname);
        _uniforms.addTexture(name, _folder + _material.sheen_texname, _files);
        mat.addDefine("MATERIAL_SHEENMAP", name);

        if (_material.sheen_texopt.origin_offset[0] != 0.0 ||
            _material.sheen_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_SHEENMAP_OFFSET", (float*)_material.sheen_texopt.origin_offset, 3);
        }

        if (_material.sheen_texopt.scale[0] != 1.0 ||
            _material.sheen_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_SHEENMAP_SCALE", (float*)_material.sheen_texopt.scale, 3);
        }
    }

    mat.addDefine("MATERIAL_SHININESS", _material.shininess);

    mat.addDefine("MATERIAL_ILLUM", _material.illum);

    if (_material.anisotropy > 0)
        mat.addDefine("MATERIAL_ANISOTROPY", _material.anisotropy);

    if (_material.anisotropy_rotation > 0)
        mat.addDefine("MATERIAL_ANISOTROPY_ROTATION", _material.anisotropy_rotation);

    if (_material.clearcoat_roughness > 0)
        mat.addDefine("MATERIAL_CLEARCOAT_ROUGHNESS", _material.clearcoat_roughness);

    if (_material.clearcoat_thickness > 0)
        mat.addDefine("MATERIAL_CLEARCOAT_THICKNESS", _material.clearcoat_thickness);

    mat.addDefine("MATERIAL_IOR", _material.ior);

    // mat.addDefine("MATERIAL_AMBIENT", (float*)_material.ambient, 3);
    // if (!_material.ambient_texname.empty()) {
    //     std::string name = getUniformName(_material.ambient_texname);
    //     _uniforms.addTexture(name, _folder + _material.ambient_texname, _files);
    //     mat.addDefine("MATERIAL_AMBIENTMAP", name);
    // }

    mat.addDefine("MATERIAL_DISSOLVE", _material.dissolve);
    mat.addDefine("MATERIAL_TRANSMITTANCE", (float*)_material.transmittance, 3);
    if (!_material.reflection_texname.empty()) {
        std::string name = getUniformName(_material.reflection_texname);
        _uniforms.addTexture(name, _folder + _material.reflection_texname, _files);
        mat.addDefine("MATERIAL_REFLECTIONMAP", name);

        if (_material.reflection_texopt.origin_offset[0] != 0.0 ||
            _material.reflection_texopt.origin_offset[1] != 0.0 ) {
            mat.addDefine("MATERIAL_REFLECTIONMAP_OFFSET", (float*)_material.reflection_texopt.origin_offset, 3);
        }

        if (_material.reflection_texopt.scale[0] != 1.0 ||
            _material.reflection_texopt.scale[1] != 1.0 ) {
            mat.addDefine("MATERIAL_REFLECTIONMAP_SCALE", (float*)_material.reflection_texopt.scale, 3);
        }
    }

    // mat.specular_highlightMap = _material.specular_highlight_texname;

    return mat;
}

bool loadOBJ(Uniforms& _uniforms, WatchFileList& _files, ada::Materials& _materials, ada::Models& _models, int _index, bool _verbose) {
    std::string filename = _files[_index].path;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    std::string base_dir = ada::getBaseDir(filename.c_str());
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), base_dir.c_str());

    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }
    if (!ret) {
        std::cerr << "Failed to load " << filename.c_str() << std::endl;
        return false;
    }

    // Append `default` material
    // materials.push_back(tinyobj::material_t());

    if (_verbose) {
        std::cerr << "Loading " << filename.c_str() << std::endl;
        printf("    Total vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
        printf("    Total colors    = %d\n", (int)(attrib.colors.size()) / 3);
        printf("    Total normals   = %d\n", (int)(attrib.normals.size()) / 3);
        printf("    Total texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
        printf("    Total materials = %d\n", (int)materials.size());
        printf("    Total shapes    = %d\n", (int)shapes.size());

        std::cerr << "Shapes: " << std::endl;
    }

    for (size_t m = 0; m < materials.size(); m++) {
        if (_materials.find( materials[m].name ) == _materials.end()) {
            if (_verbose)
                std::cout << "Add Material " << materials[m].name << std::endl;

            ada::Material mat = InitMaterial( materials[m], _uniforms, _files, base_dir );
            _materials[ materials[m].name ] = mat;
        }
    }

    for (size_t s = 0; s < shapes.size(); s++) {

        std::string name = shapes[s].name;
        if (name.empty())
            name = ada::toString(s);

        if (_verbose)
            std::cerr << name << std::endl;

        // Check for smoothing group and compute smoothing normals
        std::map<int, glm::vec3> smoothVertexNormals;
        if (hasSmoothingGroup(shapes[s]) > 0) {
            if (_verbose)
                std::cout << "    . Compute smoothingNormal" << std::endl;
            computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);
        }

        ada::Mesh mesh;
        mesh.setDrawMode(ada::TRIANGLES);

        ada::Material mat;
        std::map<int, tinyobj::index_t> unique_indices;
        std::map<int, tinyobj::index_t>::iterator iter;
        
        int mi = -1;
        int mCounter = 0;
        INDEX_TYPE iCounter = 0;
        for (size_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
            int f = (int)floor(i/3);

            tinyobj::index_t index = shapes[s].mesh.indices[i];
            int vi = index.vertex_index;
            int ni = index.normal_index;
            int ti = index.texcoord_index;

            // Associate w material
            if (shapes[s].mesh.material_ids.size() > 0) {
                int material_index = shapes[s].mesh.material_ids[f];
                if (mi != material_index) {

                    // If there is a switch of material start a new mesh
                    if (mi != -1 && mesh.getVertices().size() > 0) {

                        // std::cout << "Adding model " << name  << "_" << ada::toString(mCounter, 3, '0') << " w new material " << mat.name << std::endl;
                        
                        // Add the model to the stack 
                        addModel(_models, name + "_"+ ada::toString(mCounter,3,'0'), mesh, mat, _verbose);
                        mCounter++;

                        // Restart the mesh
                        iCounter = 0;
                        mesh.clear();
                        unique_indices.clear();
                    }

                    // assign the current material
                    mi = material_index;
                    mat = _materials[ materials[material_index].name ];
                }
            }

            bool reuse = false;
            iter = unique_indices.find(vi);

                        // if already exist 
            if (iter != unique_indices.end())
                // and have the same attributes
                if ((iter->second.normal_index == ni) &&
                    (iter->second.texcoord_index == ti) )
                    reuse = true;
            
            // Re use the vertex
            if (reuse)
                mesh.addIndex( (INDEX_TYPE)iter->second.vertex_index );
            // Other wise create a new one
            else {
                unique_indices[vi].vertex_index = (int)iCounter;
                unique_indices[vi].normal_index = ni;
                unique_indices[vi].texcoord_index = ti;
                
                mesh.addVertex( getVertex(attrib, vi) );
                mesh.addColor( getColor(attrib, vi) );

                // If there is normals add them
                if (attrib.normals.size() > 0)
                    mesh.addNormal( getNormal(attrib, ni) );

                else if (smoothVertexNormals.size() > 0)
                    if ( smoothVertexNormals.find(vi) != smoothVertexNormals.end() )
                        mesh.addNormal( smoothVertexNormals.at(vi) );

                // If there is texcoords add them
                if (attrib.texcoords.size() > 0)
                    mesh.addTexCoord( getTexCoords(attrib, ti) );

                mesh.addIndex( iCounter++ );
            }
        }

        std::string meshName = name;
        if (mCounter > 0)
            meshName = name + "_" + ada::toString(mCounter, 3, '0');

        // std::cout << "Adding model " << meshName << " w material " << mat.name << std::endl;
        addModel(_models, meshName, mesh, mat, _verbose);
    }

    return true;
}