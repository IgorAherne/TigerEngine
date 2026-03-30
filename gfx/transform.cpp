#include "transform.h"
#include "gameObject.h"
#include "../gfxmath/bshape.h"
#include "../gfxmath/obb_bshape.h"

transform::transform() {//true since we ARE the transform component.
	scale = vec3(1, 1, 1);
	//position is automatically vec3(0,0,0)
	//same for rotation values.
	parentTransform = nullptr;
};

transform::transform(const vec3 &pos, const vec3 &scale, vec3 pitch_yaw_roll,
					 const transform *parentTransform/*=nullptr*/){
					// :component(true) since we ARE the transform component.

	this->position = pos;
	this->scale = scale;
	this->pitch_yaw_roll = pitch_yaw_roll;
	this->parentTransform = parentTransform;
}


transform::~transform(){
//recall, transform has a pointer to the parent transform, but no links to children.
//It is gameObject's responsibility to delete all the child gameObjects. 
//All the corresponding components will also be removed from them.
}




void transform::componentUpdate(float dt) {
	//TODO recompute the transf mat4 based on parent's transf and the current 
	//values of yaw pitch roll

	
	//Check if parent actually exists.
	mat4 parent_transf;
	
	if (!parentTransform) {
		parent_transf.toIdentity();
		//and keep this->pitch_yaw_roll_world  as the local  pitch_yaw_roll
		this->pitch_yaw_roll_world = this->pitch_yaw_roll;
		//same with world position:
		this->position_world = this->position;
	}
	else {
		parentTransform->getTransf();
		this->pitch_yaw_roll_world = parentTransform->pitch_yaw_roll_world
															+ this->pitch_yaw_roll;
		//same with position_world, add it to the one of parent
		this->position_world = parentTransform->position_world +this->position;
	}

	 //we rely on the parent to already having completed 
	 //its "transf" mat4 re-computation.	
	
	//TODO do a single rot matrix, in one go. Do it instead of those 3:
	mat4 xRot = mat4::euler_rot(pitch_yaw_roll.x, 0,0);
	mat4 yRot = mat4::euler_rot(0, pitch_yaw_roll.y, 0); //TODO rotation order
	mat4 zRot = mat4::euler_rot(0, 0, pitch_yaw_roll.z); //TODO rotation order
	
	transf.toIdentity();
	mat4 scale_mat = mat4::scale(this->scale);
	mat4 translation_mat = mat4::translation(this->position);
	
	transf =  translation_mat * zRot*yRot*xRot * scale_mat;

	transf = parent_transf * transf; //make our transf relative to parent's one.

	//update the bounding shape manually, since it's not a component, and will NOT 
	//be updated with gameObject::updateComponents()
	bshape *_bshape = m_gameObject->getBoundingShape();
	if (_bshape) {
		//set center world position, world scale and world rotations:
		dynamic_cast<obb_bshape*>(_bshape)->setScaleRot_matrix(transf._mat3());
		//TODO don't assume it's object-aligned bounding box
		_bshape->setCenterPos_world(position_world);
	}

	//Don't envoke any child updates here.  Remeber, its gameObject who is in charge 
	//of envoking the componentUpdates on itself  and  all children gameObjects.
}


