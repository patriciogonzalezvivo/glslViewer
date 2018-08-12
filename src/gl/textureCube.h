
#pragma once

#include "texture.h"

#include <stdexcept>
extern "C" {
    #include "rgbe/rgbe.h"    
}

template <typename T> 
struct Face {

    void flipHorizontal() {
        int dataSize = width * height * 3;
        int n = sizeof(T) * 3 * width;
        T* newData = new T[dataSize];

        for (int i = 0; i < height; i++) {
            int offset = i * width * 3;
            int bias = -(i + 1) * 3 * width;

            memcpy(newData + dataSize + bias, data + offset, n);
        }

        delete[] data;
        data = newData;
    }

    void flipVertical() {
        int dataSize = width * height * 3;
        int n = sizeof(T) * 3;
        T* newData = new T[dataSize];

        for(int i = 0; i < height; ++i) {
            int lineOffset = i * width * 3;

            for(int j = 0; j < width; ++j) {
                int offset = lineOffset + j * 3;
                int bias = lineOffset + width * 3 - (j + 1) * 3;

                memcpy(newData + bias, data + offset, n);
            }
        }

        delete[] data;
        data = newData;
    }

    void upload(GLenum _face, GLenum _type) {
        // GLenum Type = GL_FLOAT;

        // if(std::is_same<T,unsigned char*>::value) {
        //     Type = GL_UNSIGNED_BYTE;
        //     std::cout << "GL_UNSIGNED_BYTE" << std::end;
        // }
        // else if(std::is_same<T,float>::value) {
        //     std::cout << "GL_FLOAT" << std::end;
        //     Type = GL_FLOAT;
        // }     

    #ifndef PLATFORM_RPI
        GLenum InternalFormat = GL_RGB16F_ARB;
    #else
        GLenum InternalFormat = GL_RGB;
    #endif

        glTexImage2D(_face, 0, InternalFormat, width, height, 0, GL_RGB, _type, data);
    }

	int width;
	int height;
	// for mem copy purposes only
	int currentOffset;
    T   *data;


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
};

