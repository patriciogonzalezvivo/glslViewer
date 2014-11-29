#include "shader.h"

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

	if(link()) {
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
	GLint linkStatus;
	
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	
	if(!linkStatus) {
        	printInfoLog(program);
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
	GLint status;
	GLuint shader = glCreateShader(type);
	const GLchar* source = (const GLchar*) src.c_str();

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if(!status) {
        printInfoLog(shader);
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

void Shader::printInfoLog(GLuint shader) {
	GLint length = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	
	if(length > 1) {
		char* info = new char[length];
		glGetShaderInfoLog(shader, length, NULL, info);
		std::cerr << info << std::endl;
		delete[] info;
	}
}
