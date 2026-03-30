#include "basic_navigator.h"
#include "../gfxmath/vec3.h"
#include "gameObject.h"


basic_navigator::basic_navigator(key left_right[2], key down_up[2], key bckwd_fwd[2],
								 vec2 mov_rot_speeds/* = vec2(3,3) */){

	memcpy(this->left_right, left_right, 2*sizeof(key));
	memcpy(this->down_up, down_up, 2*sizeof(key));
	memcpy(this->bckwd_fwd, bckwd_fwd, 2*sizeof(key));
	
	//setup movement/rotational multipliers:
	this->mov_rot_speeds = mov_rot_speeds;
}


basic_navigator::~basic_navigator(){

}


void basic_navigator::onGameObject_AddComponent() {

}


void basic_navigator::componentUpdate(float dt) {
	//TODO we assume that any canre accquires movement from the Input for now.
	//This has to change though, since not all cameras must respond to the input.
	//For demonstration purpouses this is fine however.
	float fwd_bkwd = 0.f;
	float left_right = 0.f;
	float up_down = 0.f;
	vec3 navigation; //all components are 0s at start

	if (input::getKey(this->left_right[0]))
		navigation.x = -1.0f;
	if (input::getKey(this->left_right[1]))
		navigation.x = 1.0f;
	if (input::getKey(this->down_up[0]))
		navigation.y = -1.0f;
	if (input::getKey(this->down_up[1]))
		navigation.y = 1.0f;
	if (input::getKey(this->bckwd_fwd[0]))
		navigation.z = -1.0f;
	if (input::getKey(this->bckwd_fwd[1]))
		navigation.z = 1.0f;


	mat4 curr_transf = m_transform->getTransf();

	//TODO notice, we are copying columns, not rows. Perhaps restructure/add
	//get_ourRight_asSeenInWorld get rightWorld_asSeenInUs?
	//TODO: should we normalize each extracted column?
	vec3 translation =    curr_transf.copyColumn(0)._vec3() * navigation.x
						+ curr_transf.copyColumn(1)._vec3() * navigation.y
						- curr_transf.copyColumn(2)._vec3() * navigation.z;

	translation.normalize();
	translation *= mov_rot_speeds.x; //use the movement multiplier
	translation *= dt ; //make time-fluctuation-independent input

	m_transform->addPosition(translation);


	//use dt and rotation speed multiplier.
	vec2 cursor_delta = input::getCursorDelta() * dt * mov_rot_speeds.y;
	m_transform->addPitchYawRoll(vec3(-cursor_delta.y, -cursor_delta.x, 0));
	
	//TODO make cursor displacement relative to window dimensions, NOT pixel-wise
	//larger resolutions will make cursor cover bigger pixels.
}
