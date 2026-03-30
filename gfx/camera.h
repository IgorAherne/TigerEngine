#pragma once
#include "component.h"
#include "../gfxmath/vec3.h"
#include "../gfxmath/vec2.h"
#include "../gfxmath/mat4.h"
#include <vector>

class frustum; //don't use any of it's functionality in the header!

//determines the types of projections for the projectionMatrix
enum projection_type {
	PERSPECTIVE = 0,
	ORTHOGRAPHIC,
	INVERSE_PERSPECTIVE
};


class camera : public component{
	friend class gameObject;
	friend class renderer;
	friend class light;

public:
	//creates a camera component. It relies on the transform component. Transform
	//must exist on the game object before this camera one.
	//renderers is which renderers will be working with this camera. Supply empty
	//vector if no renderer must be ralated to this camera.
	//Camera will use default field of view. Change it manually for a different 
	//value.
	camera(	const vec2 &near_far_clip, std::vector<renderer*> renderers,
			projection_type _type = projection_type::PERSPECTIVE );


	//change the field of view of the camera to the new one (in degrees)
	inline void setFov(float fov_deg) {
		this->fov_deg = fov_deg;

		updateProjMatrix();
	}

	//change the distance to the near and far planes of the viewing frustum
	inline void set_nearFar_clipPlanes(const vec2 &near_far_dist) {
		this->near_far_clip = near_far_dist;

		updateProjMatrix();
	}

	//get the distances to near and far clipping planes:
	inline vec2 getNearFar_ClipPlanes() const {
		return near_far_clip;
	}

	//supply the position of the viewport (xy) and its width and heignt (zw)
	//If z or w is zero, the camera will be outputting to the entire viewport area,
	//not some restricted part of it.
	//
	//Also, this adjusts the inverse_aspect of the camera. (this infulences 
	//the projectionMatrix generation)
	inline void  set_viewportArea(vec4 xy_wh) {
		viewport_xy_wh = xy_wh;
		last_inv_aspect = viewport_xy_wh.w / viewport_xy_wh.z; //height/width
	}



	void setProjectionType(projection_type _type) {
		this->proj_type = _type;
	}

	projection_type getProjectionType() const {
		return this->proj_type;
	}

	//get the position of the viewport (xy) and its width and heignt (zw)
	//If z or w is zero, the camera will be outputting to the entire viewport area,
	//not some restricted part of it.
	inline vec4 getViewportArea() const {
		return viewport_xy_wh;
	}


	//get the current field of view of the camera (in degrees)
	inline float getFov() const {
		return fov_deg;
	}

	inline frustum *getFrustum() {
		return m_frustum;
	}

	inline mat4 getViewMatrix() const {
		return this->viewMat;
	}

	inline mat4 getProjMatrix() const {
		return this->projMat;
	}


	//Call when camera is not attached to any gameObject, but is used somewhere 
	//else.
	//Rebuilds the viewmatrix to the one based of the given position and 
	//orientation (both in world space).
	//Recall that you can updateProjMatrix(), recomputing it.
	//Also, you can change fov or ortho zoom
	//
	//This function allways recomputes the frustum when done.
	void detached_cam_Update(vec3 position_world, vec3 pitch_yaw_roll_world);
	

	//"bakes in" the values like fov, near/far plane into the projMat of this camera.
	//Always recomputes the frustum.
	//Will recompute frustum, unless the last argument is forced into "false".
	void updateProjMatrix(bool recompute_frust = true);


protected:
	void onGameObject_AddComponent() override;

	//updates positon and rotation, then re-calculates the view matrix.
	//dt is the change in time.
	//in protected section, since only certain classes can invoke updates on 
	//components.
	void componentUpdate(float dt) override;





protected:
	//the transform of the gameObject to whcih this camera is attached:
	mat4 projMat; //the projection matrix
	projection_type proj_type;//orthographic, perpsective, inv perspective, etc.

	//built from the pitch_yaw_roll and the pos (in the TRANSFORM component of this
	//game object)
	mat4 viewMat;

	frustum *m_frustum; 
	
	vec2 near_far_clip; //near and far clipping plane distances.

	//position and width/height of the viewport. If width or height is zero,
	//then camera will be drawing drawing on the entire viewport, not a portion.
	vec4 viewport_xy_wh; 
	float fov_deg;
	float ortho_inverseZoom;//smaller values result in narrower frustum, --> zooming.
	float last_inv_aspect;//inverse aspect of the last window this camera worked with
	
	std::vector<renderer*> renderers;//renderers that are working with this camera.
};