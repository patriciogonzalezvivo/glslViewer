#pragma once

#include <string>

#include "gl.h"

enum Channels {
	RGB,
	RGBA
};

class Texture {
public:
	Texture();
	virtual ~Texture();

	virtual bool	load(const std::string& _filepath, bool _vFlip = true);
	virtual bool 	load(unsigned char* _pixels, int _width, int _height);

	static bool 	savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height);
	unsigned char	*loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);

	virtual const GLuint	getId() const { return m_id; };
	virtual std::string	 	getFilePath() const { return m_path; };
	virtual int				getWidth() const { return m_width; };
	virtual int				getHeight() const { return m_height; };

	/* Bind/Unbind the texture to GPU */
    virtual void 	bind();
    virtual void 	unbind();

protected:
	// virtual void	glHandleError();

	std::string		m_path;

	int				m_width;
	int				m_height;

	GLuint 			m_id;
};
