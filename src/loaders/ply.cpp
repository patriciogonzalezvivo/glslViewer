
#include "ply.h"

#include <iostream>
#include <fstream>
#include <string>

#include "../tools/geom.h"
#include "../tools/text.h"

bool loadPLY(Uniforms& _uniforms, WatchFileList &filenames, std::map<std::string,Material>& _materials, std::vector<Model*>& _models, int _index, bool _verbose) {
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

        _materials[default_material.name] = default_material;

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
