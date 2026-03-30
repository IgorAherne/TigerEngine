#include "bshape_intersections.h"
#include "../gfxmath/plane.h"
#include "../gfxmath/obb_bshape.h"
#include "../gfxmath/sphere_bshape.h"

namespace bshape_intersections{


bshape_isct_result bshapes_intersect(plane _plane, vec3 point) {
	vec3 pos_to_point = point - _plane.getCenterPos_world();


	if (vec3::Dot(pos_to_point, _plane.getNormal()) > 0) {
		return bshape_isct_result::OUTSIDE;
	}
	else if (vec3::Dot(pos_to_point, _plane.getNormal()) < 0) {
		return bshape_isct_result::INSIDE;
	}
	return bshape_isct_result::INTERSECTS;
}



bshape_isct_result bshapes_intersect(plane _plane, obb_bshape obb) {
	//TODO check if the plane's center pos has been updated to the current location
	//of the camera
	float dist_plane_to_origin = vec3::Dot(_plane.getCenterPos_world(),
											_plane.getNormal());

	//get the minimum / maximum points of the box when measured along our normal.
	//This distance takes into the account the center of aabb, and is in world space.
	vec2 min_max_along_normal = obb.project_on_normal_world(_plane.getNormal());

	//deduce the distance of the min/max points relative to the plane
	min_max_along_normal.x -= dist_plane_to_origin;
	min_max_along_normal.y -= dist_plane_to_origin;


	//see if the minumum point is ABOVE the plane:
	if (min_max_along_normal.x > 0) {
		return bshape_isct_result::OUTSIDE;
	}
	//see if maximum is under the plane as well
	else if (min_max_along_normal.y < 0) {
		return bshape_isct_result::INSIDE;
	}

	//otherwise the box is intersecting the plane (min and max points are on the 
	//different sides of the plane)
	return bshape_isct_result::INTERSECTS;
}



bshape_isct_result bshapes_intersect(plane _plane, sphere_bshape sphere) {
	
	//some vector from any point on the plane to the center of the sphere
	vec3 plane_to_sphere = sphere.getCenterPos_world() - _plane.getCenterPos_world();

	float dist_sphere_plane = vec3::Dot(plane_to_sphere,_plane.getNormal());

	if (dist_sphere_plane > sphere.getRadius()) {
		return bshape_isct_result::OUTSIDE;
	}
	if (std::abs(dist_sphere_plane) < sphere.getRadius()) {
		return bshape_isct_result::INTERSECTS;
	}
	
	return bshape_isct_result::INSIDE;
}

}//end namespace bshape_intersections;