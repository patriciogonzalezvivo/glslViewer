#pragma once

#include <string>

#include "gl.h"

class Texture {
public:
	Texture();
	virtual ~Texture();

	bool load(const std::string& _filepath);
	bool load(unsigned char* _pixels, int _width, int _height);

	static bool savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height, bool _dither = false);

	const GLuint getId() const { return m_id; };
	std::string	 getFilePath() const { return m_path; };

	/* Width and Height texture getters */
	int	getWidth() const { return m_width; };
	int	getHeight() const { return m_height; };

	/* Binds the texture to GPU */
    void bind();

    /* Unbinds the texture from GPU */
    void unbind();

protected:
	void	glHandleError();
	
	std::string		m_path;

	int	m_width;
	int	m_height;

	GLuint 	m_id;
};