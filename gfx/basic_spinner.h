#pragma once

#include "component.h"
#include "../gfxmath/vec3.h"

class basic_spinner : public component {
public:

	void componentUpdate(float dt) override;

	void onGameObject_AddComponent() override;

	basic_spinner(vec3 spin_speed, float orbit_point_offset);
	~basic_spinner();

	vec3 spin_speed;
	float orbit_point_offset;
};

