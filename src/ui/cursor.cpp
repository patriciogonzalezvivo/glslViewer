#include "cursor.h"

#include "types/shapes.h"
#include "types/rectangle.h"
#include "app.h"

#define STRINGIFY(A) #A

Cursor::Cursor(): m_vbo(NULL) {

}

Cursor::~Cursor(){
	delete m_vbo;
}

void Cursor::init(){
	m_vbo = cross(glm::vec3(0.,0.,0.1), 10.).getVbo();

	std::string vert =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

	std::string frag = vert;

	vert += STRINGIFY(

uniform mat4 u_modelViewProjectionMatrix;

uniform vec2 u_mouse;
uniform vec2 u_resolution;

attribute vec4 a_position;
attribute vec4 a_color;

varying vec4 v_color;
varying vec4 v_position;

void main(void) {
	v_position = vec4(u_mouse.x,u_mouse.y,0.0,0.0) + a_position;
	v_color = a_color;
	gl_Position = u_modelViewProjectionMatrix * v_position;
}

);

	frag += STRINGIFY(

uniform vec2 u_mouse;
void main(void) {
    gl_FragColor = vec4(1.0);
}
);

	frag = m_vbo->getVertexLayout()->getDefaultFragShader();

	m_shader.load(frag,vert);
}

void Cursor::draw(){

	glPolygonOffset(-1.0f, -1.0f);      // Shift depth values
    glEnable(GL_POLYGON_OFFSET_LINE);

	glLineWidth(2.0f);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// // Draw lines antialiased
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_shader.use();
	m_shader.setUniform("u_mouse", getMouseX(), getMouseY());
	m_shader.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
	m_shader.setUniform("u_modelViewProjectionMatrix", getOrthoMatrix());
	m_vbo->draw(&m_shader);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}