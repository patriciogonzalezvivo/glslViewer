#pragma once

#include <string>
#include <pthread.h>

#include "gl.h"

class Texture {
public:
	Texture();
	virtual ~Texture();

	bool load(const std::string& _path);

	const GLuint getId() const { return m_id; };
	
	int	getWidht() const { return m_width; };
	int	getHeight() const { return m_height; };

	void bind();
	void unbind();

protected:

	int m_width;
	int m_height;

	GLuint 	m_id;
};