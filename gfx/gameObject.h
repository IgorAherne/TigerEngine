#pragma once
#include <set>
#include "renderer.h",
#include <string>
#include "transform.h"

class component; //be careful, don't use any functions of component in this header.
enum class componentType; //same, don't use its functionality here. Only in .cpp file
class bshape; //bounding shape
class mesh;


class gameObject {
	friend class scene;
	friend class material;
	friend class mesh;

public:
	//create a gameObject. If invisible is left as false, a material with a default
	//shader will be attached to the gameObject.
	//Also, you can optionally supply the gameObject's parent.
	gameObject(	std::string name = "", 
				bool invisible = false,
				gameObject *parentGameObject = nullptr);
	~gameObject();


	virtual void update(float dt);

	inline const std::vector<renderer*> getRenderer(size_t renderer_id) {
		return renderers;
	}


	//returns the pointer to a specified component (or nullptr if not found).
	//TODO. if circular dependency occurs, scrap using the Template and revert
	//to using component::m_type as argument. Return void*, user has to cast into
	//the type he expected
	template<class T>
	T *getComponent() {
		for (auto itr = components.begin(); itr != components.end(); ++itr) {
			T *casted_component = dynamic_cast<T*>(*itr);

			if (casted_component) {
			   return casted_component;//return a pointer to this component.
			}
		}
		return nullptr; //couldn't find the requested component.
	}


	//attempt to attach the component to this gameObject.
	//returns false if failed to do so. In that case make sure the _component is 
	//PUBLICALLY inherriting from component class.
	template<class T>
	bool attachComponent(T *_component = nullptr) {
		component *c = dynamic_cast<component*>(_component);
		//TODO see if incoming component is transform, then update gameObject::m_transform.
		//TODO Also, delete old transform.

		//TODO see if the material was attached. If so, configure our m_material
		//shortcut in this gameObject.

	   //if conversion into component* failed:
		if (c != nullptr) { //TODO check that it converts and attaches components.
			components.push_back(c);
			c->m_gameObject = this; //tell the component that we are its gameObject.
			
			//transform must be the first component added to a gameObject.
			//Hence, after a component was added, we can guarantee that the
			//transform of this gameObject is already established and avalable.
			c->m_transform = this->m_transform;
			//for any additional set-up, envoke component's function:
			c->onGameObject_AddComponent();
			return true;
		}
		return false; //failed to cast to  component*
	}

	
	// bounding shape is typically used for gameObject culling
	//function recomputes it, using transform of this gameObject.
	//It adjusts the dimensions of the bounding shape, to encapsulate the entire
	//gameObject inside of itself.
	//Sets boundingShape to null if mesh component is absent. 
	//However, if it is present and the bounding shape is nullptr, function will 
	//create a new bounding shape for this gameObject.
	void reBoundingShape_fromTransform();

	//get bounding shape encapsulating this gameObject. Typically used for 
	//gameObject culling. It is nullptr after gameObject instantiation, unless some 
	//component, like meshcomponent doesn't change it for us or we don't set it 
	//manually.
	inline bshape *getBoundingShape() {
		return this->boundingShape;
	}

	//a mandatory component, hence we can always get it directly
	transform *getTransform() {
		return m_transform;
	}

	//material is a very common component, essential for rendering stages.
	//To be rendered, a game object must have a material.
	//Returns nullptr if there is no material component.
	material *getMaterial() {
		return m_material;
	}

	//TODO getParent should return nullptr if our parent OR US is a root.


	void attachChild(gameObject *child);

	void detachChild(gameObject *child);


	inline std::set<gameObject*> getAllChildren() {
		return children;
	}

	

private:
	//in private section, since we only want this functionality be used by the
	//listed friend classes.   dt = change in time since the previous frame.
	virtual void updateComponents(float dt);

	//additional "modules" attached to this gameObject instance
	std::vector<component*> components;  

	std::set<gameObject*> children;

	//which renderers are processing this gameObject? That way we know
	//which windows this gameObject will be shown in
	std::vector<renderer*> renderers; 

	gameObject *parentGameObject;

	//bounding shape encapsulating this gameObject. Typically used for gameObject
	//culling.
	bshape *boundingShape; 

	//every gameObject always has a transform in its components vector.
	//This is a quicklink, established during gameObject construction.
	//That way, we don't have to search for the transform component every time we 
	//need it.
	transform *m_transform;

	//direct link to the material of the gameObject. 
	//although gameObject will have this material in its components vector, it's 
	//a fast way to access the material.
	//Just as the transform, material is very common. They must be present on
	//every game object if it has to be rendered.
	//Will be nullptr if there is no material on this gameObject.
	material *m_material;

	//direct link to the mesh component of the gameObject.
	//although gameObject will have this mesh in its components vector, it's 
	//a fast way to access the mesh.
	//Just as the transform, material is very common. They must be present on
	//every game object if it has to be rendered.
	//Will be nullptr if there is no mesh on this gameObject.
	mesh *m_mesh;


	std::string name;
};

