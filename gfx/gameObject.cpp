#include "gameObject.h"
#include "component.h"
#include "mesh.h"
#include "transform.h"
#include "material.h"
#include "camera.h"
#include "scene.h"
#include "../gfxmath/obb_bshape.h"


gameObject::gameObject(	std::string name/*=""*/, bool invisible/*=false*/,
						gameObject *parentGameObject /*= nullptr*/ ){
	this->name = name;

	//always nullptr untill we or some component (mesh) sets it.
	this->boundingShape = nullptr;
	//mesh shortcut is nullptr, there is no mesh component attached yet.
	m_mesh = nullptr;


	//transform will exist on any gameObject, so we need to create one.
	//We will prepare it on the next few lines. Make sure that the boundingShape
	//and m_mesh were initiallzed to nullptr prior to that. 
	//Otherwise, finilization of this transform will fail during 
	//transform::onGameObject_AddComponent(), which will call 
	//transform::componentUpdate(), which requests bounding shape (must be nullptr).
	transform *t;

	//see if parent game object was supplied:
	if (parentGameObject){
		transform *par_transf = parentGameObject->getTransform();
		t = new transform(vec3(0, 0, 0), vec3(1, 1, 1), vec3(0, 0, 0), par_transf);
		this->parentGameObject->attachChild(this);
	}
	else { //otherwise, no parent gameObject was supplied!
		//grab a pointer to the current scene's root.
		gameObject *root = scene::getCurrentScene()->getRootGameObject();

		t = new transform(vec3(0, 0, 0), vec3(1, 1, 1), vec3(0, 0, 0), 
											root? root->getTransform() : nullptr);
			//if root exists, we should set it as our parent.
			//root might be a nullptr if we are actually creating one right now.
		if (root != nullptr)
			root->attachChild(this);
	}//end if no parent was supplied


	this->m_transform = t; //update gameObject's shortcut to the transform.

	 //finally, store the value of t as a component of this gameObj:
	attachComponent<transform>(t); //Always use this function, since it tells the 
								//component that we are its m_gameObject, and sets
								//additional variables.

	//set-up the quick-link to the material, simmilar to how the transform
	//was set-up.
	if (!invisible) {
		m_material = new material("standard"); //request standard shader
		this->attachComponent<material>(m_material);
	}
}




gameObject::~gameObject(){
	if (parentGameObject) { //if parent game object exists, re-attach our 
		for (gameObject *child : children) { //children to be its new children.
			parentGameObject->attachChild(child);
		}
	}
	else { //otherwise, our parent gameObject is null!
		for (gameObject *child : children) { 
			//ensure each consecutive child will be calling this "else statement". 
			child->parentGameObject = nullptr;// Hence, set parent to null.
			delete child;					//Then delete.
		}
	}

	delete this; //when all the children were dealt with - delete this gameObject.
}//TODO: spawn and destroy many objects / components, detect memory leaks



void gameObject::updateComponents(float dt) {

	//Update all components except for transform (we will update it in the very end):
	for (component *c : components) {  //TODO use for loop
		//TODO get rid of ugly cast (and then alter the comment above):
		if (!dynamic_cast<transform*>(c)) {
			c->componentUpdate(dt);
		}
	}//end foreach component except for transform

	if (m_transform) {
		m_transform->componentUpdate(dt);
	}

	//envoke update components on all the child game objects
	for (gameObject *child_go : children) {
		child_go->updateComponents(dt);
	}
}



void gameObject::attachChild(gameObject *child) {
	children.insert(child); //children is a set, -there won't be any duplicates.
	child->parentGameObject = this;
}



void gameObject::detachChild(gameObject *child) {
	//detached game Object should still be the child of root.
	//TODO: recompute the transform of child, since now it's relative to root.
	scene::getCurrentScene()->getRootGameObject()->attachChild(child);

	children.erase(child);
}



void gameObject::update(float dt) {
	//todo game mechanics of this game object
}


void gameObject::reBoundingShape_fromTransform() {
	if (!m_mesh) {//TODO if mesh is removed from the gameObject, trigger this function
		this->boundingShape = nullptr;
		return;
	}

	if (!boundingShape){ //TODO various bounding shapes plz:
		boundingShape = new obb_bshape( m_transform->getPosition(), 
										m_mesh->getPositions(),	
										m_transform->getScaleRotationMat() );
		//this constructor created and the bounding shape and imidiatelly adjusted 
		//its dimensions via calling recompute()
		return;
	}

	//else, if mesh exists, and the bounding shape is not null, we need to 
	//update the bounding shape.
	boundingShape->recompute(m_mesh->getPositions()); //supply mesh local coords.

	//note, transform itself will be updating the scaleRotation matrix of this
	//bounding shape, and the center_pos_world as well, 
	//during   transform::componentUpdate()
}