#pragma once

#include <string>

#include "gl.h"

class Texture {
public:
	Texture();
	virtual ~Texture();

	bool load(const std::string& _path);
	bool loadPixels(unsigned char* _pixels, int _width, int _height);

	bool save(const std::string& _path);
	static	bool savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height, bool _dither = false);

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