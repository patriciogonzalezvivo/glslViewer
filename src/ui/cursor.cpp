#include "cursor.h"

#include "types/shapes.h"
#include "types/rectangle.h"
#include "app.h"

#define STRINGIFY(A) #A

Cursor::Cursor(): m_vbo(NULL) {

}

Cursor::~Cursor(){
	// delete m_vbo;
}

void Cursor::init(){

	Rectangle r = Rectangle(.0,.0,.1,0.1);

	m_vbo = cross(glm::vec3(0.5,0.5,0.5), 0.1).getVbo();
	// m_vbo = rect(r).getVbo();

	std::string vert =
"#ifdef GL_ES\n"
"precision mediump float;\n"
"#endif\n";

	std::string frag = vert;

	vert += STRINGIFY(

uniform vec2 u_mouse;
uniform vec2 u_resolution;
attribute vec4 a_position;
void main(void) {
	vec2 mouse = (u_mouse-u_resolution*0.5)/u_resolution;
    gl_Position = (vec4(mouse.x,mouse.y,0.0,0.0)*2.0) + a_position;
}

);

	frag += STRINGIFY(

uniform vec2 u_mouse;
void main(void) {
    gl_FragColor = vec4(1.0);
}
);

	m_shader.load(frag,vert);
}

void Cursor::draw(){

	glPolygonOffset(-1.0f, -1.0f);      // Shift depth values
    glEnable(GL_POLYGON_OFFSET_LINE);

	glLineWidth(2.0f);
	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Draw lines antialiased
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_shader.use();
	m_shader.setUniform("u_mouse", getMouseX(), getMouseY());
	m_shader.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
	m_vbo->draw(&m_shader);

	// drawCross(glm::vec3(getMouseX(), getMouseY(),0.0),5.);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}