/*
Copyright (c) 2014, Patricio Gonzalez Vivo
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "cursor.h"

#include "types/shapes.h"
#include "types/rectangle.h"
#include "app.h"
#include "utils.h"

#define STRINGIFY(A) #A

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

	m_shader.load(frag,vert);
}

void Cursor::draw(){
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