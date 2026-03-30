#include "vec3.h"
#include "vec4.h"

vec3 vec4::_vec3() const {
	return vec3(x, y, z);
}

vec4::vec4(vec3 v3, float value/* = 0.0f*/) {
	x = v3.x;
	y = v3.y;
	z = v3.z;
	w = value;
}

vec4::vec4(float value, vec3 v3) {
	x = value;
	y = v3.x;
	z = v3.y;
	w = v3.z;
}