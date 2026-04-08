#include "input.h"
#include "renderer.h"
#include "window.h"

//initialize the static  key input sets
std::set<size_t>  input::downed_keys;
std::set<size_t>  input::pressed_keys;
std::set<size_t>  input::released_keys;
input::double_vec2  input::cursor_pos;
input::double_vec2  input::cursor_old_pos;



bool input::getKeyDown(key _key) {
	if (downed_keys.find(_key) != downed_keys.end()) return true;
	return false;
}


bool input::getKey(key _key) {
	if (pressed_keys.find(_key) != pressed_keys.end()) return true;
	return false;
}


bool input::getKeyUp(key _key) {
	if (released_keys.find(_key) != released_keys.end()) return true;
	return false;
}


void input::updateInput(float dt){

	static bool first_call = true;
	if (first_call) {
		first_call = false;
		// seed both to the current real cursor position
		auto& r = *renderer::rendererInstances.begin();
		double cx, cy;
		glfwGetCursorPos(r.second->m_window->glfw_window, &cx, &cy);
		cursor_pos.x = cx;  cursor_pos.y = cy;
		cursor_old_pos = cursor_pos;
	}

	//update old pos. Don't do it in callback, since it will only be called if there
	//is a change in cursor position. This will mean that cursor_old_pos will never
	//be able to catch up with the current_position, forcing cursor_delta() return
	//non-zero values all the time.
	cursor_old_pos = cursor_pos; //do it BEFORE poll events.

	//check if some of the windows recieved input 
	//TODO only 1 window will be focused, so introduce the notion of "focused windwo"
	//it was already mentioned in todos somewhere here, with an actual OGL function.
	for (auto &r : renderer::rendererInstances) {
		window *w = r.second->m_window;
		glfwMakeContextCurrent(w->glfw_window);
		glfwPollEvents(); //see if there is any input for this window.
		//this will invoke primary_KeyInputCallBack, as it was registered initially.
		//The callback willl set all the keys as necessary.
	}

	//std::cout << "\n\ncursor pos: " << cursor_pos.x << ", " << cursor_pos.y <<"\n\n";
	//std::cout << "\n\delta cursor: " << getCursorDelta().x << ", " << getCursorDelta().y << "\n\n";
}


void input::primary_KeyInputCallback(GLFWwindow *_window, int key, int scancode,
															int action, int mods) {
	switch (action) {
		case GLFW_PRESS: {
			if (downed_keys.find(key) == downed_keys.end()) {
				downed_keys.insert(key);
			}
		}break;
		case GLFW_REPEAT: {
			if (pressed_keys.find(key) == pressed_keys.end()) {
				pressed_keys.insert(key);
			}
		}break;
		case GLFW_RELEASE: {
			if (released_keys.find(key) == released_keys.end()) {
				released_keys.insert(key);
			}
		}break;

	}//end switch
}



void input::primary_cursorInputCallback(GLFWwindow *, double x, double y) {
		//update current pos:
	cursor_pos.x = x; 
	cursor_pos.y = y;

	//Don't update old pos in this callback, since it will only be called if there
	//is a change in cursor position. This will mean that cursor_old_pos will never
	//be able to catch up with the current_position, forcing cursor_delta() return
	//non-zero values all the time.
	//We do it in updateInput instead
}