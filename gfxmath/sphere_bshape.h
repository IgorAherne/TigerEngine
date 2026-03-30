#pragma once
#include "bshape.h"

class sphere_bshape : public bshape{
public:
	//creates a sphere with radius 0.5 at origin coordinate
	sphere_bshape() : bshape(vec3()) {}; 

	sphere_bshape(vec3 position, float radius) : bshape(position), radius(radius) {};

	~sphere_bshape() {};

	//returns minimum and maximum value when this shape is projected on a unit-length
	//direction. This allows to see how far along any arbitrary axis the shape 
	//extends. (In world space)
	vec2 project_on_normal_world(vec3 unit_length_dir_world) const override;
	
	//TODO write explanation for sphere, or snatch if from bshape.
	//relativelly heavy computation,
	//uses information from the array of vertex positions 
	//(expects to see coordinates in local space), to tweek the dimensions of
	//the bounding shape. That way it will sit tighter arround the vertices.
	void recompute(const std::vector<vec3> &vert_coords_local) override;



	inline float getRadius(){ return radius; }

private:
	float radius;
};

