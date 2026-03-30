#include "framework.h"
#include "input.h"

namespace framework {

	void init() {
		if (!glfwInit()) { //GLFW major initialization
			std::cout << "glfw couldn't initialize" << std::endl;
			return;
		}
		//glew can't be initialized yet, since it can only be initialized 
		//after a context was created and made current
	}


	void poll_glfw_events_for_context(GLFWwindow *currContext) {
		glfwPollEvents();
	}


	void terminate() {
		glfwTerminate();
	}

	void fetch_OGL_errors(std::string arg/*  = ""  */) {
		//std::cout << "\nChecking for API errors   arg: " << arg << "\n";
		GLenum err = GL_NO_ERROR;
		while ((err = glGetError()) != GL_NO_ERROR){
			//Process/log the error.
			switch (err){//TODO use glew's error-to-string function instead of switch
				case GL_INVALID_VALUE: {
					std::cout << "\n\nGL_INVALID_VALUE\n\n";
				}break;
				case GL_INVALID_OPERATION: {
					std::cout << "\n\nGL_INVALID_OPERATION\n\n";
				}break;
				case GL_STACK_OVERFLOW: {
					std::cout << "\n\nGL_STACK_OVERFLOW\n\n";
				}break;
				case GL_STACK_UNDERFLOW: {
					std::cout << "\n\nGL_STACK_UNDERFLOW\n\n";
				}break;
				case GL_OUT_OF_MEMORY: {
					std::cout << "\n\nGL_OUT_OF_MEMORY\n\n";
				}break;
				case GL_INVALID_FRAMEBUFFER_OPERATION: {
					std::cout << "\n\nGL_INVALID_FRAMEBUFFER_OPERATION\n\n";
				}break;
				case GL_CONTEXT_LOST: {
					std::cout << "\n\nGL_CONTEXT_LOST\n\n";
				}break;
			}
		}//end while

	}

	void fetch_OGL_errors(int arg) {
		std::cout << "\nChecking for API errors   arg: " << arg << "\n";
		GLenum err = GL_NO_ERROR;
		while ((err = glGetError()) != GL_NO_ERROR) {
			//Process/log the error.
			switch (err) {
			case GL_INVALID_VALUE: {
				std::cout << "\n\nGL_INVALID_VALUE\n\n";
			}break;
			case GL_INVALID_OPERATION: {
				std::cout << "\n\nGL_INVALID_OPERATION\n\n";
			}break;
			case GL_STACK_OVERFLOW: {
				std::cout << "\n\nGL_STACK_OVERFLOW\n\n";
			}break;
			case GL_STACK_UNDERFLOW: {
				std::cout << "\n\nGL_STACK_UNDERFLOW\n\n";
			}break;
			case GL_OUT_OF_MEMORY: {
				std::cout << "\n\nGL_OUT_OF_MEMORY\n\n";
			}break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: {
				std::cout << "\n\nGL_INVALID_FRAMEBUFFER_OPERATION\n\n";
			}break;
			case GL_CONTEXT_LOST: {
				std::cout << "\n\nGL_CONTEXT_LOST\n\n";
			}break;
			}
		}//end while

	}



	void  test_if_framebuffer_complete(){
		GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "framebuffer is not complete \n\n";
		}
	}

}