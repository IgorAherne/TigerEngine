#include "window.h"
#include "renderer.h"
#include <limits>


	//callback when glfw detects window was resized.
	//Updates m_width, m_height and sets glViewport
void windowResize(GLFWwindow *window, int width, int height);


	window::window(	renderer *r, std::string name,
					size_t width, size_t height, window *shared_w){

		m_renderer = r;

		//context is created in the end of init(), when we have glfw_window handle.
		m_name = name;
		m_width = width;
		m_height = height;

		//shared_w could be passed in as nullptr if no context must be shared with 
		//this window:
		if (!init(shared_w)) { 
			std::cout << "window " << name << " could not be created\n\n";
		}
		//we don't set any key callbacks here, since the renderer who created
		//this window will do it for us
	}
	




	window::~window() {
		glfwDestroyWindow(glfw_window);
		//no need to remove any callbacks for this window, as glfw will never call
		//them again for this window
	}





	void window::output() {
		glFinish(); //TODO this removes periodical jitter issue in fps. REMOVE LATER
		glfwSwapBuffers(glfw_window); //TODO this command takes 8 ms in debug mode per window. Check it

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}






	bool window::closed() const {
		return glfwWindowShouldClose(glfw_window) == 1;
	}






	//supply shared_window  as nullptr if you don't want to share its context.
	bool window::init(window *shared_window) {
		//window initialization:
		glfw_window = glfwCreateWindow(	m_width, m_height, m_name.c_str(), NULL, 
										shared_window == nullptr ? 
										NULL : shared_window->glfw_window);

		if (!glfw_window) {
			std::cout << "GLFW window failed to initialize!" << std::endl;
			return false;
		}

		//having a window pointer, we will be able to get back to the Window 
		//instance, which has it as a member variable.
		glfwSetWindowUserPointer(glfw_window, this);

		//every time a given window is called the corresponding function will be 
		//enoked
		glfwSetWindowSizeCallback(glfw_window,  windowResize);
		
		
		//going to be drawing to this current window:
		glfwMakeContextCurrent(glfw_window);
		
		
		//initialize glew after the context was created and made current.
		if (glewInit() != GLEW_OK) {
			std::cout << "GLEW couldn't initialize" << std::endl;
			return false;
		}
		glfwSwapInterval(0);

		glClearColor(1, 0.5f, 0.f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		return true;
	}



	void windowResize(GLFWwindow *glfw_window, int width, int height) {
		glViewport(0, 0, width, height); 
		window *w = static_cast<window*>(glfwGetWindowUserPointer(glfw_window));

		w->setWidth(width);
		w->setHeight(height);

		w->getMyRenderer()->onWindowResize();
	}