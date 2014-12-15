#include "shader.h"
#include <stdio.h>
#include <vector>

Shader::Shader() {

}

Shader::~Shader() {
    glDeleteProgram(program);
}

bool Shader::build(const std::string& fragmentSrc, const std::string& vertexSrc) {
	vertexShader = compileShader(vertexSrc, GL_VERTEX_SHADER);

	if(!vertexShader) {
		glDeleteShader(vertexShader);
		return false;
	}

	fragmentShader = compileShader(fragmentSrc, GL_FRAGMENT_SHADER);

	if(!fragmentShader) {
		glDeleteShader(fragmentShader);
		return false;
	}

	if ( link() ) {
    		glDeleteShader(vertexShader);
    		glDeleteShader(fragmentShader);
	} else {
		return false;
	}

	return true;
}

GLuint Shader::link() {
	program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);	
	glLinkProgram(program);

	GLint isLinked;
	glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
	
	if (isLinked == GL_FALSE) {
		GLint infoLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
		if (infoLength > 1) {
			std::vector<GLchar> infoLog(infoLength);
			glGetProgramInfoLog(program, infoLength, NULL, &infoLog[0]);
			printf("Error linking program:\n%s\n", &infoLog[0]);
		}
		glDeleteProgram(program);
		return 0;
	}

	return program;
}

const GLint Shader::getAttribLocation(const std::string& attribute) {
	return glGetAttribLocation(program, attribute.c_str());
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

GLuint Shader::compileShader(const std::string& src, GLenum type) {

	GLuint shader = glCreateShader(type);
	const GLchar* source = (const GLchar*) src.c_str();

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

void Shader::detach(GLenum type) {
	bool vert = (GL_VERTEX_SHADER & type) == GL_VERTEX_SHADER;
	bool frag = (GL_FRAGMENT_SHADER & type) == GL_FRAGMENT_SHADER;
	
	if(vert) {
		glDeleteShader(vertexShader);
		glDetachShader(vertexShader, GL_VERTEX_SHADER);
	}

	if(frag) {
		glDeleteShader(fragmentShader);
		glDetachShader(fragmentShader, GL_FRAGMENT_SHADER);
	}
}

GLint Shader::getUniformLocation(const std::string& uniformName) const {
	GLint uniform = glGetUniformLocation(program, uniformName.c_str());
	return uniform;
}

void Shader::sendUniform(const std::string& name, float x) {
	if(isInUse()) {
		glUniform1f(getUniformLocation(name), x);
	}
}

void Shader::sendUniform(const std::string& name, float x, float y) {
	if(isInUse()) {
		glUniform2f(getUniformLocation(name), x, y);
	}
}

void Shader::sendUniform(const std::string& name, float x, float y, float z) {
	if(isInUse()) {
		glUniform3f(getUniformLocation(name), x, y, z);
	}
}

