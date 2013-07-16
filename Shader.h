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
	Shader(const std::string &vertexShaderFilename, const std::string &fragmentShaderFilename);
	bool build();
	GLuint handle() const;
private:
	bool compile(const std::string &filename, GLuint shaderType);
	bool link();
private:
	std::string mVertexShaderFilename;
	std::string mFragmentShaderFilename;
	GLuint mHandle; 
};

#endif