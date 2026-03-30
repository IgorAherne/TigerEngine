#include "basic_rotator.h"
#include "../gfxmath/vec3.h"
#include "transform.h"

basic_rotator::basic_rotator(key left_right[2], key down_up[2], key bckwd_fwd[2],
							 float rot_speeds/* = 3*/) {
	memcpy(this->negPosX, left_right, 2 * sizeof(key));
	memcpy(this->negPosY, down_up, 2 * sizeof(key));
	memcpy(this->negPosZ, bckwd_fwd, 2 * sizeof(key));

	//setup movement/rotational multipliers:
	this->rot_speeds = rot_speeds;
}


basic_rotator::~basic_rotator() {

}


void basic_rotator::onGameObject_AddComponent() {

}



void basic_rotator::componentUpdate(float dt) {

	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	vec3 yawPitchRoll; //all components are 0s at start

	if (input::getKey(this->negPosX[0]))
		yawPitchRoll.x = -1.0f;
	if (input::getKey(this->negPosX[1]))
		yawPitchRoll.x = 1.0f;
	if (input::getKey(this->negPosY[0]))
		yawPitchRoll.y = -1.0f;
	if (input::getKey(this->negPosY[1]))
		yawPitchRoll.y = 1.0f;
	if (input::getKey(this->negPosZ[0]))
		yawPitchRoll.z = -1.0f;
	if (input::getKey(this->negPosZ[1]))
		yawPitchRoll.z = 1.0f;


	mat4 curr_transf = m_transform->getTransf();

	//TODO notice, we are copying columns, not rows. Perhaps restructure/add
	//get_ourRight_asSeenInWorld get rightWorld_asSeenInUs?
	//TODO: should we normalize each extracted column?

	yawPitchRoll *= rot_speeds; //use the rotation multiplier
	yawPitchRoll *= dt; //make time-fluctuation-independent input

	m_transform->addPitchYawRoll(yawPitchRoll);

}