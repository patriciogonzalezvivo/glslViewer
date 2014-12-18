#pragma once

#include <string>

#include <GLES2/gl2.h>

class Texture {
public:
	Texture();
	virtual ~Texture();

	bool load(const std::string& _path);

	const GLuint getId() const { return m_id; };
	
	int	getWidth() const { return m_width; };
	int	getHeight() const { return m_height; };

	void bind();
	void unbind();

protected:
	void	glHandleError();

	unsigned int	m_width;
	unsigned int	m_height;
	unsigned int 	m_bpp;

	GLuint 	m_id;
};