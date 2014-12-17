#include "shader.h"

#include <stdio.h>
#include <vector>
#include <iostream>

Shader::Shader():m_program(0),m_fragmentShader(0),m_vertexShader(0){

}

Shader::~Shader() {
    glDeleteProgram(m_program);
}

bool Shader::load(const std::string& _fragmentSrc, const std::string& _vertexSrc) {
	m_vertexShader = compileShader(_vertexSrc, GL_VERTEX_SHADER);

	if(!m_vertexShader) {
		glDeleteShader(m_vertexShader);
		std::cout << "Error loading vertex shader src" << std::endl;
		return false;
	}

	m_fragmentShader = compileShader(_fragmentSrc, GL_FRAGMENT_SHADER);

	if(!m_fragmentShader) {
		std::cout << "Error loading fragment shader src" << std::endl;
		glDeleteShader(m_fragmentShader);
		return false;
	}

	m_program = glCreateProgram();

	glAttachShader(m_program, m_vertexShader);
	glAttachShader(m_program, m_fragmentShader);	
	glLinkProgram(m_program);

	GLint isLinked;
	glGetProgramiv(m_program, GL_LINK_STATUS, &isLinked);
	
	if (isLinked == GL_FALSE) {
		GLint infoLength = 0;
		glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &infoLength);
		if (infoLength > 1) {
			std::vector<GLchar> infoLog(infoLength);
			glGetProgramInfoLog(m_program, infoLength, NULL, &infoLog[0]);
			printf("Error linking m_program:\n%s\n", &infoLog[0]);
		}
		glDeleteProgram(m_program);
		return false;
	} else {
		// glDeleteShader(m_vertexShader);
    	// glDeleteShader(m_fragmentShader);
    	return true;
	}
}

const GLint Shader::getAttribLocation(const std::string& _attribute) {
	return glGetAttribLocation(m_program, _attribute.c_str());
}

void Shader::use() const {
	if(!isInUse()) {
		glUseProgram(getProgram());
	}
}

bool Shader::isInUse() const {
	GLint currentProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	return (getProgram() == (GLuint)currentProgram);
}

GLuint Shader::compileShader(const std::string& _src, GLenum _type) {

	GLuint shader = glCreateShader(_type);
	const GLchar* source = (const GLchar*) _src.c_str();

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	
	GLint isCompiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled == GL_FALSE) {
		GLint infoLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
		if (infoLength > 1) {
			std::vector<GLchar> infoLog(infoLength);
			glGetShaderInfoLog(shader, infoLength, NULL, &infoLog[0]);
		    printf("Error compiling shader:\n%s\n", &infoLog[0]);
		}
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

void Shader::detach(GLenum _type) {
	bool vert = (GL_VERTEX_SHADER & _type) == GL_VERTEX_SHADER;
	bool frag = (GL_FRAGMENT_SHADER & _type) == GL_FRAGMENT_SHADER;
	
	if(vert) {
		glDeleteShader(m_vertexShader);
		glDetachShader(m_vertexShader, GL_VERTEX_SHADER);
	}

	if(frag) {
		glDeleteShader(m_fragmentShader);
		glDetachShader(m_fragmentShader, GL_FRAGMENT_SHADER);
	}
}

GLint Shader::getUniformLocation(const std::string& _uniformName) const {
	GLint loc = glGetUniformLocation(m_program, _uniformName.c_str());
	if(loc == -1){
		// std::cerr << "Uniform " << _uniformName << " not found" << std::endl;
	}
	return loc;
}

void Shader::sendUniform(const std::string& _name, float _x) {
	if(isInUse()) {
		glUniform1f(getUniformLocation(_name), _x);
		// std::cout << "Uniform " << _name << ": float(" << _x << ")" << std::endl;
	}
}

void Shader::sendUniform(const std::string& _name, float _x, float _y) {
	if(isInUse()) {
		glUniform2f(getUniformLocation(_name), _x, _y);
		// std::cout << "Uniform " << _name << ": vec2(" << _x << "," << _y << ")" << std::endl;
	}
}

void Shader::sendUniform(const std::string& _name, float _x, float _y, float _z) {
	if(isInUse()) {
		glUniform3f(getUniformLocation(_name), _x, _y, _z);
		// std::cout << "Uniform " << _name << ": vec3(" << _x << "," << _y << "," << _z <<")" << std::endl;
	}
}

void Shader::sendUniform(const std::string& _name, const Texture* _tex, unsigned int _texLoc){
	if(isInUse()) {
		// glUniform1i(getUniformLocation(_name), _texLoc);
		glActiveTexture(GL_TEXTURE0 + _texLoc);
		glBindTexture(GL_TEXTURE_2D, _tex->getId());
		glUniform1i(getUniformLocation(_name), _texLoc);
	}
}