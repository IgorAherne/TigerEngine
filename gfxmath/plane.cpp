#include "plane.h"



//constructs a plane which passes through the given point and has the provided
//direction
plane::plane(vec3 point_on_plane, vec3 normal) : bshape(point_on_plane) {
	this->normal = normal;
}


plane::plane(vec3 coplanar_point1, vec3 coplanar_point2, vec3 coplanar_point3):
															 bshape(coplanar_point1){
	vec3 normal = vec3::Cross(coplanar_point1 - coplanar_point2,
							  coplanar_point3 - coplanar_point2);
}

