#include "mesh.h"

#include <iostream>
#include <fstream> 

#include "tools/fs.h"
#include "tools/geom.h"
#include "tools/text.h"
#include "gl/vertexLayout.h"

#include "tinyobjloader/tiny_obj_loader.h"

Mesh::Mesh():m_drawMode(GL_TRIANGLES) {

}

Mesh::Mesh(const Mesh &_mother):m_drawMode(_mother.getDrawMode()) {
    add(_mother);
}

Mesh::~Mesh() {

}

bool Mesh::load(const std::string& _file) {
    if ( haveExt(_file,"ply") || haveExt(_file,"PLY") ) {
        std::fstream is(_file.c_str(), std::ios::in);
        if (is.is_open()) {

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
                        std::cout << "ERROR glMesh, load(): mesh loaded from \"" << _file << "\" has no vertices" << std::endl;
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
            clear();
            addColors(colors);
            addVertices(vertices);
            addTexCoords(texcoord);

            if ( indices.size() > 0 ){
                addIndices( indices );
            }
            else {
                setDrawMode( GL_POINTS );
            }
            

            if ( normals.size() > 0 ) {
                addNormals( normals );
            }
            else if ( getDrawMode() == GL_TRIANGLES ) {
                computeNormals();
            }

            if ( texcoord.size() == m_vertices.size() && getDrawMode() == GL_TRIANGLES ) {
                computeTangents();
            }

            return true;

        clean:
            std::cout << "ERROR glMesh, load(): " << lineNum << ":" << error << std::endl;
            std::cout << "ERROR glMesh, load(): \"" << line << "\"" << std::endl;

        }

        is.close();
        std::cout << "ERROR glMesh, can not load  " << _file << std::endl;
        return false;
    } 
    else if ( haveExt(_file,"obj") || haveExt(_file,"OBJ") ) {
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err = tinyobj::LoadObj(shapes, materials, _file.c_str(), NULL);
        if (!err.empty()) {
            std::cerr << err << std::endl;
            return false;
        }

        for (size_t i = 0; i < shapes.size(); i++) {
            if ((shapes[i].mesh.positions.size() % 3) == 0) {
                for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
                    addVertex(glm::vec3(shapes[i].mesh.positions[3*v+0],
                                        shapes[i].mesh.positions[3*v+1],
                                        shapes[i].mesh.positions[3*v+2]));
                }
            }

            if ( (shapes[i].mesh.normals.size() % 3) == 0) {
                for (size_t v = 0; v < shapes[i].mesh.normals.size() / 3; v++) {
                    addNormal(glm::vec3(shapes[i].mesh.normals[3*v+0],
                                        shapes[i].mesh.normals[3*v+1],
                                        shapes[i].mesh.normals[3*v+2]));
                }
            }

            if ( (shapes[i].mesh.texcoords.size() % 2) == 0) {
                for (size_t v = 0; v < shapes[i].mesh.texcoords.size() / 2; v++) {
                    addTexCoord(glm::vec2(  shapes[i].mesh.texcoords[2*v+0],
                                            shapes[i].mesh.texcoords[2*v+1] ));
                }
            }

            if ( (shapes[i].mesh.indices.size() % 3) == 0) {
                for (size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++) {
                    addTriangle(shapes[i].mesh.indices[3*f+0], shapes[i].mesh.indices[3*f+1],shapes[i].mesh.indices[3*f+2]);
                }
            }
        }

        if (hasNormals() && getDrawMode() == GL_TRIANGLES) {
            computeTangents();
        }
    }
    return false;
}

bool Mesh::save(const std::string& _file, bool _useBinary) {
    if (haveExt(_file,"ply")) {
        std::ios_base::openmode binary_mode = _useBinary ? std::ios::binary : (std::ios_base::openmode)0;
        std::fstream os(_file.c_str(), std::ios::out | binary_mode);

        os << "ply" << std::endl;
        if (_useBinary) {
            os << "format binary_little_endian 1.0" << std::endl;
        }
        else {
            os << "format ascii 1.0" << std::endl;
        }

        if (getVertices().size()) {
            os << "element vertex " << getVertices().size() << std::endl;
            os << "property float x" << std::endl;
            os << "property float y" << std::endl;
            os << "property float z" << std::endl;
            if (hasColors()) {
                os << "property uchar red" << std::endl;
                os << "property uchar green" << std::endl;
                os << "property uchar blue" << std::endl;
                os << "property uchar alpha" << std::endl;
            }
            if (hasTexCoords()) {
                os << "property float u" << std::endl;
                os << "property float v" << std::endl;
            }
            if (hasNormals()) {
                os << "property float nx" << std::endl;
                os << "property float ny" << std::endl;
                os << "property float nz" << std::endl;
            }
        }

        unsigned char faceSize = 3;
        if (hasIndices()) {
            os << "element face " << getIndices().size() / faceSize << std::endl;
            os << "property list uchar int vertex_indices" << std::endl;
        } 
        else if (getDrawMode() == GL_TRIANGLES) {
            os << "element face " << getIndices().size() / faceSize << std::endl;
            os << "property list uchar int vertex_indices" << std::endl;
        }

        os << "end_header" << std::endl;

        for(uint i = 0; i < getVertices().size(); i++) {
            if (_useBinary) {
                os.write((char*) &getVertices()[i], sizeof(glm::vec3));
            } 
            else {
                os << getVertices()[i].x << " " << getVertices()[i].y << " " << getVertices()[i].z;
            }
            if (hasColors()) {
                // VCG lib / MeshLab don't support float colors, so we have to cast
                glm::vec4 c = getColors()[i] * glm::vec4(255,255,255,255);
                glm::ivec4 cur = glm::ivec4(c.r,c.g,c.b,c.a);
                if (_useBinary) {
                    os.write((char*) &cur, sizeof(glm::ivec4));
                }
                else {
                    os << " " << (int) cur.r << " " << (int) cur.g << " " << (int) cur.b << " " << (int) cur.a;
                }
            }
            if (hasTexCoords()) {
                if (_useBinary) {
                    os.write((char*) &getTexCoords()[i], sizeof(glm::vec2));
                } 
                else {
                    os << " " << getTexCoords()[i].x << " " << getTexCoords()[i].y;
                }
            }
            if (hasNormals()) {
                glm::vec3 norm = glm::normalize(getNormals()[i]);
                if (_useBinary) {
                    os.write((char*) &norm, sizeof(glm::vec3));
                }
                else {
                    os << " " << norm.x << " " << norm.y << " " << norm.z;
                }
            }
            if (!_useBinary) {
                os << std::endl;
            }
        }

        if (hasIndices()) {
            for (uint i = 0; i < getIndices().size(); i += faceSize) {
                if (_useBinary) {
                    os.write((char*) &faceSize, sizeof(unsigned char));
                    for(int j = 0; j < faceSize; j++) {
                        int curIndex = getIndices()[i + j];
                        os.write((char*) &curIndex, sizeof(int));
                    }
                }
                else {
                    os << (int) faceSize << " " << getIndices()[i] << " " << getIndices()[i+1] << " " << getIndices()[i+2] << std::endl;
                }
            }
        }
        else if (getDrawMode() == GL_TRIANGLES) {
            for(uint i = 0; i < getVertices().size(); i += faceSize) {
                uint indices[] = {i, i + 1, i + 2};
                if (_useBinary) {
                    os.write((char*) &faceSize, sizeof(unsigned char));
                    for(int j = 0; j < faceSize; j++) {
                        os.write((char*) &indices[j], sizeof(int));
                    }
                }
                else {
                    os << (int) faceSize << " " << indices[0] << " " << indices[1] << " " << indices[2] << std::endl;
                }
            }
        }

        os.close();
        return true;
    }
    return false;
}

void Mesh::setDrawMode(GLenum _drawMode) {
    m_drawMode = _drawMode;
}

void Mesh::setColor(const glm::vec4 &_color) {
    m_colors.clear();
    for (uint i = 0; i < m_vertices.size(); i++) {
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

    for (uint i = 0; i < _mesh.getIndices().size(); i++) {
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

void Mesh::computeNormals() {

    if (getDrawMode() == GL_TRIANGLES) {
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

    }
    else {
        //  TODO
        //
        std::cout << "ERROR: computeNormals(): Mesh only add GL_TRIANGLES for NOW !!" << std::endl;
    }
}

// http://www.terathon.com/code/tangent.html
void Mesh::computeTangents() {

    if (getDrawMode() == GL_TRIANGLES) {
        //The number of the vertices
        int nV = m_vertices.size();

        //The number of the triangles
        int nT = m_indices.size() / 3;

        std::vector<glm::vec3> tan1( nV );
        std::vector<glm::vec3> tan2( nV );

        //Scan all the triangles. For each triangle add its
        //normal to norm's vectors of triangle's vertices
        for (int t = 0; t < nT; t++) {

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
        for (int i = 0; i < nV; i++) {
            const glm::vec3 &n = m_normals[i];
            const glm::vec3 &t = tan1[i];
            
            // Gram-Schmidt orthogonalize
            glm::vec3 tangent = t - n * glm::normalize( glm::dot(n, t));
    
            // Calculate handedness
            float hardedness = (glm::dot( glm::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;

            addTangent(glm::vec4(tangent, hardedness));
        }
    }
    else {
        //  TODO
        //
        std::cout << "ERROR: computeTangents(): Mesh only add GL_TRIANGLES for NOW !!" << std::endl;
    }
}

Vbo* Mesh::getVbo() {

    // Create Vertex Layout
    //
    std::vector<VertexLayout::VertexAttrib> attribs;
    attribs.push_back({"position", 3, GL_FLOAT, POSITION_ATTRIBUTE, false, 0});
    int  nBits = 3;

    bool bColor = false;
    if (hasColors() && getColors().size() == m_vertices.size()) {
        attribs.push_back({"color", 4, GL_FLOAT, COLOR_ATTRIBUTE, false, 0});
        bColor = true;
        nBits += 4;
    }

    bool bNormals = false;
    if (hasNormals() > 0 && getNormals().size() == m_vertices.size()) {
        attribs.push_back({"normal", 3, GL_FLOAT, NORMAL_ATTRIBUTE, false, 0});
        bNormals = true;
        nBits += 3;
    }

    bool bTexCoords = false;
    if (hasTexCoords() > 0 && getTexCoords().size() == m_vertices.size()) {
        attribs.push_back({"texcoord", 2, GL_FLOAT, TEXCOORD_ATTRIBUTE, false, 0});
        bTexCoords = true;
        nBits += 2;
    }

    bool bTangents = false;
    if (hasTangents() > 0 && getTangents().size() == m_vertices.size()) {
        attribs.push_back({"tangent", 4, GL_FLOAT, TANGENT_ATTRIBUTE, false, 0});
        bTangents = true;
        nBits += 4;
    }

    VertexLayout* vertexLayout = new VertexLayout(attribs);
    Vbo* tmpMesh = new Vbo(vertexLayout);
    tmpMesh->setDrawMode(getDrawMode());

    std::vector<GLfloat> data;
    for(uint i = 0; i < m_vertices.size(); i++) {
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
            for (uint i = 0; i < getVertices().size(); i++) {
                addIndex(i);
            }
        }
        else if ( getDrawMode() == GL_LINE_STRIP ) {
            for (uint i = 1; i < getVertices().size(); i++) {
                addIndex(i-1);
                addIndex(i);
            }
        }
    }

    tmpMesh->addIndices(m_indices.data(), m_indices.size());

    return tmpMesh;
}
