
#include "ply.h"

#include <iostream>
#include <fstream>
#include <string>

#include "ada/geom/ops.h"
#include "ada/string.h"

#if defined(SUPPORT_PLY_BINARY)

#define TINYPLY_IMPLEMENTATION
#include "tinyply/tinyply.h"


bool loadPLY(WatchFileList& _files, int _index, ada::Scene& _scene, bool _verbose) {
    std::string filename = _files[_index].path;

    std::string name = filename.substr(0, filename.size()-4);

    ada::Material default_material;
    std::vector<glm::vec4> mesh_colors;
    std::vector<glm::vec3> mesh_vertices;
    std::vector<glm::vec3> mesh_normals;
    std::vector<glm::vec2> mesh_texcoords;
    std::vector<INDEX_TYPE> face_indices;
    std::vector<INDEX_TYPE> edge_indices;

    std::unique_ptr<std::istream> file_stream;
    try
    {
        file_stream.reset(new std::ifstream(filename, std::ios::binary));

        if (!file_stream || file_stream->fail()) throw std::runtime_error("file_stream failed to open " + filename);

        std::vector<std::string> colors_names, texcoords_names, faces_name, edges_name;

        tinyply::PlyFile file;
        file.parse_header(*file_stream);

        // std::cout << "\t[ply_header] Type: " << (file.is_binary_file() ? "binary" : "ascii") << std::endl;
        // for (const auto & c : file.get_comments()) std::cout << "\t[ply_header] Comment: " << c << std::endl;
        // for (const auto & c : file.get_info()) std::cout << "\t[ply_header] Info: " << c << std::endl;

        std::vector<std::string> comments = file.get_comments();
        for (size_t i = 0; i < comments.size(); i++) {
        //     std::vector<std::string> parts = split(comments[i], ' ', true);
        //     if (parts[0] == "TextureFile") {
        //         Material mat("default");
        //         mat.set("diffuse", parts[1]);
        //         _mesh.addMaterial(mat);
        //     }
        }

        for (const auto & e : file.get_elements()) {
            for (const auto & p : e.properties) {
                // std::cout << "\t[ply_header] \tproperty: " << p.name << " (type=" << tinyply::PropertyTable[p.propertyType].str << ")";
                // if (p.isList) std::cout << " (list_type=" << tinyply::PropertyTable[p.listType].str << ")";
                // std::cout << std::endl;
                if (p.name == "red" || p.name == "green" || p.name == "blue" || p.name == "alpha" ||
                    p.name == "r" || p.name == "g" || p.name == "b" || p.name == "a")
                    colors_names.push_back(p.name);
                else if (   p.name == "u" || p.name == "v" || 
                            p.name == "s" || p.name == "t" ||
                        p.name == "texture_u" || p.name == "texture_v")
                    texcoords_names.push_back(p.name);
                else if (p.name == "vertex_indices" || p.name == "vertex_index")
                    faces_name.push_back(p.name);
                else if (p.name == "vertex1" || p.name == "vertex2")
                    edges_name.push_back(p.name);
            }
        }


        // Because most people have their own mesh types, tinyply treats parsed data as structured/typed byte buffers. 
        // See examples below on how to marry your own application-specific data structures with this one. 
        std::shared_ptr<PlyData> vertices, normals, colors, texcoords, faces, edges;

        // The header information can be used to programmatically extract properties on elements
        // known to exist in the header prior to reading the data. For brevity of this sample, properties 
        // like vertex position are hard-coded: 
        try { vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }); }
        catch (const std::exception & e) { }

        try { normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }); }
        catch (const std::exception & e) { }

        if (!colors_names.empty()) {
            try { colors = file.request_properties_from_element("vertex", colors_names); }
            catch (const std::exception & e) { }
        }
        
        if (!texcoords_names.empty()) {
            try { texcoords = file.request_properties_from_element("vertex", texcoords_names); }
            catch (const std::exception & ) { }
        }
        
        // Providing a list size hint (the last argument) is a 2x performance improvement. If you have 
        // arbitrary ply files, it is best to leave this 0. 
        if (faces_name.size() == 1) {
            try { faces = file.request_properties_from_element("face", faces_name, 3); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }
        }

        if (edges_name.size() > 0) {
            try { edges = file.request_properties_from_element("edge", edges_name, 2); }
            catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }
        }

        // // Tristrips must always be read with a 0 list size hint (unless you know exactly how many elements
        // // are specifically in the file, which is unlikely); 
        // try { tripstrip = file.request_properties_from_element("tristrips", { "vertex_indices" }, 0); }
        // catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }

        file.read(*file_stream);
        
        if (vertices) {
            const size_t numVerticesBytes = vertices->buffer.size_bytes();

            mesh_vertices.resize(vertices->count);
            std::memcpy(mesh_vertices.data(), vertices->buffer.get(), numVerticesBytes);

            if (colors) {
                size_t numChannels = colors_names.size();
                size_t numColors = colors->count;

                float* fClr   = (float *) colors->buffer.get();
                uint8_t* bClr = (uint8_t *) colors->buffer.get();

                for (size_t i = 0; i < numColors; i++) {
                    if (colors->t == tinyply::Type::FLOAT32) {
                        const float r = *fClr++;
                        const float g = *fClr++;
                        const float b = *fClr++;
                        float a = 1.0f;
                        if (numChannels == 4)
                            a = *fClr++;

                        mesh_colors.push_back( glm::vec4(r, g, b, a) );
                    }
                    if (colors->t == tinyply::Type::UINT8) {    
                        const int r = *bClr++;
                        const int g = *bClr++;
                        const int b = *bClr++;   
                        int a = 255;
                        if (numChannels == 4)
                            a = *bClr++;

                        mesh_colors.push_back( glm::vec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f) ); 
                    }
                }
            }

            if (normals) {
                if (normals->t == tinyply::Type::FLOAT32) {
                    const size_t numNormalsBytes = normals->buffer.size_bytes();
                    mesh_normals.resize(normals->count);
                    std::memcpy(mesh_normals.data(), normals->buffer.get(), numNormalsBytes);
                }
            }

            if (texcoords) {
                if (texcoords->t == tinyply::Type::FLOAT32) {
                    const size_t numTexcoordsBytes = texcoords->buffer.size_bytes();
                    mesh_texcoords.resize(texcoords->count);
                    std::memcpy(mesh_texcoords.data(), texcoords->buffer.get(), numTexcoordsBytes);
                }
            }

            if (faces) {
                const INDEX_TYPE* array1D = reinterpret_cast<INDEX_TYPE*>(faces->buffer.get());
                size_t n = faces->count * 3;
                face_indices.insert(face_indices.end(), array1D, array1D + n);
            }

            if (edges) {
                const INDEX_TYPE* array1D = reinterpret_cast<INDEX_TYPE*>(edges->buffer.get());
                size_t n = faces->count * 2;
                edge_indices.insert(edge_indices.end(), array1D, array1D + n);
            }
        }
    }
    catch (const std::exception & e)
    {
        std::cerr << "Caught tinyply exception: " << e.what() << std::endl;
    }

    // return true;

    //  Succed loading the PLY data
    //  (proceed replacing the data on mesh)
    //
    _materials[default_material.name] = default_material;

    if (face_indices.size() > 0) {
        ada::Mesh mesh;
        mesh.addColors(mesh_colors);
        mesh.addVertices(mesh_vertices);
        mesh.addTexCoords(mesh_texcoords);
        mesh.addIndices( face_indices );

        if ( mesh_normals.size() > 0 )
            mesh.addNormals( mesh_normals );
        else
            mesh.computeNormals();

        mesh.computeTangents();

        _scene.setModel( name, new ada::Model(name, mesh, default_material) );
    }

    if (edge_indices.size() > 0) {
        ada::Mesh mesh;
        mesh.setDrawMode( GL_LINES );
        mesh.addColors(mesh_colors);
        mesh.addVertices(mesh_vertices);
        mesh.addTexCoords(mesh_texcoords);
        if ( mesh_normals.size() > 0 )
            mesh.addNormals( mesh_normals );

        mesh.addIndices( edge_indices );

        _scene.setModel( name + "_edges", new ada::Model(name + "_edges", mesh, default_material) );

    }

    if ( face_indices.size() == 0 && edge_indices.size() == 0) {
        ada::Mesh mesh;
        mesh.setDrawMode( GL_POINTS );
        mesh.addColors(mesh_colors);
        mesh.addVertices(mesh_vertices);
        mesh.addTexCoords(mesh_texcoords);
        mesh.addNormals( mesh_normals );

        _scene.setModel( name + "_points", new ada::Model(name + "_points", mesh, default_material) );
    }
    
    return false;
}

#else

bool loadPLY(const std::string& _filename, ada::Scene& _scene, bool _verbose) {
    std::fstream is(_filename.c_str(), std::ios::in);
    if (is.is_open()) {

        ada::Material default_material;
        ada::Mesh mesh;

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
                orderVertices = glm::max(orderIndices, 0)+1;
                vertices.resize(ada::toInt(line.substr(15)));
                continue;
            }

            if ((state==Header || state==VertexDef) && line.find("element face")==0) {
                state = FaceDef;
                orderIndices = glm::max(orderVertices, 0)+1;
                indices.resize(ada::toInt(line.substr(13))*3);
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
                    error =  "data has color coordiantes but not correct number of components. Found " + ada::toString(colorCompsFound) + " expecting 3 or 4";
                    goto clean;
                }
                if (normals.size() && normalsCoordsFound!=3) {
                    error = "data has normal coordiantes but not correct number of components. Found " + ada::toString(normalsCoordsFound) + " expecting 3";
                    goto clean;
                }
                if (!vertices.size()) {
                    std::cout << "ERROR glMesh, load(): mesh loaded from \"" << _filename << "\" has no vertices" << std::endl;
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
                if ((uint32_t)currentVertex==vertices.size()) {
                    if (orderVertices<orderIndices) {
                        state = Faces;
                    }
                    else{
                        state = Vertices;
                    }
                }
                continue;
            }

            if (state==Faces && indices.size() > 0) {
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
                if ((uint32_t)currentFace==indices.size()/3) {
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
            mesh.setDrawMode( ada::POINTS );
        }
        
        if ( normals.size() > 0 ) {
            mesh.addNormals( normals );
        }
        else {
            mesh.computeNormals();
        }

        mesh.computeTangents();

        _scene.materials[default_material.name] = default_material;

        if (mesh.getDrawMode() == GL_POINTS)
            name = "points";
        else if (mesh.getDrawMode() == GL_LINES)
            name = "lines";
        else
            name = "mesh";
        _scene.setModel(name, new ada::Model(name, mesh, default_material) );

        return true;

    clean:
        std::cout << "ERROR glMesh, load(): " << lineNum << ":" << error << std::endl;
        std::cout << "ERROR glMesh, load(): \"" << line << "\"" << std::endl;

    }

    is.close();
    std::cout << "ERROR glMesh, can not load  " << _filename << std::endl;

    return false;
}

#endif
