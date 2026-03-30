#pragma once
#include "component.h"
#include "../gfxmath/vec4.h"
#include "../gfxmath/vec2.h"
#include "input.h"


class basic_rotator : public component {
public:
	basic_rotator(key left_right[2], key down_up[2], key bckwd_fwd[2],
		float rot_speeds = 3);
	~basic_rotator();


	void componentUpdate(float dt) override;

	void onGameObject_AddComponent() override;


private:
	key negPosX[2];
	key negPosY[2];
	key negPosZ[2];

	float rot_speeds;
};

