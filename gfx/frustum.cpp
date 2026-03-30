#include "frustum.h"
#include "scene.h"
#include "gameObject.h"
#include "transform.h"
#include "mesh.h"
#include <algorithm>//for sorting
#include <functional> //for sorting predicate (tells to sort in greater/less order)
#include "../gfxmath/bshape_intersections.h"
#include "../gfxmath/obb_bshape.h"



frustum::frustum(mat4 viewProjMatrix){
	reShape(viewProjMatrix); //construct 6 planes of the frustum. 
}


frustum::~frustum(){
}



void frustum::frustumCapture(const vec3 &frustumPos, bool ignoreInvis){
	scene* curr_scene = scene::getCurrentScene();
	gameObject *root = curr_scene->getRootGameObject();

	clear_FrustumContents();
	captureChildGameObjects(frustumPos, root);
	
	sort_FrustumContents(ignoreInvis); //sort all the objects 
}


void frustum::captureChildGameObjects(vec3 frustumPos_world, gameObject *parent) {


	auto children = parent->getAllChildren();

	for (gameObject *child : children) {
		//see if this particular child is inside of the frustum.
		//if yes - adds it to the correct vector.
		capture_gameObject(frustumPos_world, child);

		//test if the its children, grandchildren, etc.  fit into the frustum:
		captureChildGameObjects(frustumPos_world, child);
	}//end for every child of parent.
}



void frustum::capture_gameObject(const vec3 &frustumPos_world, gameObject *go) {
	using namespace bshape_intersections;
	//TODO don't asume its obb_bshape
	obb_bshape *incoming_shape = dynamic_cast<obb_bshape*>(go->getBoundingShape());

	//immediately return, since this gameObject doesn't have ANY 
	//bounding shape. It cannot be included in this frustum.
	if (incoming_shape == nullptr)
		return;


	bool contained = true;
	for (plane p : planes) {
		//TODO determine which type of bshape the incoming object is.
		//right now we cast it only into aabb:

		if (bshapes_intersect(p, *incoming_shape) == bshape_isct_result::OUTSIDE) {
			//contained = false; //if at least 1 plane doesn't contain this object
			//break; //break, since it's not inside/intersecting of this frustum
		}
	}

	if (contained)
		store_gameObject(frustumPos_world, go);
}



void frustum::store_gameObject(	const vec3 &frustumPos_world, 
								gameObject *contained_go){


	float sqr_dist = (contained_go->getTransform()->getPos_world() -frustumPos_world)
																	  .sqr_length();
	//pack the gameObject into struct, so it can be sorted with other
	//captured objects.
	distanceSortedObj capturedObj(sqr_dist, contained_go);
	//TODO: test performance of getComponent.
	//perhaps use direct pointers to material and mesh, like
	//gameObject::m_material or m_mesh.
	mesh *_mesh = contained_go->getComponent<mesh>();

	if (_mesh == nullptr) { //TODO include material component here too.
		frustum_invis_Contents.push_back(capturedObj);
	}
	else { //object has both of those components, check if transparent
			//if (!_mat.isTransparent) { //or not transparent.
		frustum_o_Contents.push_back(capturedObj);
		//}
		//else { //TODO uncomment when material is  ready
		//	frustum_t_Contents.push_back(capturedObj);
		//}
	}
}



void frustum::reShape(plane planes[6]) {//copy  left,right,  bottom,top,  near,far.
	for (int p = 0; p < 6; ++p) { 
		this->planes[p] = planes[p];
	}
}

//note that viewProjMatrix must be based of the WORLD position, not the one relative
void frustum::reShape(mat4 viewProjMatrix) {
	vec4 w_axis = viewProjMatrix.copyRow(3);

	for (int p = 0; p < 6; ++p) { //construct  left,right,  bottom,top,  near, far 
		planes[p] = constructPlane(w_axis, viewProjMatrix, p);
	}
}


plane frustum::constructPlane(vec4 w_axis, mat4 viewProjMat, size_t plane_index) {
	int wanted_row = plane_index / 2;

	//sign will be 1 for left, bot, near  planes.     
	//otherwise will be -1.  Alters sign from addition to subtraction when 
	//calculating normal and point on the plane
	float sign = plane_index % 2 == 0 ? 1 : -1;

	vec4 searched_axis = viewProjMat.copyRow(wanted_row);

	//example: normal for right plane is  w_axis - x_axis.
	//It's not unit-length yet.
	vec3 plane_norm = w_axis._vec3() +   searched_axis._vec3() * sign;

	//example: distance to right plane is  w_dist - x_distance
	//distance is stored as the last (4th component of any row)
	float plane_dist = w_axis.w +    searched_axis.w * sign;


	float norm_length = plane_norm.length();
	plane_norm /= norm_length; //normalize normal.
	plane_dist /= norm_length; //distance has to have this division too.

	vec3 point_on_plane = plane_norm * plane_dist;

	return plane(point_on_plane, plane_norm);
}



void frustum::sort_FrustumContents(bool ignoreInvis) {
	//TODO: check that std::less is correct predicate, and we are indeed sorting in
	//the correct order:

	//sort in ASCENDING order (closest objects will be first)
	std::sort(frustum_o_Contents.begin(), frustum_o_Contents.end(), 
												std::less<distanceSortedObj>());

	//sort in DESCENDING order (closest objects will be on the final indices)
	std::sort(frustum_t_Contents.begin(), frustum_t_Contents.end(),
												std::greater<distanceSortedObj>());

	//if we must sort invisible vector aswell, do so:
	if (! ignoreInvis) {
		//sort in ASCENDING order (closest objects will be first)
		std::sort(frustum_invis_Contents.begin(), frustum_invis_Contents.end(),
			std::less<distanceSortedObj>());
	}
}


