#pragma once
#include "../gfxmath/vec3.h"

class plane;
class obb_bshape;
class sphere_bshape;

//enum used as a result of intersection test between 2 bounding shapes.
enum bshape_isct_result {
	OUTSIDE,  
	INTERSECTS,
	INSIDE
};


//namespace used to test if a bounding shape contains OR intersects the incomming 
//bounding shape
namespace bshape_intersections {
	//see if a given point lies above the plane, below or is coplanar to it.
	bshape_isct_result bshapes_intersect(plane _plane, vec3 point);

	//see if provided axis-aligned bounding box shape  sits above the plane, below, 
	//or intersects it.
	bshape_isct_result bshapes_intersect(plane _plane, obb_bshape aabb);

	//see if provided sphere bounding shape  sits above the plane, below, 
	//or intersects it.
	bshape_isct_result bshapes_intersect(plane _plane, sphere_bshape sphere);

};