#pragma once
#include <iostream>
#include <string>
#define GLEW_STATIC
#include <GLEW/glew.h>
#include <GLFW\glfw3.h>


namespace framework{

	void init();

	//clean up previous states of the input class,
	//then poll new events FOR THE CURRENT context.
	void poll_glfw_events_for_context(GLFWwindow *currContext);

	//gets called by glfw every time a key is pressed. 
	//the function will envoke other callbacks on all the objects which 
	//registered for input with Renderer calss.
	static void primary_KeyInputCallBack(GLFWwindow *_window, int key, int scancode,
															  int action, int mods);

	void terminate();

	void fetch_OGL_errors(std::string arg = "");
	void fetch_OGL_errors(int arg);

	void test_if_framebuffer_complete();
};

