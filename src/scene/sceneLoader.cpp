
#include "sceneLoader.h"

#include <iostream>
#include <fstream>
#include <string>

#include "../tools/geom.h"
#include "../tools/text.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

bool loadPLY(Uniforms& _uniforms, WatchFileList &filenames, std::vector<Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose) {
    std::string filename = filenames[_index].path;
    std::fstream is(filename.c_str(), std::ios::in);
    if (is.is_open()) {

        Material default_material;
        Mesh mesh;

        std::string line;
        std::string error;

        int orderVertices=-1;
        int orderIndices=-1;

        int vertexCoordsFound=0;
        int colorCompsFound=0;
        int texCoordsFound=0;
        int normalsCoordsFound=0;

        int currentVertex = 0;
        int currentFace = 0;

        bool floatColor = false;

        enum State{
            Header,
            VertexDef,
            FaceDef,
            Vertices,
            Normals,
            Faces
        };

        State state = Header;

        int lineNum = 0;

        std::string name;

        std::vector<glm::vec4> colors;
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoord;
        std::vector<INDEX_TYPE> indices;

        std::getline(is,line);
        lineNum++;
        if (line!="ply") {
            error = "wrong format, expecting 'ply'";
            goto clean;
        }

        std::getline(is,line);
        lineNum++;
        if (line!="format ascii 1.0") {
            error = "wrong format, expecting 'format ascii 1.0'";
            goto clean;
        }

        while(std::getline(is,line)) {
            lineNum++;
            if (line.find("comment")==0) {
                continue;
            }

            if ((state==Header || state==FaceDef) && line.find("element vertex")==0) {
                state = VertexDef;
                orderVertices = MAX(orderIndices, 0)+1;
                vertices.resize(toInt(line.substr(15)));
                continue;
            }

            if ((state==Header || state==VertexDef) && line.find("element face")==0) {
                state = FaceDef;
                orderIndices = MAX(orderVertices, 0)+1;
                indices.resize(toInt(line.substr(13))*3);
                continue;
            }

            if (state==VertexDef && (line.find("property float x")==0 || line.find("property float y")==0 || line.find("property float z")==0)) {
                vertexCoordsFound++;
                continue;
            }

            if (state==VertexDef && (line.find("property float nx")==0 || line.find("property float ny")==0 || line.find("property float nz")==0)) {
                normalsCoordsFound++;
                if (normalsCoordsFound==3) normals.resize(vertices.size());
                continue;
            }

            if (state==VertexDef && (line.find("property float r")==0 || line.find("property float g")==0 || line.find("property float b")==0 || line.find("property float a")==0)) {
                colorCompsFound++;
                colors.resize(vertices.size());
                floatColor = true;
                continue;
            }
            else if (state==VertexDef && (line.find("property uchar red")==0 || line.find("property uchar green")==0 || line.find("property uchar blue")==0 || line.find("property uchar alpha")==0)) {
                colorCompsFound++;
                colors.resize(vertices.size());
                floatColor = false;
                continue;
            }

            if (state==VertexDef && (line.find("property float u")==0 || line.find("property float v")==0)) {
                texCoordsFound++;
                texcoord.resize(vertices.size());
                continue;
            }
            else if (state==VertexDef && (line.find("property float texture_u")==0 || line.find("property float texture_v")==0)) {
                texCoordsFound++;
                texcoord.resize(vertices.size());
                continue;
            }

            if (state==FaceDef && line.find("property list")!=0 && line!="end_header") {
                error = "wrong face definition";
                goto clean;
            }

            if (line=="end_header") {
                if (colors.size() && colorCompsFound!=3 && colorCompsFound!=4) {
                    error =  "data has color coordiantes but not correct number of components. Found " + toString(colorCompsFound) + " expecting 3 or 4";
                    goto clean;
                }
                if (normals.size() && normalsCoordsFound!=3) {
                    error = "data has normal coordiantes but not correct number of components. Found " + toString(normalsCoordsFound) + " expecting 3";
                    goto clean;
                }
                if (!vertices.size()) {
                    std::cout << "ERROR glMesh, load(): mesh loaded from \"" << filename << "\" has no vertices" << std::endl;
                }
                if (orderVertices==-1) orderVertices=9999;
                if (orderIndices==-1) orderIndices=9999;

                if (orderVertices < orderIndices) {
                    state = Vertices;
                }
                else {
                    state = Faces;
                }
                continue;
            }

            if (state==Vertices) {
                std::stringstream sline;
                sline.str(line);
                glm::vec3 v;
                sline >> v.x;
                sline >> v.y;
                if ( vertexCoordsFound > 2) sline >> v.z;
                vertices[currentVertex] = v;

                if (normalsCoordsFound > 0) {
                    glm::vec3 n;
                    sline >> n.x;
                    sline >> n.y;
                    sline >> n.z;
                    normals[currentVertex] = n;
                }

                if (colorCompsFound > 0) {
                    if (floatColor) {
                        glm::vec4 c;
                        sline >> c.r;
                        sline >> c.g;
                        sline >> c.b;
                        if (colorCompsFound > 3) sline >> c.a;
                        colors[currentVertex] = c;
                    }
                    else {
                        float r, g, b, a = 255;
                        sline >> r;
                        sline >> g;
                        sline >> b;
                        if (colorCompsFound > 3) sline >> a;
                        colors[currentVertex] = glm::vec4(r/255.0, g/255.0, b/255.0, a/255.0);
                    }
                }

                if (texCoordsFound>0) {
                    glm::vec2 uv;
                    sline >> uv.x;
                    sline >> uv.y;
                    texcoord[currentVertex] = uv;
                }

                currentVertex++;
                if ((uint)currentVertex==vertices.size()) {
                    if (orderVertices<orderIndices) {
                        state = Faces;
                    }
                    else{
                        state = Vertices;
                    }
                }
                continue;
            }

            if (state==Faces) {
                std::stringstream sline;
                sline.str(line);
                int numV;
                sline >> numV;
                if (numV!=3) {
                    error = "face not a triangle";
                    goto clean;
                }
                int i;
                sline >> i;
                indices[currentFace*3] = i;
                sline >> i;
                indices[currentFace*3+1] = i;
                sline >> i;
                indices[currentFace*3+2] = i;

                currentFace++;
                if ((uint)currentFace==indices.size()/3) {
                    if (orderVertices<orderIndices) {
                        state = Vertices;
                    }
                    else {
                        state = Faces;
                    }
                }
                continue;
            }
        }
        is.close();

        //  Succed loading the PLY data
        //  (proceed replacing the data on mesh)
        //
        mesh.addColors(colors);
        mesh.addVertices(vertices);
        mesh.addTexCoords(texcoord);

        if ( indices.size() > 0 ){
            mesh.addIndices( indices );
        }
        else {
            mesh.setDrawMode( GL_POINTS );
        }
        
        if ( normals.size() > 0 ) {
            mesh.addNormals( normals );
        }
        else {
            mesh.computeNormals();
        }

        mesh.computeTangents();

        _materials.push_back(default_material);
        if (mesh.getDrawMode() == GL_POINTS)
            name = "points";
        else if (mesh.getDrawMode() == GL_LINES)
            name = "lines";
        else
            name = "mesh";
        _models.push_back( new Model(name, mesh, default_material) );

        return true;

    clean:
        std::cout << "ERROR glMesh, load(): " << lineNum << ":" << error << std::endl;
        std::cout << "ERROR glMesh, load(): \"" << line << "\"" << std::endl;

    }

    is.close();
    std::cout << "ERROR glMesh, can not load  " << filename << std::endl;

    return false;
}

// ------------------------------------------------------------------------------------- 

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

void getNormal(const tinyobj::attrib_t& _attrib, const std::map<int, glm::vec3>& _smoothVertexNormals, const tinyobj::index_t& _index, std::vector<glm::vec3>& _normals) {
    // There is normal
    if (((int)_attrib.normals.size() > 0) &&
        (_index.normal_index >= 0) && 
        (_index.normal_index < (int)_attrib.normals.size()))
            _normals.push_back( getNormal(_attrib, _index.normal_index) );

    // There is smoothnormal
    else if ((int)_smoothVertexNormals.size() > 0)
        if ( _smoothVertexNormals.find(_index.vertex_index) != _smoothVertexNormals.end() )
            _normals.push_back( _smoothVertexNormals.at(_index.vertex_index) );
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
        calcNormal(v[0], v[1], v[2], normal);

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

void getTexCoords(const tinyobj::attrib_t& _attrib, const tinyobj::index_t& _index, std::vector<glm::vec2>& _texcoords) {
    if (_attrib.texcoords.size() > 0)
        if ((_index.texcoord_index >= 0) &&
            (_attrib.texcoords.size() > size_t(2 * _index.texcoord_index + 1)))
                _texcoords.push_back( getTexCoords(_attrib, _index.texcoord_index) );
}

glm::vec3 getAmbient(const tinyobj::material_t& _material) {
    glm::vec3 c = glm::vec4(1.0);
    for (int i = 0; i < 3; i++)
        c[i] = (float)_material.ambient[i];
    return c;
}

glm::vec3 getDiffuse(const tinyobj::material_t& _material) {
    glm::vec3 c = glm::vec4(1.0);
    for (int i = 0; i < 3; i++)
        c[i] = (float)_material.diffuse[i];
    return c;
}

glm::vec3 getSpecular(const tinyobj::material_t& _material) {
    glm::vec4 c = glm::vec4(1.0);
    for (int i = 0; i < 3; i++)
        c[i] = (float)_material.specular[i];
    return c;
}

glm::vec3 getTransmittance(const tinyobj::material_t& _material) {
    glm::vec3 c = glm::vec4(1.0);
    for (int i = 0; i < 3; i++)
        c[i] = (float)_material.transmittance[i];
    return c;
}

glm::vec3 getEmission(const tinyobj::material_t& _material) {
    glm::vec3 c = glm::vec4(1.0);
    for (int i = 0; i < 3; i++)
        c[i] = (float)_material.emission[i];
    return c;
}

Material getMaterial (const tinyobj::material_t& _material) {
    Material mat;
    mat.name = toLower( toUnderscore( purifyString( _material.name ) ) );
    mat.ambient = getAmbient(_material);
    mat.diffuse = getDiffuse(_material);
    mat.specular = getSpecular(_material);
    mat.transmittance = getTransmittance(_material);
    mat.emission = getEmission(_material);
    mat.shininess = _material.shininess;
    mat.ior = _material.ior;
    mat.dissolve = _material.dissolve;
    mat.illum = _material.illum;
    mat.ambientMap = _material.ambient_texname;
    mat.diffuseMap = _material.diffuse_texname;
    mat.specularMap = _material.specular_texname;
    mat.specular_highlightMap = _material.specular_highlight_texname;
    mat.bumpMap = _material.bump_texname;
    mat.displacementMap = _material.displacement_texname;
    mat.alphaMap = _material.alpha_texname;
    mat.reflectionMap = _material.reflection_texname;
    mat.roughness = _material.roughness;
    mat.metallic = _material.metallic;
    mat.sheen = _material.sheen;
    mat.clearcoat_thickness = _material.clearcoat_roughness;
    mat.clearcoat_roughness = _material.clearcoat_roughness;
    mat.anisotropy = _material.anisotropy;
    mat.anisotropy_rotation = _material.anisotropy_rotation;
    mat.roughnessMap = _material.roughness_texname;
    mat.metallicMap = _material.metallic_texname;
    mat.sheenMap = _material.sheen_texname;
    mat.emissiveMap = _material.emissive_texname;
    mat.normalMap = _material.normal_texname;

    return mat;
}

bool loadOBJ(Uniforms& _uniforms, WatchFileList& _files, std::vector<Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose) {
    std::string filename = _files[_index].path;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    std::string base_dir = getBaseDir(filename.c_str());
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
        std::cout << "Material " << materials[m].name << std::endl;
        Material mat = getMaterial( materials[m] );
        mat.loadTextures(_uniforms, _files, base_dir);
        _materials.push_back( mat );
    }

    for (size_t s = 0; s < shapes.size(); s++) {

        std::string name = shapes[s].name;
        if (name.empty())
            name = toString(s);

        if (_verbose)
            std::cerr << name << std::endl;

        // Check for smoothing group and compute smoothing normals
        std::map<int, glm::vec3> smoothVertexNormals;
        if (hasSmoothingGroup(shapes[s]) > 0) {
            if (_verbose)
                std::cout << "    . Compute smoothingNormal" << std::endl;
            computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);
        }

        Mesh mesh;
        mesh.setDrawMode(GL_TRIANGLES);

        Material mat;
        int mi = -1;
        std::map<int, tinyobj::index_t> unique_indices;
        std::map<int, tinyobj::index_t>::iterator iter;

        INDEX_TYPE counter = 0;
        for (size_t i = 0; i < shapes[s].mesh.indices.size(); i++) {
            tinyobj::index_t index = shapes[s].mesh.indices[i];

            int vi = index.vertex_index;
            int ni = index.normal_index;
            int ti = index.texcoord_index;

            bool reuse = false;
            iter = unique_indices.find(vi);

            // if already exist 
            if (iter != unique_indices.end())
                // and have the same attributes
                if ((iter->second.normal_index == ni) &&
                    (iter->second.texcoord_index == ti) )
                    reuse = true;

            if (shapes[s].mesh.material_ids.size() > 0) {
                mi = shapes[s].mesh.material_ids[floor(i/3)];
                mat = _materials[mi];
            }
            
            // Re use the vertex
            if (reuse)
                mesh.addIndex( (INDEX_TYPE)iter->second.vertex_index );
            // Other wise create a new one
            else {
                unique_indices[vi].vertex_index = (int)counter;
                unique_indices[vi].normal_index = ni;
                unique_indices[vi].texcoord_index = ti;
                
                

                mesh.addVertex( getVertex(attrib, vi) );

                // If the model have color use 
                if ((attrib.colors.size() > 0) && 
                    (attrib.colors.size() == attrib.vertices.size()))
                    mesh.addColor( getColor(attrib, vi) );

                // other whise try to extract them from the material
                else if ((mi >= 0) && (mi < static_cast<int>(materials.size()))) {
                    glm::vec3 c = getDiffuse(materials[mi]);
                    mesh.addColor( glm::vec4(c.r, c.g, c.b, 1.0) );
                }

                // If there is normals add them
                if (attrib.normals.size() > 0)
                    mesh.addNormal( getNormal(attrib, ni) );

                // If there is texcoords add them
                if (attrib.texcoords.size() > 0)
                    mesh.addTexCoord( getTexCoords(attrib, ti) );

                mesh.addIndex( counter++ );
            }
            
        }
        // // If model have normals or texcoords we have to parse each face individually
        // else {
        //     INDEX_TYPE index = 0;
        //     std::vector<glm::vec4> colors;
        //     std::vector<glm::vec3> normals;
        //     std::vector<glm::vec2> uvs;

        //     for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++) {
        //         tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
        //         tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
        //         tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];
        //         int current_material_id = shapes[s].mesh.material_ids[f];

        //         // Vertices
        //         mesh.addVertex( getVertex(attrib, idx0.vertex_index) );
        //         mesh.addVertex( getVertex(attrib, idx1.vertex_index) );
        //         mesh.addVertex( getVertex(attrib, idx2.vertex_index) );

        //         // Color
        //         if ((current_material_id >= 0) && (current_material_id < static_cast<int>(materials.size()))) {
        //             // Extract vertex color
        //             glm::vec4 c = getDiffuse(materials[current_material_id]);
        //             colors.push_back(c);
        //             colors.push_back(c);
        //             colors.push_back(c);
        //         }

        //         getNormal(attrib, smoothVertexNormals, idx0, normals);
        //         getNormal(attrib, smoothVertexNormals, idx1, normals);
        //         getNormal(attrib, smoothVertexNormals, idx2, normals);

        //         getTexCoords(attrib, idx0, uvs);
        //         getTexCoords(attrib, idx1, uvs);
        //         getTexCoords(attrib, idx2, uvs);

        //         mesh.addIndex(index++);
        //         mesh.addIndex(index++);
        //         mesh.addIndex(index++);
        //     }

        //     if (colors.size() == mesh.getVertices().size())
        //         mesh.addColors(colors);

        //     if (normals.size() == mesh.getVertices().size())
        //         mesh.addNormals(normals);

        //     if (uvs.size() == mesh.getVertices().size())
        //         mesh.addTexCoords(uvs);
        // }

        if (_verbose) {
            std::cout << "    vertices = " << mesh.getVertices().size() << std::endl;
            std::cout << "    colors   = " << mesh.getColors().size() << std::endl;
            std::cout << "    normals  = " << mesh.getNormals().size() << std::endl;
            std::cout << "    uvs      = " << mesh.getTexCoords().size() << std::endl;
        }

        if ( !mesh.hasNormals() )
            if ( mesh.computeNormals() )
                if ( _verbose )
                    std::cout << "    . Compute normals" << std::endl;

        if ( mesh.computeTangents() )
            if ( _verbose )
                std::cout << "    . Compute tangents" << std::endl;

        _models.push_back( new Model(name, mesh, mat) );
    }

    return true;
}