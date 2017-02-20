#include "cursor.h"

#include "types/shapes.h"
#include "types/rectangle.h"
#include "app.h"
#include "utils.h"

#ifndef STRINGIFY
#define STRINGIFY(A) #A
#endif

Cursor::Cursor(): m_vbo(NULL) {

}

Cursor::~Cursor(){
    delete m_vbo;
}

void Cursor::init(){

    m_vbo = cross(glm::vec3(0.,0.,0.1), 10.).getVbo(); 

	std::string vert, frag =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

	vert += STRINGIFY(
uniform mat4 u_modelViewProjectionMatrix;
uniform vec2 u_mouse;
attribute vec4 a_position;
void main(void) {
    vec4 position = vec4(u_mouse.x,u_mouse.y,0.0,0.0) + a_position;
    gl_Position = u_modelViewProjectionMatrix * position;
} );

	frag += STRINGIFY(
void main(void) {
    gl_FragColor = vec4(1.0);
} );

	m_shader.load(nullptr, frag, nullptr, vert, false);
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
