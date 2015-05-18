#include "shapes.h"

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

Mesh line (const glm::vec3 &_a, const glm::vec3 &_b) {
    glm::vec3 linePoints[2];
    linePoints[0] = glm::vec3(_a.x,_a.y,_a.z);
    linePoints[1] = glm::vec3(_b.x,_b.y,_b.z);;
    
    Mesh mesh;
    mesh.addVertices(linePoints,2);
    mesh.setDrawMode(GL_LINES);
    return mesh;
};

Mesh polyline (const std::vector<glm::vec3> &_pts ) {
	Mesh mesh;
    mesh.addVertices(_pts);
    mesh.setDrawMode(GL_LINE_STRIP);
    return mesh;
}