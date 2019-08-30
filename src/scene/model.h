#pragma once

#include "node.h"
#include "gl/vbo.h"

typedef std::vector<std::string> List;

class Model : public Node {
public:
    Model();
    virtual ~Model();

    void    load(const std::string &_path, List &_defines);
    bool    loaded() { return m_bbox_vbo != nullptr; }
    void    clear();

    Vbo*    getVbo() { return m_model_vbo; }
    float   getArea() { return m_area; }
    Vbo*    getBboxVbo() { return m_bbox_vbo; }

    void    draw();

protected:
    Vbo*    m_model_vbo;
    Vbo*    m_bbox_vbo;
    float   m_area;
};