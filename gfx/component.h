#pragma once

//components can be attached to GameObject instances.
//they can be mesh, transform, camera, etc. All of them inherit from component.

//TODO might be obsolete, but is a way back in case gameObject::getComponent() 
//results in circular dependincies.
enum componentType {
	NONE = 0,
	TRANSFORM,
	MESH,
	CAMERA, 
	MATERIAL
};

class transform;


class component{
	friend class gameObject;


public:
	component();
	~component();

	//returns the transform, which is attached to the 
	//same gameObject as this component.
	inline const transform *get_Transform() const {
		return m_transform;
	}


	inline gameObject *getGameObject() const {
		return m_gameObject;
	}

protected:
	//get envoked every frame by the gameObject, to which this component is 
	//attached.  dt = change in time since the previous update.
	virtual void componentUpdate(float dt) = 0;

	//envoked by the gameObject once this component is added to the gameObject.
	//Allows for additional initializations to be carried out. 
	//Remember that  m_gameObject  and  m_transform  will be set in
	//gameObject::AttachComponent() anyway.
	virtual void onGameObject_AddComponent() = 0;

	gameObject *m_gameObject; //game object to which this component is attached.

	//every component will have a link to the transform of this gameObject.
	//(even the transform component itself)
	transform *m_transform;


	 //tells if this component a mesh, camera, transform etc.
	//allows to perform reinterpret owncasts directly, since we will know the type.
	componentType m_type;
};

