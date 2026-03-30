#include "obb_bshape.h"
#include "mat4.h"
#include <limits>

obb_bshape::obb_bshape(vec3 center_pos_world, mat3 scaleRot_matrix)
													: bshape(center_pos_world) {
	half_dimensions_local = vec3(0.5f, 0.5f, 0.5f);
	this->scaleRot_matrix = scaleRot_matrix;
}



obb_bshape::obb_bshape(vec3 center_pos_world, vec3 half_dimensions, 
									mat3 scaleRot_matrix) : bshape(center_pos_world){
	this->half_dimensions_local = half_dimensions;
	this->scaleRot_matrix = scaleRot_matrix;
}



obb_bshape::obb_bshape( vec3 center_pos_world, 
						const std::vector<vec3> &vert_coords_local,
						mat3 scaleRot_matrix) : bshape(center_pos_world){

	this->scaleRot_matrix = scaleRot_matrix;
	
	//find out precise half dimensions of the box:
	recompute(vert_coords_local);
}



obb_bshape::~obb_bshape() {};





//function to aid code re-usability. Not mentioned in header, only used in this .cpp
//Notice, we have 3 out variables, which might be updated.
void update_pos_vert_neg_vert(	vec3 world_corner, vec3 &neg_vert, vec3 &pos_vert,
								vec3 dir, vec2 &min_max_along_dir  ){

	float dot = vec3::Dot(world_corner, dir);
	if (dot < min_max_along_dir.x) {
		neg_vert = world_corner;
		//store for later, that this was the smallest dot so far:
		min_max_along_dir.x = dot; 
		return;
	}

	if (dot > min_max_along_dir.y) {
		pos_vert = world_corner;
		//store for later, that this was the largest dot so far:
		min_max_along_dir.y = dot;
	}
	//else - nothing changed.  Corner was not bigger/smaller than the projections of
	//pos vert or negative vert
}




//unit_length_dir is in world space.
vec2 obb_bshape::project_on_normal_world(vec3 unit_length_dir) const {
	//transform the box dimensions into world space
	vec3 half_dims_scaled_rotated = scaleRot_matrix * half_dimensions_local;
	//we will be using the correct, shifted center pos.
	//Notice, that our local offset has to be first rotated as well, since it's in
	//local space by default.
	vec3 shifted_center_world = center_pos_world 
										+ scaleRot_matrix*average_mesh_coord_local;
	
	//find the greatest (positive) vertex along the provided normal and
	//the smallest one.
	vec3 neg_vert, pos_vert;
	vec2 min_max(FLT_MAX, -FLT_MAX); //make minumum = biggest posttibe value, and
								//maximum = smallest possible value.
							//We will calculate them from two opposite directions.

	

	vec3 corner = -half_dims_scaled_rotated; //neg x, neg y, neg z
	corner += center_pos_world; //take position into account 
	//(half_dims_scaled_rotated is only based on mat3, which took scale and rotation
	//into the account, but not the position).

	//each of 8 blocks will perform a dot product
	update_pos_vert_neg_vert(corner, neg_vert, pos_vert, unit_length_dir, min_max);
	

	corner = vec3(half_dims_scaled_rotated.x, //pos x, neg y, neg z
					-half_dims_scaled_rotated.y,
					 -half_dims_scaled_rotated.z);
	corner += center_pos_world; //take position into account
	update_pos_vert_neg_vert(corner, neg_vert, pos_vert, unit_length_dir, min_max);


	corner = vec3(-half_dims_scaled_rotated.x, //neg x
					half_dims_scaled_rotated.y, //pos y
					-half_dims_scaled_rotated.z); //neg z
	corner += center_pos_world;//take position into account
	update_pos_vert_neg_vert(corner, neg_vert, pos_vert, unit_length_dir, min_max);

	
	corner = vec3(-half_dims_scaled_rotated.x, //neg x
					-half_dims_scaled_rotated.y, //neg y
					  half_dims_scaled_rotated.z); //pos z
	corner += center_pos_world;//take position into account
	update_pos_vert_neg_vert(corner, neg_vert, pos_vert, unit_length_dir, min_max);

	
	corner = vec3(half_dims_scaled_rotated.x, //pos x
					 half_dims_scaled_rotated.y, //pos y
					 -half_dims_scaled_rotated.z); //neg z
	corner += center_pos_world;//take position into account
	update_pos_vert_neg_vert(corner, neg_vert, pos_vert, unit_length_dir, min_max);

	
	corner = vec3(-half_dims_scaled_rotated.x, //neg x
					half_dims_scaled_rotated.y, //pos y
					 half_dims_scaled_rotated.z); //pos z
	corner += center_pos_world;//take position into account
	update_pos_vert_neg_vert(corner, neg_vert, pos_vert, unit_length_dir, min_max);

	
	corner = half_dims_scaled_rotated; //pos x, pos y, pos z
	corner += center_pos_world; //take position into account
	update_pos_vert_neg_vert(corner, neg_vert, pos_vert, unit_length_dir, min_max);

	
	corner = vec3(half_dims_scaled_rotated.x, //pos x
					-half_dims_scaled_rotated.y, //neg y
					  half_dims_scaled_rotated.z); //pos z
	corner += center_pos_world;//take position into account
	update_pos_vert_neg_vert(corner, neg_vert, pos_vert, unit_length_dir, min_max);


	//provide the smallest and the greatest value of vertex when projected on 
	//a normal,  all in world space.
	return min_max;
}




void obb_bshape::recompute(const std::vector<vec3> &vert_coords_local){
	//the shape's modelMatrix is unchanged. However, changes in local space have to
	//be accounted for, because a mesh's structure could have changed.

	//this function recomputes dimensions of the box, and the average_mesh_coordinate
	//in local space. Therefore no transformations are required.
	
	vec2 min_max_X(FLT_MAX, -FLT_MAX); //initial initialization should make 
	vec2 min_max_Y(FLT_MAX, -FLT_MAX); //minimum have max value, and maximum have 
	vec2 min_max_Z(FLT_MAX, -FLT_MAX); //min value.

	//try to find out the minimum and maximum amount along each dimension 
	//in a model's mesh. We progressivelly shrink the range defined with  3 values:
	//min_max_X/Y/Z  defined a on the lines above ago.
	for (int c = 0; c < vert_coords_local.size(); ++c) {
			vec4 curr_vector = vert_coords_local[c];
				
			if (min_max_X.x > curr_vector.x)
				min_max_X.x = curr_vector.x;

			else if (min_max_X.y < curr_vector.x)
				min_max_X.y = curr_vector.x;

					
				if (min_max_Y.x > curr_vector.y)
					min_max_Y.x = curr_vector.y;
					 
				else if (min_max_Y.y < curr_vector.y)
					min_max_Y.y = curr_vector.y;

					
			if (min_max_Z.x > curr_vector.z)
				min_max_Z.x = curr_vector.z;

			else if (min_max_Z.y < curr_vector.z)
				min_max_Z.y = curr_vector.z;
	}


	average_mesh_coord_local = vec3(); //reset the average before accomulating it
	average_mesh_coord_local = vec3(min_max_X.x, min_max_Y.x, min_max_Z.x) *0.5f;
	average_mesh_coord_local += vec3(min_max_X.y, min_max_Y.y, min_max_Z.y) *0.5f;
	//we multipliled each one by 0.5 to avoid float overflow if summed THEN divided. 


	//now we have average coordinate within a mesh.
	//let's determine the dimensions of the box.
	//half dimension = (maximum - minimum)/2
	half_dimensions_local.x = min_max_X.y*0.5f - min_max_X.x*0.5f; 
	half_dimensions_local.y = min_max_Y.y*0.5f - min_max_Y.x*0.5f; 
	half_dimensions_local.z = min_max_Z.y*0.5f - min_max_Z.x*0.5f; 
	//ensure that half dimensions have some minimal thickness.
	if (half_dimensions_local.x < 0.000001f)
		half_dimensions_local.x = 0.000001f;

	if(half_dimensions_local.y < 0.00001f)
		half_dimensions_local.y = 0.000001f;

	if(half_dimensions_local.z < 0.00001f)
		half_dimensions_local.z = 0.000001f;
}