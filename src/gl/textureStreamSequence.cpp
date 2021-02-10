#include "textureStreamSequence.h"

#include "../io/fs.h"
#include "../io/pixels.h"

TextureStreamSequence::TextureStreamSequence() : m_currentFrame(0), m_bits(8) {

}

TextureStreamSequence::~TextureStreamSequence() {
    clear();
}

bool TextureStreamSequence::load(const std::string& _path, bool _vFlip) {
    m_path = _path;
    m_vFlip = _vFlip;

    std::vector<std::string> files = glob(_path);
    for (size_t i = 0; i < files.size(); i++) {

        std::string ext = getExt(files[i]);

        // BMP non-1bpp, non-RLE
        // GIF (*comp always reports as 4-channel)
        // JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
        if (ext == "bmp"    || ext == "BMP" ||
            ext == "jpg"    || ext == "JPG" ||
            ext == "jpeg"   || ext == "JPEG" ) {

            m_bits = 8;
            unsigned char* pixels = loadPixels(files[i], &m_width, &m_height, RGB_ALPHA, _vFlip);
            #ifdef PLATFORM_RPI
            int max_size = std::max(m_width, m_height);
            if ( max_size > 1024) {
                float factor = max_size/1024.0;
                int w = m_width/factor;
                int h = m_height/factor;
                unsigned char * data = new unsigned char [w * h * 4];
                rescalePixels((unsigned char*)pixels, m_width, m_height, 4, w, h, data);
                delete[] pixels;
                pixels = data;
                m_width = w;
                m_height = h;
            }
            #endif
            m_frames.push_back( (void*)pixels );
        }
        else if (   ext == "png" || ext == "PNG" ||
                    ext == "psd" || ext == "PSD" ||
                    ext == "tga" || ext == "TGA" ) {
            // m_bits = 8;
            // unsigned char* pixels = loadPixels(files[i], &m_width, &m_height, RGB_ALPHA, _vFlip);

            #ifdef PLATFORM_RPI
            m_bits = 8;
            // If we are in a Raspberry Pi don't take the risk of loading a 16bit image
            unsigned char* pixels = loadPixels(files[i], &m_width, &m_height, RGB_ALPHA, _vFlip);
            int max_size = std::max(m_width, m_height);
            if ( max_size > 1024) {
                float factor = max_size/1024.0;
                int w = m_width/factor;
                int h = m_height/factor;
                unsigned char * data = new unsigned char [w * h * 4];
                rescalePixels((unsigned char*)pixels, m_width, m_height, 4, w, h, data);
                delete[] pixels;
                pixels = data;
                m_width = w;
                m_height = h;
            }
            m_frames.push_back( (void*)pixels );
            #else
            m_bits = 16;
            uint16_t* pixels = loadPixels16(files[i], &m_width, &m_height, RGB_ALPHA, _vFlip);
            m_frames.push_back( (void*)pixels );
            #endif
        }
    }
    
    return true;
}

bool TextureStreamSequence::update() {
    if (m_frames.size() == 0)
        return false;

    if ( Texture::load(m_width, m_height, 4, m_bits, m_frames[ m_currentFrame ]) ) {
        m_currentFrame = (m_currentFrame + 1) % m_frames.size();
        return true;
    }

    return false;
}

void TextureStreamSequence::clear() {
    for (size_t i = 0; i < m_frames.size(); i++)
        delete [] (m_frames[i]);

    m_frames.clear();

    if (m_id != 0)
        glDeleteTextures(1, &m_id);

    m_id = 0;
}
