#include "sphere_bshape.h"
#include "vec2.h"
#include "vec3.h"


vec2 sphere_bshape::project_on_normal_world(vec3 unit_length_dir) const {
	
	//now we have the p-vert and the n-vert


	//provide the smallest and the greatest value of vertex when projected on 
	//a normal
	float proj_of_center = vec3::Dot(this->center_pos_world, unit_length_dir);

	return vec2(proj_of_center - this->radius,   proj_of_center + this->radius);
}


void sphere_bshape::recompute(const std::vector<vec3> &vert_coords_local) {
	//TODO
}