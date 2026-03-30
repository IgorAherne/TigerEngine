#pragma once
#include <GLEW\glew.h>
#include <GLFW\glfw3.h>
#include "framework.h"
#include <set>
#include "../gfxmath/vec2.h"
#include <limits>
#include "window.h"

enum key{
KEY_UNKNOWN       = -1,
KEY_SPACE         = 32,
KEY_APOSTROPHE    = 39,
KEY_COMMA         = 44,
KEY_MINUS         = 45,
KEY_PERIOD        = 46,
KEY_SLASH         = 47,
KEY_0             = 48,
KEY_1             = 49,
KEY_2             = 50,
KEY_3             = 51,
KEY_4             = 52,
KEY_5             = 53,
KEY_6             = 54,
KEY_7             = 55,
KEY_8             = 56,
KEY_9             = 57,
KEY_SEMICOLON     = 59,
KEY_EQUAL         = 61,
KEY_A             = 65,
KEY_B             = 66,
KEY_C             = 67,
KEY_D             = 68,
KEY_E             = 69,
KEY_F             = 70,
KEY_G             = 71,
KEY_H             = 72,
KEY_I             = 73,
KEY_J             = 74,
KEY_K             = 75,
KEY_L             = 76,
KEY_M             = 77,
KEY_N             = 78,
KEY_O             = 79,
KEY_P             = 80,
KEY_Q             = 81,
KEY_R             = 82,
KEY_S             = 83,
KEY_T             = 84,
KEY_U             = 85,
KEY_V             = 86,
KEY_W             = 87,
KEY_X             = 88,
KEY_Y             = 89,
KEY_Z             = 90,
KEY_LEFT_BRACKET  = 91,
KEY_BACKSLASH     = 92,
KEY_RIGHT_BRACKET = 93,
KEY_GRAVE_ACCENT  = 96,
KEY_WORLD_1       = 161,
KEY_WORLD_2       = 162,

KEY_ESCAPE        = 256,
KEY_ENTER         = 257,
KEY_TAB           = 258,
KEY_BACKSPACE     = 259,
KEY_INSERT        = 260,
KEY_DELETE        = 261,
KEY_RIGHT         = 262,
KEY_LEFT          = 263,
KEY_DOWN          = 264,
KEY_UP            = 265,
KEY_PAGE_UP       = 266,
KEY_PAGE_DOWN     = 267,
KEY_HOME          = 268,
KEY_END           = 269,
KEY_CAPS_LOCK     = 280,
KEY_SCROLL_LOCK   = 281,
KEY_NUM_LOCK      = 282,
KEY_PRINT_SCREEN  = 283,
KEY_PAUSE         = 284,
KEY_F1            = 290,
KEY_F2            = 291,
KEY_F3            = 292,
KEY_F4            = 293,
KEY_F5            = 294,
KEY_F6            = 295,
KEY_F7            = 296,
KEY_F8            = 297,
KEY_F9            = 298,
KEY_F10           = 299,
KEY_F11           = 300,
KEY_F12           = 301,
KEY_F13           = 302,
KEY_F14           = 303,
KEY_F15           = 304,
KEY_F16           = 305,
KEY_F17           = 306,
KEY_F18           = 307,
KEY_F19           = 308,
KEY_F20           = 309,
KEY_F21           = 310,
KEY_F22           = 311,
KEY_F23           = 312,
KEY_F24           = 313,
KEY_F25           = 314,
KEY_KP_0          = 320,
KEY_KP_1          = 321,
KEY_KP_2          = 322,
KEY_KP_3          = 323,
KEY_KP_4          = 324,
KEY_KP_5          = 325,
KEY_KP_6          = 326,
KEY_KP_7          = 327,
KEY_KP_8          = 328,
KEY_KP_9          = 329,
KEY_KP_DECIMAL    = 330,
KEY_KP_DIVIDE     = 331,
KEY_KP_MULTIPLY   = 332,
KEY_KP_SUBTRACT   = 333,
KEY_KP_ADD        = 334,
KEY_KP_ENTER      = 335,
KEY_KP_EQUAL      = 336,
KEY_LEFT_SHIFT    = 340,
KEY_LEFT_CONTROL  = 341,
KEY_LEFT_ALT      = 342,
KEY_LEFT_SUPER    = 343,
KEY_RIGHT_SHIFT   = 344,
KEY_RIGHT_CONTROL = 345,
KEY_RIGHT_ALT     = 346,
KEY_RIGHT_SUPER   = 347,
KEY_MENU          = 348,
KEY_LAST          
};


class input sealed{
	


	//friend function from framework namespace.
	friend void framework::primary_KeyInputCallBack(GLFWwindow*, int, int, int, int);
	friend class renderer;

public:
	inline static std::set<size_t> getAllKeysDown() { 
		return downed_keys; //comletely value-type, don't worry, we don't return ref.
	};
	static std::set<size_t> getAllKeysPressed() {
		return pressed_keys; //comletely value-type 
	}
	static std::set<size_t> getAllKeysUp(){
		return released_keys;//comletely value-type
	}


	//find out if the given key is being pressed down for the first time or not.
	static bool getKeyDown(key _key);

	//find out if the given key is being pressed over some period of time or not.
	static bool getKey(key _key);

	//find out if the given key is released or not.
	static bool getKeyUp(key _key);


	//separate from y, to avoid vector's low-precision floats.
	static double getCursorPos_x() {
		return cursor_pos.x;
	}
	//separate from x, to avoid vector's low-precision floats.
	static  double getCursorPos_y() {
		return cursor_pos.y;
	}

	
	//returns NON-unit length change in position from previous frame
	static vec2 getCursorDelta() {
		return vec2( cursor_pos.x - cursor_old_pos.x, 
					 cursor_pos.y - cursor_old_pos.y );
	}


	//has to be called at the end of the main loop.
	//clears the key inputs (released_keys, pressed_keys and downed_keys sets).
	//This effectively wipes out all data produced by glfwPollEvents()  of
	//ALL the contributed contexts.
	//
	//TODO: press forward, resize window, release forward, release resize and 
	//the camera will move forward forever. Solve this issue.
	inline static void flush_keys() {
		//see if some keys are downed AND are not in released yet.
		//place such keys into PRESSED keys.
		for (int key : downed_keys) {
			if (released_keys.find(key) == released_keys.end()) {
				pressed_keys.insert(key);
			}
		}
		downed_keys.clear(); //clear the downed keys.
		
		//erase any key from the pressed keys  which is mentioned in released.
		for (int key : released_keys) {
			pressed_keys.erase(key);
		}
		released_keys.clear(); //clear the released keys.
		//as you acn see, pressed keys are cleared by manually removing elements.
		//This allows for a lightweight solution towards the latency present in
		//GLEW's input. (GLFW_PRESS, few seconds, then continious GLFW_REPEAT)
	}

	//see if there is any input.
	static void updateInput(float dt);


private:
	//don't ever make the constructor or destructor public!
	input() {
		
		
	};

	~input() {};


	//envoked by GLFW. 
	//was registered with GLFW as the key callback (every window selects this same
	//function as a key callback, upon that window's creation)
	static void primary_KeyInputCallback(GLFWwindow *_window, int key, int scancode,
															  int action, int mods);


	//enboked by GLFW
	//was registered with GLFW as the cursor position callback (every window selects
	//this same function as a cursor position callback, upon that window's creation)
	static void primary_cursorInputCallback(GLFWwindow *, double x, double y);



	struct double_vec2 {
		double_vec2(double _x = 0.0, double _y = 0.0) : x(_x), y(_y) {}
		double x, y;
	};

	//latest position of the cursor in the window:
	//screen-position (horizontal, vertical)
	static double_vec2 cursor_pos;
	static double_vec2 cursor_old_pos;

	//keys that just went down
	static std::set<size_t> downed_keys;

	//keys that were pressed for some period of time
	static std::set<size_t> pressed_keys;

	//any key that GLFW maked as being "unpressed" or "released", has to go in this
	//set. 
	static std::set<size_t> released_keys;
	
	

};

