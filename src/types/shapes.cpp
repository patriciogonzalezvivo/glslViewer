#include "shapes.h"
#include "../tools/geom.h"

Mesh line (const glm::vec3 &_a, const glm::vec3 &_b) {
    glm::vec3 linePoints[2];
    linePoints[0] = glm::vec3(_a.x,_a.y,_a.z);
    linePoints[1] = glm::vec3(_b.x,_b.y,_b.z);;

    Mesh mesh;
    mesh.addVertices(linePoints,2);
    mesh.setDrawMode(GL_LINES);
    return mesh;
};

Mesh lineTo(const glm::vec3 &_a, const glm::vec3 &_dir, float _size) {
    return line(_a, _a + normalize(_dir) * _size );
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

Mesh rect (const Rectangle &_rect) {
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

Mesh rectBorders(const Rectangle &_rect) {
    glm::vec3 linePoints[5] = {_rect.getTopLeft(), _rect.getTopRight(), _rect.getBottomRight(), _rect.getBottomLeft(), _rect.getTopLeft()};

    Mesh mesh;
    mesh.addVertices(linePoints,5);
    mesh.setDrawMode(GL_LINE_STRIP);
    return mesh;
}

Mesh rectCorners(const Rectangle &_rect, float _width ) {
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

Mesh polyline (const std::vector<glm::vec3> &_pts ) {
    Mesh mesh;
    mesh.addVertices(_pts);
    mesh.setDrawMode(GL_LINE_STRIP);
    return mesh;
}

Mesh cube(float _size) {
    float vertices[] = {
        -_size,  _size,  _size,
        -_size, -_size,  _size,
        _size, -_size,  _size,
        _size,  _size,  _size,
        -_size,  _size, -_size,
        -_size, -_size, -_size,
        _size, -_size, -_size,
        _size,  _size, -_size,
    };

    uint32_t indices[] = {
        0, 1, 2,
        0, 2, 3,
        3, 2, 6,
        3, 6, 7,
        0, 4, 7,
        0, 7, 3,
        4, 6, 7,
        4, 6, 5,
        0, 5, 4,
        0, 5, 1,
        1, 6, 5,
        1, 6, 2,
    };

    Mesh mesh;
    mesh.addVertices(reinterpret_cast<glm::vec3*>(vertices), 8);
    mesh.addIndices(indices, 36);
    return mesh;
}

Mesh cubeCorners(const glm::vec3 &_min_v, const glm::vec3 &_max_v, float _size) {
    float size = glm::min(glm::length(_min_v), glm::length(_max_v)) * _size *  0.5;

    //    D ---- A
    // C ---- B  |
    // |  |   |  |
    // |  I --|- F
    // H .... G

    glm::vec3 A = _max_v;
    glm::vec3 H = _min_v;

    glm::vec3 B = glm::vec3(A.x, A.y, H.z);
    glm::vec3 C = glm::vec3(H.x, A.y, H.z);
    glm::vec3 D = glm::vec3(H.x, A.y, A.z);

    glm::vec3 F = glm::vec3(A.x, H.y, A.z);
    glm::vec3 G = glm::vec3(A.x, H.y, H.z);
    glm::vec3 I = glm::vec3(H.x, H.y, A.z);

    Mesh mesh;
    mesh.setDrawMode(GL_LINES);
    mesh.add( lineTo(A, normalize(D-A), size) );
    mesh.add( lineTo(A, normalize(B-A), size) );
    mesh.add( lineTo(A, normalize(F-A), size) );

    mesh.add( lineTo(B, normalize(A-B), size) );
    mesh.add( lineTo(B, normalize(C-B), size) );
    mesh.add( lineTo(B, normalize(G-B), size) );

    mesh.add( lineTo(C, normalize(D-C), size) );
    mesh.add( lineTo(C, normalize(B-C), size) );
    mesh.add( lineTo(C, normalize(H-C), size) );
    
    mesh.add( lineTo(D, normalize(A-D), size) );
    mesh.add( lineTo(D, normalize(C-D), size) );
    mesh.add( lineTo(D, normalize(I-D), size) );

    mesh.add( lineTo(F, normalize(G-F), size) );
    mesh.add( lineTo(F, normalize(A-F), size) );
    mesh.add( lineTo(F, normalize(I-F), size) );

    mesh.add( lineTo(G, normalize(H-G), size) );
    mesh.add( lineTo(G, normalize(F-G), size) );
    mesh.add( lineTo(G, normalize(B-G), size) );

    mesh.add( lineTo(H, normalize(I-H), size) );
    mesh.add( lineTo(H, normalize(G-H), size) );
    mesh.add( lineTo(H, normalize(C-H), size) );

    mesh.add( lineTo(I, normalize(F-I), size) );
    mesh.add( lineTo(I, normalize(H-I), size) );
    mesh.add( lineTo(I, normalize(D-I), size) );

    return mesh;
}

Mesh cubeCorners(const std::vector<glm::vec3> &_pts, float _size) {
    glm::vec3 min_v;
    glm::vec3 max_v;
    getBoundingBox( _pts, min_v, max_v);
    return cubeCorners(min_v, max_v, _size);
}

Mesh grid (float _width, float _height, int _columns, int _rows) {
    Mesh mesh;
    mesh.setDrawMode(GL_LINES);

    // the origin of the plane is at the center //
    float halfW = _width  * 0.5f;
    float halfH = _height * 0.5f;

    //  . --- A
    //  |     |
    //  B --- .

    glm::vec3 A = glm::vec3(halfW, 0.0, halfH);
    glm::vec3 B = glm::vec3(-halfW, 0.0, -halfH);

    // add the vertexes //
    for(int iy = 0; iy != _rows; iy++) {
        float pct = ((float)iy/((float)_rows-1));

        glm::vec3 left = glm::mix(A, B, glm::vec3(0.0, 0.0, pct));
        glm::vec3 right = glm::mix(A, B, glm::vec3(1.0, 0.0, pct));

        mesh.add( line(left, right) );
    }

    for(int ix = 0; ix != _columns; ix++) {
        float pct = ((float)ix/((float)_columns-1));

        glm::vec3 top = glm::mix(A, B, glm::vec3(pct, 0.0, 0.0));
        glm::vec3 down = glm::mix(A, B, glm::vec3(pct, 0.0, 1.0));

        mesh.add( line(top, down) );
    }

    return mesh;
}

Mesh grid(float _size, int _segments) {
    return grid(_size, _size, _segments, _segments);
}

Mesh axis(float _size) {
    Mesh mesh;
    mesh.setDrawMode(GL_LINES);

    mesh.add( line(glm::vec3(_size,0.0,0.0), glm::vec3(-_size,0.0,0.0)));
    mesh.add( line(glm::vec3(0.0, _size, 0.0), glm::vec3(0.0, -_size, 0.0)));
    mesh.add( line(glm::vec3(0.0, 0.0, _size), glm::vec3(0.0, 0.0, -_size)));

    return mesh;
}