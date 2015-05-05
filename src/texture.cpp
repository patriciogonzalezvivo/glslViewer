#include "texture.h"
#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Texture::Texture():m_path(""),m_width(0),m_height(0),m_id(0) {
}

Texture::~Texture() {
	glDeleteTextures(1, &m_id);
}

bool Texture::load(const std::string& _path) {
    stbi_set_flip_vertically_on_load(true);
    int comp;
    unsigned char* pixels = stbi_load(_path.c_str(), &m_width, &m_height, &comp, STBI_rgb_alpha);

    load(pixels, m_width, m_height);
    stbi_image_free(pixels);

    m_path = _path;
    return true;
}

bool Texture::load(unsigned char* _pixels, int _width, int _height) {
    m_width = _width;
    m_height = _height;

    glEnable(GL_TEXTURE_2D);

    // Generate an OpenGL texture ID for this texture
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

#ifdef PLATFORM_RPI
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, m_width, m_height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, _pixels);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _pixels);
#endif

    unbind();

    return true;
}

// bool Texture::save(const std::string& _path) {
//     bind();
//     unsigned char* pixels = new unsigned char[m_width*m_height*4];
//     glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
//     bool rta = savePixels(_path, pixels, m_width, m_height);
//     unbind();

//     return rta;
// }

bool Texture::savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height) {

    // Flip the image on Y
    int w = _width, h = _height;
    int depth = 4;
    unsigned char *result = new unsigned char[w*h*depth];
    memcpy(result, _pixels, w*h*depth);
    int row,col,z;
    stbi_uc temp;

    for (row = 0; row < (h>>1); row++) {
     for (col = 0; col < w; col++) {
        for (z = 0; z < depth; z++) {
           temp = result[(row * w + col) * depth + z];
           result[(row * w + col) * depth + z] = result[((h - row - 1) * w + col) * depth + z];
           result[((h - row - 1) * w + col) * depth + z] = temp;
        }
     }
    }

    stbi_write_png(_path.c_str(), _width, _height, 4, result, _width * 4);
    delete [] result;
    
    return true;
}

void Texture::bind() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}
