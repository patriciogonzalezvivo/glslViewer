#include "fbo.h"
#include <iostream>

Fbo::Fbo():m_width(0), m_height(0), m_id(0), m_color_texture(0), m_depth_texture(0) {
    // Create a frame buffer
    glGenFramebuffers(1, &m_id);

    // Generate a texture to hold the colour buffer
    glGenTextures(1, &m_color_texture);

    // Create a texture to hold the depth buffer
    glGenTextures(1, &m_depth_texture);
}

Fbo::Fbo(unsigned int _width, unsigned int _height):Fbo() {
    // Allocate Textures
    resize(_width,_height);
}

Fbo::~Fbo() {
    glDeleteFramebuffers(1, &m_id);
}

void Fbo::resize(const unsigned int _width, const unsigned int _height) {
    m_width = _width;
    m_height = _height;

    //  Render (color) texture
    glBindTexture(GL_TEXTURE_2D, m_color_texture);
    glTexImage2D(   GL_TEXTURE_2D, 0, GL_RGBA,
                    m_width, m_height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth Texture
    glBindTexture(GL_TEXTURE_2D, m_depth_texture);
    glTexImage2D(   GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                    m_width, m_height,
                    0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Associate the textures with the FBO
    bind();
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_color_texture, 0);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_texture, 0);
    unbind();

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Failed to create FBO" << std::endl;
    }
}

void Fbo::bind() {
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&m_old_fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, m_id);
}

void Fbo::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_old_fbo_id);
}
