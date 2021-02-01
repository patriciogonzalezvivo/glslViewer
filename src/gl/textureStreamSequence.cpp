#include "textureStreamSequence.h"

#include "../io/fs.h"
#include "../io/pixels.h"

TextureStreamSequence::TextureStreamSequence() : m_currentFrame(0) {

}

TextureStreamSequence::~TextureStreamSequence() {
    clear();
}

bool TextureStreamSequence::load(const std::string& _path, bool _vFlip) {
    m_path = _path;
    m_vFlip = _vFlip;

    std::vector<std::string> files = glob(_path);
    for (size_t i = 0; i < files.size(); i++) {
        // Load all images as 8bits images
        // TODOs:
        //  - this can be improve making PNGs 16bits by using void* instead of unsigned char *;
        m_frames.push_back( loadPixels(files[i], &m_width, &m_height, RGB_ALPHA, _vFlip) );
    }
    
    return true;
}

bool TextureStreamSequence::update() {
    if (m_frames.size() == 0)
        return false;

    bool loaded = Texture::load(m_width, m_height, 4, 8, m_frames[ m_currentFrame ]);
    m_currentFrame = (m_currentFrame + 1) % m_frames.size();

    return loaded;
}

void TextureStreamSequence::clear() {
    for (size_t i = 0; i < m_frames.size(); i++)
        delete m_frames[i];

    m_frames.clear();

    if (m_id != 0)
        glDeleteTextures(1, &m_id);

    m_id = 0;
}