#include "texture.h"
#include "image.h"

Texture::Texture():m_width(0),m_height(0),m_bpp(0),m_id(0) {
}

Texture::~Texture() {
	glDeleteTextures(1, &m_id);
}

bool Texture::load(const std::string& _path) {
	FIBITMAP *image = loadImage(_path);
    int width = FreeImage_GetWidth(image);
	int height = FreeImage_GetHeight(image);

	// If somehow one of these failed (they shouldn't), return failure
	if(image == NULL || width == 0 || height == 0){
		std::cerr << "Texture: something went wrong" << std::endl;
		return false; 
	} 

    loadPixels((unsigned char*)FreeImage_GetBits(image),width,height);

    if (image != NULL) {
        FreeImage_Unload(image);
    }
    
    return true;
}

bool Texture::loadPixels(unsigned char* _pixels, int _width, int _height) {
    m_width = _width;
    m_height = _height;

    glEnable(GL_TEXTURE_2D);

    // Generate an OpenGL texture ID for this texture
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id); 

#ifdef PLATFORM_RPI
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, m_width, m_height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, _pixels);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, _pixels);
#endif

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    unbind();

    return true;
}

bool Texture::save(const std::string& _path) {
    bind();
    unsigned char* pixels = new unsigned char[m_width*m_height*4];
    glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    bool rta = savePixels(_path, pixels, m_width, m_height);
    unbind();

    return rta;
}

bool Texture::savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height, bool _dither) {
    return saveImage(_path, _pixels, _width, _height, _dither);
}

void Texture::bind() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}
