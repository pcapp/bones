#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#if __APPLE__
	#include <GLUT/glut.h>
#else 
	#include <GL/glut.h>
#endif

#include <string>

class Shader {
public:
	Shader();
	bool compile(const std::string &filename, GLuint shaderType);
	bool link();
	GLuint handle() const;	
private:
	GLuint mHandle; 
};

#endif