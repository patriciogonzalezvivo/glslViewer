
#pragma once

#include "texture.h"

struct Face {
	float *data;
	int width;
	int height;
	// for mem copy purposes only
	int currentOffset;
};

const GLenum CubeMapFace[6] { 
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, 
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 
};
	
class TextureCube : public Texture {
public:
    TextureCube();
    virtual ~TextureCube();

    virtual bool    load(const std::string &_fileName, bool _vFlip = true);
    virtual void    bind();

private:
    void    _loadFaces();
    void    _flipHorizontal(Face* face);
    void    _flipVertical(Face* face);

    Face    **m_faces;
};

