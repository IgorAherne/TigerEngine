#pragma once
#include "component.h"
#include "../gfxmath/vec4.h"
#include "../gfxmath/vec2.h"
#include "input.h"

class basic_navigator :	public component {
public:
	basic_navigator(key left_right[2], key down_up[2], key bckwd_fwd[2],
					vec2 mov_rot_speeds = vec2(3,3) );
	~basic_navigator();


	void componentUpdate(float dt) override;

	void onGameObject_AddComponent() override;


private:
	key left_right[2];
	key bckwd_fwd[2];
	key down_up[2];

	vec2 mov_rot_speeds;
};

