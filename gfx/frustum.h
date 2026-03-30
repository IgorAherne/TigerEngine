#pragma once
#include <vector>
#include "../gfxmath/vec3.h"
#include "../gfxmath/mat4.h"
#include "../gfxmath/plane.h"

class gameObject;

//struct which allows to store a squared distance, and the actual object.
//such structs can then be sorted by distance.
struct distanceSortedObj {
	distanceSortedObj(float sqr_dist_to_camera, gameObject *_gameObject) {
		this->sqr_dist_to_camera = sqr_dist_to_camera;
		this->_gameObject = _gameObject;
	};
	~distanceSortedObj() {};

	//will be used by sorting algorythms.
	//will sort a datastructure in the ASCENDING order
	bool operator<(const distanceSortedObj rhs) const {
		return  this->sqr_dist_to_camera < rhs.sqr_dist_to_camera ? true : false;
	}
	//will sort a datastructure in the DESCENDING order
	bool operator>(const distanceSortedObj rhs) const {
		return  this->sqr_dist_to_camera < rhs.sqr_dist_to_camera ? true : false;
	}
	

	float sqr_dist_to_camera; //squared distance to camera (to be able to sort)
	gameObject *_gameObject; //pointer to the actual gameObject.
};



//frustum is typically a property of a camera.
//looks at the contents of the scene, then picks the objects which fall inside of 
//the "field of view", frustum in other words.
class frustum{
	friend class camera; //only camera can create/delete frustums

public:
	//find out which of the game objects in the scene are inside of the frustum area.
	//Resulting contents are packed into 2 vectors of objects: 1 for opaque,
	//1 for transparent. A 3rd vector is used for "invisible" gameObjects - those
	//that don't have a mesh component or material components. 
	//
	//MAKE SURE to supply frustum position in the WORLD space, NOT a position
	//relative to parent transform.
	void frustumCapture(const vec3 &frustumPos_world, bool ignoreInvisible); //TODO reserve vector member-varible space by the 
				//area of octree the camera is in, and its population numbers etc.


	//Returns vector of opaque objects IN ASCENDING order
	//(closest objects will be first)
	inline std::vector<distanceSortedObj> &get_opaqueContents_asc() {
		return frustum_o_Contents;
	}

	//Returns Transparent objects in DESCENDING oreder 
	// (closest objects will be on the final indices)
	std::vector<distanceSortedObj> &get_transparContents_desc() {
		return frustum_t_Contents;
	}


	//extract 6 planes from the VIEW-PROJECTION matrix and change the volume of
	//frustum.
	//note that viewProjMatrix must be based of the WORLD position,
	//not the one relative to a parent transform.
	void reShape(mat4 viewProjMatrix);

	//use the 6 provided planes to re-define this frustum.
	//Order is   left, right,  bottom, top,  near, far.
	void reShape(plane planes[6] );

private:
	//private constructor/destructor
	frustum(mat4 viewProjMatrix);  //only camera can create/delete frustums
	~frustum();


	void sort_FrustumContents(bool ignoreInvis);//TODO
	
	//clears the lists of all the gameObjects which made it into the frustum
	//Opaque, transparent and the ones without material (invisible ones)
	void clear_FrustumContents(){
		frustum_o_Contents.clear();
		frustum_t_Contents.clear();
		frustum_invis_Contents.clear();
	}; 
	
	//recursive function, used only by frusumCapture()
	//MAKE SURE to supply frustum position in the WORLD space, NOT a position
	//relative to parent transform.
	void captureChildGameObjects(vec3 frustumPos_world, gameObject *parent);


	//test if a particular gameObject fits into the frustum. 
	//Envokes store_gameObject() if yes.
	//MAKE SURE to supply frustum position in the WORLD space, NOT a position
	//relative to parent transform.
	void capture_gameObject(const vec3 &frustumPos_world, gameObject *go);

	//Adds gameObject to the correct vector.
	//MAKE SURE to supply frustum position in the WORLD space, NOT a position
	//relative to parent transform.
	void store_gameObject(const vec3 &frustumPos_world, gameObject *contained_gameObj);


	//given the VIEW-PROJECTION matrix, its w_axis (last row) and the index of plane,
	//construct one of the 6 planes. Order is   left, right, bottom, top, near, far.
	plane constructPlane(vec4 w_axis, mat4 viewProjMat, size_t plane_index);


private:
	//flat surfaces defining the volume of this frustum.
	//left, right, bottom, top, near, far
	plane planes[6];  
	
	
	std::vector<distanceSortedObj> frustum_o_Contents; //opaque
	std::vector<distanceSortedObj> frustum_t_Contents; //transparent

	//invisible (no mesh/no material on the gameObject):
	std::vector<distanceSortedObj> frustum_invis_Contents; 

};

