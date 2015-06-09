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

#include "shapes.h"

Mesh line (const glm::vec3 &_a, const glm::vec3 &_b) {
    glm::vec3 linePoints[2];
    linePoints[0] = glm::vec3(_a.x,_a.y,_a.z);
    linePoints[1] = glm::vec3(_b.x,_b.y,_b.z);;
    
    Mesh mesh;
    mesh.addVertices(linePoints,2);
    mesh.setDrawMode(GL_LINES);
    return mesh;
};

void drawLine(const glm::vec3 &_a, const glm::vec3 &_b){

    GLfloat g_vertex_buffer_data[] = {  _a.x,_a.y,_a.z,
                                        _b.x,_b.y,_b.z  };

    glVertexAttribPointer(
                        0,                      //vertexPosition_modelspaceID,
                        3,                      // size
                        GL_FLOAT,               // type
                        GL_FALSE,               // normalized?
                        0,                      // stride
                        g_vertex_buffer_data    // (void*)0 
                );

    glEnableVertexAttribArray ( 0 );
    glDrawArrays(GL_LINES, 0, 2);
};

Mesh cross (const glm::vec3 &_pos, float _width) {
	glm::vec3 linePoints[4] = {glm::vec3(_pos.x,_pos.y,_pos.z),
        glm::vec3(_pos.x,_pos.y,_pos.z),
        glm::vec3(_pos.x,_pos.y,_pos.z),
        glm::vec3(_pos.x,_pos.y,_pos.z) };

    linePoints[0].x -= _width;
    linePoints[1].x += _width;
    linePoints[2].y -= _width;
    linePoints[3].y += _width;
    
    Mesh mesh;
    mesh.addVertices(linePoints,4);
    mesh.setDrawMode(GL_LINES);
    return mesh;
}


void drawCross(const glm::vec3 &_pos, float _width ){
    glm::vec3 linePoints[4] = { glm::vec3(_pos.x,_pos.y,_pos.z),
                                glm::vec3(_pos.x,_pos.y,_pos.z),
                                glm::vec3(_pos.x,_pos.y,_pos.z),
                                glm::vec3(_pos.x,_pos.y,_pos.z) };
    linePoints[0].x -= _width;
    linePoints[1].x += _width;
    linePoints[2].y -= _width;
    linePoints[3].y += _width;

    // glEnableClientState(GL_VERTEX_ARRAY);
    // glVertexPointer(3, GL_FLOAT, 0, &linePoints[0].x);
    // glDrawArrays(GL_LINES, 0, 4);

    glVertexAttribPointer(
                        0,                  // vertexPosition_modelspaceID,
                        3,                  // size
                        GL_FLOAT,           // type
                        GL_FALSE,           // normalized?
                        0,                  // stride
                        (void*)&linePoints[0].x // (void*)0 
                        );

    glEnableVertexAttribArray( 0 );
    glDrawArrays(GL_LINES, 0, 4);
}

// Billboard
//============================================================================
Mesh rect (float _x, float _y, float _w, float _h) {
    float x = _x-1.0f;
    float y = _y-1.0f;
    float w = _w*2.0f;
    float h = _h*2.0f;

    Mesh mesh;
    mesh.addVertex(glm::vec3(x, y, 0.0));
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(0.0, 0.0));

    mesh.addVertex(glm::vec3(x+w, y, 0.0));
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(1.0, 0.0));

    mesh.addVertex(glm::vec3(x+w, y+h, 0.0));
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(1.0, 1.0));

    mesh.addVertex(glm::vec3(x, y+h, 0.0));
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(0.0, 1.0));

    mesh.addIndex(0);   mesh.addIndex(1);   mesh.addIndex(2);
    mesh.addIndex(2);   mesh.addIndex(3);   mesh.addIndex(0);
    
    return mesh;
}

Mesh rect (const Rectangle &_rect){
    Mesh mesh;
    mesh.addVertex(_rect.getBottomLeft());
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(0.0, 0.0));

    mesh.addVertex(_rect.getBottomRight());
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(1.0, 0.0));

    mesh.addVertex(_rect.getTopRight());
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(1.0, 1.0));

    mesh.addVertex(_rect.getTopLeft());
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(0.0, 1.0));

    mesh.addIndex(2);   mesh.addIndex(1);   mesh.addIndex(0);
    mesh.addIndex(0);   mesh.addIndex(3);   mesh.addIndex(2);
    
    return mesh;
}

Mesh rectBorders(const Rectangle &_rect){
    glm::vec3 linePoints[5] = {_rect.getTopLeft(), _rect.getTopRight(), _rect.getBottomRight(), _rect.getBottomLeft(), _rect.getTopLeft()};
    
    Mesh mesh;
    mesh.addVertices(linePoints,5);
    mesh.setDrawMode(GL_LINE_STRIP);
    return mesh;
}

void drawBorders( const Rectangle &_rect ){
    glm::vec3 linePoints[5] = { _rect.getTopLeft(), _rect.getTopRight(), _rect.getBottomRight(), _rect.getBottomLeft(), _rect.getTopLeft()};
    
    glVertexAttribPointer(
                        0,                  //vertexPosition_modelspaceID,
                        3,                  // size
                        GL_FLOAT,           // type
                        GL_FALSE,           // normalized?
                        0,                  // stride
                        &linePoints[0].x    // (void*)0 
                        );

    glEnableVertexAttribArray ( 0 );
    glDrawArrays(GL_LINE_STRIP, 0, 5);
}

Mesh rectCorners(const Rectangle &_rect, float _width ){
    glm::vec3 linePoints[16] = {_rect.getTopLeft(), _rect.getTopLeft(),_rect.getTopLeft(), _rect.getTopLeft(),
                                _rect.getTopRight(), _rect.getTopRight(),_rect.getTopRight(), _rect.getTopRight(),
                                _rect.getBottomRight(), _rect.getBottomRight(), _rect.getBottomRight(), _rect.getBottomRight(),
                                _rect.getBottomLeft(), _rect.getBottomLeft(),_rect.getBottomLeft(), _rect.getBottomLeft() };
    linePoints[0].x += _width;
    linePoints[3].y += _width;
    
    linePoints[4].x -= _width;
    linePoints[7].y += _width;
    
    linePoints[8].x -= _width;
    linePoints[11].y -= _width;
    
    linePoints[12].x += _width;
    linePoints[15].y -= _width;
    
    Mesh mesh;
    mesh.addVertices(linePoints,16);

    mesh.setDrawMode(GL_LINES);
    return mesh;
}

void drawCorners(const Rectangle &_rect, float _width){
    glm::vec3 linePoints[16] = {_rect.getTopLeft(), _rect.getTopLeft(),_rect.getTopLeft(), _rect.getTopLeft(),
                                _rect.getTopRight(), _rect.getTopRight(),_rect.getTopRight(), _rect.getTopRight(),
                                _rect.getBottomRight(), _rect.getBottomRight(), _rect.getBottomRight(), _rect.getBottomRight(),
                                _rect.getBottomLeft(), _rect.getBottomLeft(),_rect.getBottomLeft(), _rect.getBottomLeft() };
    linePoints[0].x += _width;
    linePoints[3].y += _width;
    
    linePoints[4].x -= _width;
    linePoints[7].y += _width;
    
    linePoints[8].x -= _width;
    linePoints[11].y -= _width;
    
    linePoints[12].x += _width;
    linePoints[15].y -= _width;

    glVertexAttribPointer(
                        0,                  //vertexPosition_modelspaceID,
                        3,                  // size
                        GL_FLOAT,           // type
                        GL_FALSE,           // normalized?
                        0,                  // stride
                        &linePoints[0].x    // (void*)0 
                        );

    glEnableVertexAttribArray ( 0 );
    glDrawArrays(GL_LINES, 0, 16);
}

Mesh polyline (const std::vector<glm::vec3> &_pts ) {
	Mesh mesh;
    mesh.addVertices(_pts);
    mesh.setDrawMode(GL_LINE_STRIP);
    return mesh;
}

void drawPolyline(const std::vector<glm::vec3> &_pts ){
    glVertexAttribPointer(
                        0,          //vertexPosition_modelspaceID,
                        3,          // size
                        GL_FLOAT,   // type
                        GL_FALSE,   // normalized?
                        0,          // stride
                        &_pts[0].x  // (void*)0 
                        );

    glEnableVertexAttribArray ( 0 );
    glDrawArrays(GL_LINE_STRIP, 0, _pts.size());
}