#include "shader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace shaderTools {

	bool extractShaderSrc(std::string path, std::string &outputSrc) {
		std::cout << "\n\nassembling the shader from " + path + "\n";

		std::ifstream in;
		in.open(path);

		if (in.fail()) {
			std::cout << "\n\nshader wrong path  or  name\n\n";
			system("pause");
			return false;
		}

		std::stringstream src;
		src << in.rdbuf();
		outputSrc = src.str();
		in.close();
		return true;
	}



	//doesn't have signature in the header file (hidden)
	bool shaderCompileStatus(GLuint &shader_id) {
		//check the compile status
		GLint compiled;
		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);

		if (compiled) { return true; }
		//retrieve the compiler messages when compilation fails
		GLint infoLen = 0;
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1) {
			std::vector<GLchar> errorLog(infoLen);

			glGetShaderInfoLog(shader_id,  infoLen, &infoLen, &errorLog[0]);
			std::cout << "\n\nError compiling the shader!\n";
			for (char c : errorLog) {
				std::cout << c;
			}
			std::cout << "\n\n";

			glDeleteShader(shader_id);
			shader_id = 0;
			system("pause");
		}
		return false;
	}


	//doesn't have signature in the header file (hidden)
	bool programLinkStatus(GLuint &program_id) {
		GLint linked;
		glGetProgramiv(program_id, GL_LINK_STATUS, &linked);
  
		if (!linked) {
			GLint infoLen = 0;
			glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen > 1) {
				std::vector<GLchar> errorLog(infoLen);

				glGetProgramInfoLog(program_id, infoLen, NULL, &errorLog[0]);
				std::cout << "\nerror linking the program!\n";
				for (char c : errorLog) {
					std::cout << c;
				}
				std::cout << "\n\n";

				glDeleteProgram(program_id);
				program_id = 0;
				system("pause");
			}
			return false;
		}

		glGetProgramiv(program_id, GL_VALIDATE_STATUS, &linked);

		if (!linked) {
			GLint infoLen = 0;
			glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen > 1) {
				std::vector<GLchar> errorLog(infoLen);

				glGetProgramInfoLog(program_id, infoLen, NULL, &errorLog[0]);
				std::cout << "\nerror validating the program!\n";
				for (char c : errorLog) {
					std::cout << c;
				}
				std::cout << "\n\n";

				glDeleteProgram(program_id);
				program_id = 0;
				system("pause");
			}
			return false;
		}

		return true;
	}


	bool CreateShaderObject(GLuint &shader_id, GLuint shaderType, const char *shaderSrc) {

		shader_id = glCreateShader(shaderType);
		glShaderSource(shader_id, 1, &shaderSrc, NULL); //load up the shader source
		glCompileShader(shader_id);
		return shaderCompileStatus(shader_id); //check for errors during compilation.
	}
}//end shaderTools namespace


shader::shader() {
	vso_id = gso_id = fso_id = compute_id = 0;
	program_id = 0;
	//this shader cannot be used yet, first provide it with vertex + fragment 
	//source code. Then compile it.
}


shader::shader(std::string vertPath, std::string fragPath, std::string geomPath/*=""*/){
	using namespace shaderTools;

	vso_id = gso_id = fso_id = compute_id = 0;
	program_id = 0;
	char *src = nullptr;

	loadSource(vertPath, GL_VERTEX_SHADER);
	loadSource(fragPath, GL_FRAGMENT_SHADER);
	if (geomPath != "") {
		loadSource(geomPath, GL_GEOMETRY_SHADER);
	}

	if (isReadyForCompile()) {
		compileShader();
	}//end if vso and fso are avalable
}


//create a compute shader program:
shader::shader(std::string computePath) {
	using namespace shaderTools;

	compute_id = 0;
	program_id = 0;

	char *src = nullptr;
	loadSource(computePath, GL_COMPUTE_SHADER);

	if (compute_id != 0) {
		compileShader();
	}
}



shader::~shader(){
	glDetachShader(program_id, vso_id);
	glDeleteShader(vso_id);
	glDetachShader(program_id, fso_id);
	glDeleteShader(fso_id);

	if (gso_id) {
		glDetachShader(program_id, gso_id);
		glDeleteShader(gso_id);
	}
	glDeleteProgram(program_id);
}



bool shader::loadSource(std::string source_url, GLuint type) {
	using namespace shaderTools;

	std::string src = "";
	//store source in a string
	if (!extractShaderSrc(source_url, src))
		return false;

	switch (type) {
		case GL_VERTEX_SHADER: {
			//create shader object and populate vso_id
			return CreateShaderObject(vso_id, type, src.c_str());
		}break;
		case GL_FRAGMENT_SHADER: {
			return CreateShaderObject(fso_id, type, src.c_str());
		}break;
		case GL_GEOMETRY_SHADER: {
			return CreateShaderObject(gso_id, type, src.c_str());
		}break;
		case GL_COMPUTE_SHADER: {
			return CreateShaderObject(compute_id, type, src.c_str());
		}break;
	}
	//didn't return so far, means the type was wrong.
	return false;
}


bool shader::compileShader() {
	using namespace shaderTools;

	if (program_id != 0) { //get rid of the old program_id
		glDeleteProgram(program_id); //TODO perhaps just detach all shader objects from it.
	} //that way, we won't have to re-create shader program like this every time.
	program_id = glCreateProgram();

	if (compute_id) {
		glAttachShader(program_id, compute_id);
	}
	else {
		glAttachShader(program_id, vso_id);
		glAttachShader(program_id, fso_id);

		if (gso_id)
			glAttachShader(program_id, gso_id);
	}

	glLinkProgram(program_id);
	
	return programLinkStatus(program_id);
}