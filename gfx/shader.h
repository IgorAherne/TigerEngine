#pragma once
#include <string>
#include "GLEW\glew.h"



class shader{
public:
	//instantiate a dummy shader. To make it work, load vert and fragment source,
	//then envoke compileShader.
	shader();

	// give it paths relative to Shaders folder, for instance
	//  "MyFolderInShaders/myVertShader.glsl"
	shader(std::string vertPath, std::string fragPath, std::string geomPath="");
	
	//craetes a program including one compute shader.
	shader(std::string computePath);


	~shader();

	//returns the id of the shader program
	inline GLuint getShaderProgramID() const { return program_id; }

	//if the shader instance was created without any source,
	//you can provide the source code via this function. 
	//Don't forget to compileShader() when all the source you want was loaded - 
	//at least vertex and fragment.
	//type can be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER, etc.
	bool loadSource(std::string vert_source_url, GLuint type);


	//see if this shader can be now complied (if it contains at least
	//vertex  and fragment source)
	inline bool isReadyForCompile() {
		if ( (vso_id && fso_id) || compute_id){
			return true;
		}
		return false; //if vertex or fragment are not avalable - can't compile yet.
	}


	//check with isReadyForCompile before you call this function.
	//Returns false if the created  OpenGL shader program  could not be linked.
	bool compileShader();

	
	//is the geometry shader object attached into the shader program?
	inline bool isGeometryShaderUsed() { return gso_id != 0; }

private:
	GLuint vso_id; //vertex shader object id
	GLuint gso_id; //geometry shader object id
	GLuint fso_id; //fragment shader object id
	GLuint compute_id; //compute shader object id;

	GLuint program_id;
};



namespace shaderTools {
	//read .txt file and store the contents inside a string.
	bool extractShaderSrc(std::string path, std::string &outputSrc);

	//load shader source, compile, check for errors.
	bool CreateShaderObject(GLuint &shader_id,
							GLuint shaderType,
							const char *shaderSrc);
}