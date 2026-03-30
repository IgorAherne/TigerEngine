#pragma once
#include "vec2.h"
#include "vec3.h"
#include <vector>


struct vec2;
struct vec3; 
class mesh;

//bounding shape
class bshape{

public:
	bshape(vec3 center_pos_world);
	~bshape();

	//returns minimum and maximum value when this shape is projected on a unit-length
	//direction. This allows to see how far along any arbitrary axis the shape 
	//extends. (In world space)
	virtual vec2 project_on_normal_world(vec3 unit_length_dir_world) const = 0;

	//relativelly heavy computation (depends on the bounding shape type), 
	//uses information from the array of vertex positions 
	//(expects to see coordinates in local space), to tweek the dimensions of
	//the bounding shape. That way it will sit tighter arround the vertices.
	virtual void recompute(const std::vector<vec3> &vert_coords_local) = 0;


	//returns the center poistion in the world space.
	inline vec3 getCenterPos_world() const {
		return center_pos_world;
	}

	//make sure to supply it the world position, NOT the usual transform's position,
	//which is relative to the parent. Supply  transform::position_world  instead.
	inline void setCenterPos_world(vec3 world_position) {
		center_pos_world = world_position;
	}


protected:
	vec3 center_pos_world; 
};

