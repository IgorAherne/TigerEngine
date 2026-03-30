#pragma once
#include "component.h"
#include "../gfxmath/mat4.h"
#include "../gfxmath/vec3.h"

//Transform simply tracks the local (relative to parent transform) position, scale 
//and rotation, and the world ones.
//Every component update, transform refreshes its "trasf" matrix4, which is 
//used as the modelMatrix of the gameObject to which this transform is attached.
//
//Transform also updates the bounding shape of the gameObject. It updates its 
//world poisitions, scale and rotations, in the end of componentUpdate()

class transform : public component{
	friend class gameObject;

public:
	//relative to the parent's transform
	inline void setPosition(const vec3 &pos){
		position = pos; 
		//new world position will be increased by the change of old local position 
		//to the new local position

		//TODO: incorrect     transform new pos into world first.
		position_world = position;
	}

	inline void addPosition(const vec3 &extra_pos) {
		position += extra_pos;
		//TODO: incorrect     transform new pos into world first.
		position_world = position;
	}

	//relative to the parent's transform
	inline void setScale(const vec3 &scale) {
		this->scale = scale;
	}

	//set all three rotations (xyz) IN DEGREES
	//relative to the parent's transform
	inline void setPitchYawRoll(const vec3& pitch_yaw_roll) {
		this->pitch_yaw_roll = pitch_yaw_roll;
		//TODO, just as in setPosition(), we need to update the 
		//world yaw_pitch_roll
	}

	//increase all three rotations (xyz) IN DEGREES.
	//Relative to the parent's transform.
	inline void addPitchYawRoll(const vec3 additional_pitch_yaw_roll) {
		this->pitch_yaw_roll += additional_pitch_yaw_roll;
		//TODO, just as in setPosition(), we need to update the 
		//world yaw_pitch_roll
	}

	//set x-rotation IN DEGREES
	//relative to the parent's transform
	inline void setPitch(float pitch) {
		pitch_yaw_roll.x = pitch;
		//TODO, just as in setPosition(), we need to update the 
		//world  pitch
	}
	//set y-rotation IN DEGREES	
	//relative to the parent's transform
	inline void setYaw(float yaw) {
		pitch_yaw_roll.y = yaw;
		//TODO, just as in setPosition(), we need to update the 
		//world yaw
	}
	//set z-rotation IN DEGREES
	//relative to the parent's transform
	inline void setRoll(float roll) {
		pitch_yaw_roll.z = roll;
		//TODO, just as in setPosition(), we need to update the 
		//world roll
	}
	
	//return the copy of  transformation matrix from this transform.
	//Usefull when a transform of the child game object wants to update its 
	//own transf.
	//The returned matrix represents a transformation of this object relative 
	//to world's axis.
	//This matrix was based on all the transfromations of the transform parents.
	//Therefore, when all of them were combined, we
	//got this matrix, which represents the object's transform relative to world.
	inline const mat4 &getTransf() const {
		return transf;
	}

	//relative to the parent's transform
	inline vec3 getPosition() const {
		return position;
	}
	//relative to the parent's transform
	inline vec3 getScale() const {
		return scale;
	}
	//returns rotations arround all 3 axis, in a single vector.
	//Relative to the parent's transform
	inline vec3 getPitchYawRoll() const {
		return pitch_yaw_roll;
	}

	//makes the rotation matrix out of our pitch, yaw and roll, then returns it
	//If you just want rotation and scale, use getScaleRotationMat(), which is faster
	//The returned matrix represents ROTATIONS of this object relative 
	//to world's axis.
	//This matrix was based on all the transfromations of the transform parents.
	//Therefore, when all of them were combined, we
	//got this matrix, which represents the object's transform relative to world.
	inline mat4 getRotationMat() const {
		return mat4::euler_rot(pitch_yaw_roll.x, pitch_yaw_roll.y, pitch_yaw_roll.z);
	}

	//TODO get the total scale, accomulated from all the parents


	//extract a portion of transform matrix, which will contain rotation AND scaling.
	//
	//This matrix was based on all the transfromations of the transform parents.
	//Therefore, when all of them were combined, we
	//got this matrix, which represents the object's transform relative to world.
	inline mat3 getScaleRotationMat() const {
		return transf._mat3();
	}

	//get x-rotation in degrees
	inline float getPitch() const {
		return pitch_yaw_roll.x;
	}
	//get y-rotation in degrees
	inline float getYaw() const {
		return pitch_yaw_roll.y;
	}
	//get z-rotation in degrees	
	inline float getRoll() const {
		return pitch_yaw_roll.z;
	}

	//position NOT relative to parents, but directly to WORLD zero coordinate.
	inline vec3 getPos_world() const {
		return position_world;
	}
	//rotation NOT relative to parents, but directly to WORLD axis.
	inline vec3 getPitchYawRoll_world() const {
		return pitch_yaw_roll_world;
	}
	//get x-rotation in degrees relative to WORLD center, not to parent.
	inline float getPitch_world() const {
		return pitch_yaw_roll_world.x;
	}
	//get y-rotation in degrees relative to WORLD center, not to parent.
	inline float getYaw_world() const {
		return pitch_yaw_roll_world.y;
	}
	//get z-rotation in degrees relative to WORLD center, not to parent.
	inline float getRoll_world() const {
		return pitch_yaw_roll_world.z;
	}
	





protected:
	void onGameObject_AddComponent() override{
		//make sure the transf mat4 is setup before we let go
		componentUpdate(0.0f); 
	}
	void componentUpdate(float dt) override;


private:
	//Never make constructor/destructor public.
	//It's only the game object who has the right to create/delete its own transform.

	//create a transform without the parent, at world zero, scale 1 and 0 rotaiton.
	transform();

	//if parentTransform is default, then the transform won't have parentTransform.
	transform(	const vec3 &pos, const vec3 &scale, vec3 pitch_yaw_roll,
				const transform *parentTransform = nullptr );

	~transform(); 



	mat4 transf; //relative to parent. Used in all the spacial manipulations.

	//position, scall and pitch_yaw_roll are only used to build the transf matrix.
	vec3 position;
	vec3 scale;
	vec3 pitch_yaw_roll; //rotations arround x,y,z

	//xyz rotaiton NOT relative to parents, but directly to WORLD zero coordinate.
	vec3 pitch_yaw_roll_world; //TODO (expensive to extract) just add to parent's one
	//pos NOT relative to parents, but directly to WORLD zero coordinate.
	vec3 position_world;


	//direct link to the transform class of the parent.
	//We have no right to detach or change that transform!
	//However, like this we will be able to quickly traverse the transform tree graph
	//and build up the correct transformations based of parent-transform.
	const transform *parentTransform; 

	//TODO later on we might use this class for storing occlusion culling data, etc.
};

