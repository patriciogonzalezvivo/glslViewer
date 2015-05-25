#pragma once

#include "gl/vbo.h"
#include "gl/shader.h"

class Cursor {
public:
	Cursor();
	virtual ~Cursor();

	void init();
	void draw();

private:
    Vbo* m_vbo;
    Shader m_shader;
};
