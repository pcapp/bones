#include "Shader.h"

#include <iostream>
#include <fstream>
#include <sstream>

using std::ifstream;
using std::stringstream;
using std::cout;
using std::endl;

Shader::Shader(const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename)
	: mVertexShaderFilename(vertexShaderFilename), mFragmentShaderFilename(fragmentShaderFilename) 
{
	
}

bool Shader::build() {
	mHandle = glCreateProgram();

	if(!compile(mVertexShaderFilename, GL_VERTEX_SHADER)) {
		return false;
	}

	if(!compile(mFragmentShaderFilename, GL_FRAGMENT_SHADER)) {
		return false;
	}

	if(!link()) {
		return false;
	}

	return true;	
}

bool Shader::compile(const std::string &filename, GLuint shaderType) {
	ifstream in(filename);

	if(!in) {
		cout << "Could not open " << filename << endl;
		return false;
	}
	
	stringstream inStream;
	inStream << in.rdbuf() << '\0';

	int bufSize = inStream.str().length();
	char *buffer = new char[bufSize];
	memcpy(buffer, inStream.str().c_str(), bufSize);
	
	const GLchar *sources[] = {buffer};
	
	GLuint hShader = glCreateShader(shaderType);
	glShaderSource(hShader, 1, sources, NULL);
	glCompileShader(hShader);

	int compileStatus;
	glGetShaderiv(hShader, GL_COMPILE_STATUS, &compileStatus);

	if(GL_FALSE == compileStatus) {
		cout << "Could not compile the shader" << endl;
		GLint logLength;
		glGetShaderiv(hShader, GL_INFO_LOG_LENGTH, &logLength);
		if(logLength > 0) {
			GLsizei numBytesRead;
			char *log = new char[logLength];
			glGetShaderInfoLog(hShader, logLength, &numBytesRead, log);
			cout << log << endl;
			delete [] log;
		}
		delete[] buffer;
		return false;
	}

	glAttachShader(mHandle, hShader);
	
	delete[] buffer;
	return true;
}

bool Shader::link() {
	GLint linkStatus;
	glLinkProgram(mHandle);
	glGetProgramiv(mHandle, GL_LINK_STATUS, &linkStatus);
	if(GL_FALSE == linkStatus) {
		cout << "Error linking the shader program." << endl;
		GLint logLength;
		glGetProgramiv(mHandle, GL_INFO_LOG_LENGTH, &logLength);

		if(logLength > 0) {
			GLsizei bytesRead;
			char *log = new char[logLength];
			glGetProgramInfoLog(mHandle, logLength, &bytesRead, log);	
			cout << log << endl;	
			delete []log;
		}

		return false;
	}

	return true;
}

GLuint Shader::handle() const {
	return mHandle;
}