#include <iostream>

#include "texture.h"

#include "gl/gl.h"

#include "tools/fs.h"

#define STB_IMAGE_IMPLEMENTATION
#include "std/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "std/stb_image_write.h"

Texture::Texture():m_path(""),m_width(0),m_height(0),m_id(0) {
}

Texture::~Texture() {
	glDeleteTextures(1, &m_id);
}

/**
 * pixels_buffer - Pixels buffer to be operated
 * width - Image width
 * height - Image height
 * bytes_per_pixel - Number of image components, ie: 3 for rgb, 4 rgba, etc...
 **/
template<typename T>
void flipVertically(T *_pixels, const unsigned int _width, const unsigned int _height, const int _bytes_per_pixel) {
    int row, col, z;
    T temp;
    for (row = 0; row < (_height>>1); row++) {
        for (col = 0; col < _width; col++) {
            for (z = 0; z < _bytes_per_pixel; z++) {
                temp = _pixels[(row * _width + col) * _bytes_per_pixel + z];
                _pixels[(row * _width + col) * _bytes_per_pixel + z] = _pixels[((_height - row - 1) * _width + col) * _bytes_per_pixel + z];
                _pixels[((_height - row - 1) * _width + col) * _bytes_per_pixel + z] = temp;
            }
        }
    }

    // const size_t stride = _width * _bytes_per_pixel;
    // T *row = (T*)malloc(stride);
    // T *low = _pixels;
    // T *high = &_pixels[(_height - 1) * stride];

    // for (; low < high; low += stride, high -= stride) {
    //     memcpy(row, low, stride * sizeof(T));
    //     memcpy(low, high, stride * sizeof(T));
    //     memcpy(high, row, stride * sizeof(T));
    // }
    // free(row);

    // const unsigned int rows = _height / 2; // Iterate only half the buffer to get a full flip
    // const unsigned int row_stride = _width * _bytes_per_pixel;
    // T* temp_row = (T*)malloc(row_stride);

    // int source_offset, target_offset;

    // for (int rowIndex = 0; rowIndex < rows; rowIndex++) {
    //     source_offset = rowIndex * row_stride;
    //     target_offset = (_height - rowIndex - 1) * row_stride;

    //     memcpy(temp_row, _pixels + source_offset, row_stride * sizeof(T));
    //     memcpy(_pixels + source_offset, _pixels + target_offset, row_stride * sizeof(T));
    //     memcpy(_pixels + target_offset, temp_row, row_stride * sizeof(T));
    // }

    // free(temp_row);
    // temp_row = NULL;
}

bool Texture::load(const std::string& _path, bool _vFlip) {
    m_path = _path;

    if (haveExt(_path,"png") || haveExt(_path,"PNG") ||
        haveExt(_path,"jpg") || haveExt(_path,"JPG") ||
        haveExt(_path,"jpeg") || haveExt(_path,"JPEG")) {

        std::cout << "PNG/JPG" << std::endl;

        stbi_set_flip_vertically_on_load(_vFlip);
        int comp;
        unsigned char* pixels = stbi_load(_path.c_str(), &m_width, &m_height, &comp, STBI_rgb_alpha);

        load(pixels, m_width, m_height);
        stbi_image_free(pixels);
    }
    else if (haveExt(_path, "hdr") || haveExt(_path,"HDR")) {
        std::cout << "HDR" << std::endl;
        int channels = 3;
        FILE* file = fopen(_path.c_str(), "rb");
        RGBE_ReadHeader(file, &m_width, &m_height, NULL);
        float* pixels = new float[m_width * m_height * channels];
        RGBE_ReadPixels_RLE(file, pixels, m_width, m_height);
        
        if (_vFlip) {
            flipVertically<float>(pixels, m_width, m_height, channels);
        }
 
        loadFloat(pixels, m_width, m_height);

        fclose(file);
        delete[] pixels;
    }

    return true;
}

bool Texture::load(unsigned char* _pixels, int _width, int _height) {
    m_width = _width;
    m_height = _height;

    glEnable(GL_TEXTURE_2D);

    if(m_id == 0){
        // Generate an OpenGL texture ID for this texture
        glGenTextures(1, &m_id);
    }

    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _pixels);

    unbind();

    return true;
}

bool Texture::loadFloat(float* _pixels, int _width, int _height) {
    m_width = _width;
    m_height = _height;

    glEnable(GL_TEXTURE_2D);

    if(m_id == 0){
        // Generate an OpenGL texture ID for this texture
        glGenTextures(1, &m_id);
    }

    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

#ifndef PLATFORM_RPI
    GLenum InternalFormat = GL_RGB16F_ARB;
#else
    GLenum InternalFormat = GL_RGB;
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, m_width, m_height, 0, GL_RGB, GL_FLOAT, _pixels);

    unbind();

    return true;
}

unsigned char *Texture::loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels, bool _vFlip) {
    stbi_set_flip_vertically_on_load(_vFlip);
    int comp;
    return stbi_load(_path.c_str(), _width, _height, &comp, (_channels == RGB)? STBI_rgb : STBI_rgb_alpha);
}

bool Texture::savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height) {

    // Flip the image on Y
    int depth = 4;
    // unsigned char *result = new unsigned char[_width*_height*depth];
    // memcpy(result, _pixels, _width * _height * depth);
    flipVertically<unsigned char>(_pixels, _width, _height, depth);
    if (0 == stbi_write_png(_path.c_str(), _width, _height, 4, _pixels, _width * 4)) {
        std::cout << "can't create file " << _path << std::endl;
    }

    // int row,col,z;
    // stbi_uc temp;

    // for (row = 0; row < (_height>>1); row++) {
    //  for (col = 0; col < _width; col++) {
    //     for (z = 0; z < depth; z++) {
    //        temp = result[(row * _width + col) * depth + z];
    //        result[(row * _width + col) * depth + z] = result[((_height - row - 1) * _width + col) * depth + z];
    //        result[((_height - row - 1) * _width + col) * depth + z] = temp;
    //     }
    //  }
    // }
    // if (0 == stbi_write_png(_path.c_str(), _width, _height, 4, result, _width * 4)) {
    //     std::cout << "can't create file " << _path << std::endl;
    // }
    // delete [] result;

    return true;
}

void Texture::bind() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}
