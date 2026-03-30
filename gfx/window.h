#pragma once
#include <iostream>
#include "../gfx/framework.h"
#include <string>



class window {
	friend class renderer;
	friend class input;

	public:
		void output();
		bool closed()const; //is GLFW window closing

		inline int getWidth() const { return m_width; }
		inline int getHeight() const { return m_height; }
		inline void setWidth(int width) { m_width = width; }
		inline void setHeight(int height) { m_height = height; }
		inline renderer *getMyRenderer() { return m_renderer;  }




	private:
		//constructor / destructor are private, since only renderer instance
		//can create its window.
		//Allows for sharing a context of some other window (last argument),
		//supply nullptr if don't want to share
		window(renderer *r, std::string name, size_t width, size_t height, 
																 window* shared_w);
		~window();
		//called in constructor; Initializes m_window.
		//Supply shared_window  as nullptr if you don't want to share that window's
		//context.
		bool init(window *shared_window); 




	private:
		std::string m_name;
		int m_width, m_height;
		GLFWwindow* glfw_window;
		bool m_closed; //is this window closed (in GLFW terms)
		renderer *m_renderer; //allows backwards communication to our "boss"
};//end Window
