/*
Copyright (c) 2014, Patricio Gonzalez Vivo
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <vector>
#include <string>

#include "glm/glm.hpp"

#include "../gl/vbo.h"

class Mesh {
public:
    
    Mesh();
    Mesh(const Mesh &_mother);
    virtual ~Mesh();
    
    bool    load(const std::string& _file);
    bool    save(const std::string& _file, bool _useBinary = false);

    void    setDrawMode(GLenum _drawMode = GL_TRIANGLES);
    
    void    setColor(const glm::vec4 &_color);
    void    addColor(const glm::vec4 &_color);
    void    addColors(const std::vector<glm::vec4> &_colors);
    
    void    addVertex(const glm::vec3 &_point);
    void    addVertices(const std::vector<glm::vec3>& _verts);
    void    addVertices(const glm::vec3* _verts, int _amt);
    
    void    addNormal(const glm::vec3 &_normal);
    void    addNormals(const std::vector<glm::vec3> &_normals );
    
    void    addTexCoord(const glm::vec2 &_uv);
    void    addTexCoords(const std::vector<glm::vec2> &_uvs);
    
    void    addIndex(uint16_t _i);
    void    addIndices(const std::vector<uint16_t>& _inds);
    void    addIndices(const uint16_t* _inds, int _amt);
    
    void    addTriangle(uint16_t index1, uint16_t index2, uint16_t index3);
    
    void    add(const Mesh &_mesh);
    
    GLenum  getDrawMode() const;
    std::vector<glm::ivec3>  getTriangles() const ;
    
    const std::vector<glm::vec4> & getColors() const;
    const std::vector<glm::vec3> & getVertices() const;
    const std::vector<glm::vec3> & getNormals() const;
    const std::vector<glm::vec2> & getTexCoords() const;
    const std::vector<uint16_t>  & getIndices() const;

    Vbo*    getVbo();

    void    computeNormals();
    void    clear();
    
private:
    std::vector<glm::vec4>  m_colors;
    std::vector<glm::vec3>  m_vertices;
    std::vector<glm::vec3>  m_normals;
    std::vector<glm::vec2>  m_texCoords;
    std::vector<uint16_t>   m_indices;
    
    GLenum    m_drawMode;
};
