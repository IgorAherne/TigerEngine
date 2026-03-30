#include "basic_spinner.h"
#include "transform.h"


void basic_spinner::componentUpdate(float dt) {
	vec3 pos_world = m_transform->getPosition();
	m_transform->addPitchYawRoll(spin_speed * dt);
}


basic_spinner::basic_spinner(vec3 spin_speed, float orbit_point_offset) {
	this->spin_speed = spin_speed;
	this->orbit_point_offset = orbit_point_offset;
}


void basic_spinner::onGameObject_AddComponent() {
}


basic_spinner::~basic_spinner() {
}
