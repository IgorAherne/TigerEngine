#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

vec3 vec2::_vec3(float z/*=0.0*/) const {
	return vec3(x, y, z);
}

vec4 vec2::_vec4(float z/*=0.0*/, float w/*=0.0*/) const {
	return vec4(x, y, z, w);
}