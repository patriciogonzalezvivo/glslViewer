#include "cursor.h"

#include "window.h"
#include "tools/text.h"
#include "types/shapes.h"
#include "types/rectangle.h"

std::string cursor_vert = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform vec2 u_mouse;\n\
attribute vec4 a_position;\n\
void main(void) {\n\
    vec4 position = vec4(u_mouse.x,u_mouse.y,0.0,0.0) + a_position;\n\
    gl_Position = u_modelViewProjectionMatrix * position;\n\
}\n";

std::string cursor_frag = "\n\
#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
void main(void) { \n\
    gl_FragColor = vec4(1.0);\n\
}\n";

Cursor::Cursor(): m_vbo(NULL) {
}

Cursor::~Cursor(){
    delete m_vbo;
}

void Cursor::init(){

    m_vbo = cross(glm::vec3(0.,0.,0.1), 10.).getVbo();

    std::vector<std::string> defines;
	m_shader.load(cursor_frag, cursor_vert, defines);
}

void Cursor::draw(){
    if (m_vbo != NULL) {
        glLineWidth(2.0f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_shader.use();
        m_shader.setUniform("u_mouse", getMouseX(), getMouseY());
        m_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
        m_vbo->draw(&m_shader);

        glLineWidth(1.0f);
        glDisable(GL_BLEND);
    }
}
