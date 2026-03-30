#include "vec3.h"
#include "vec4.h"
#include "vec2.h"

vec4 vec3::_vec4(float w/* = 0.0f*/) const {
	return vec4(x, y, z, w);
}

vec2 vec3::_vec2() const {
	return vec2(x, y);
}