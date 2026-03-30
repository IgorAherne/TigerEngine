#include "camera.h"
#include "gameObject.h"
#include "transform.h"
#include "frustum.h"
#include "input.h"



camera::camera(	const vec2 &near_far_clip,  std::vector<renderer*> renderers,
				projection_type proj_type/* = projection_type::PERSPECTIVE */) {

	this->near_far_clip = near_far_clip;
	this->proj_type = proj_type; 
	this->fov_deg = 90; //default field of view.
	this->ortho_inverseZoom = 10.0f;

	for (renderer *r : renderers) {
		//record which renderer this camera is working with:
		this->renderers.push_back(r);
		//tell the renderer that we are its camera:
		r->registerCamera(this);
	}

	//note, frustum will initialize its 6 planes. 
	//projMat and view Mat will be identity for now.
	//This is nessesary since camera instance might never get attached to a
	//gameObject, and will be used on its own (for example, in pointLight).
	this->m_frustum = new frustum(projMat * viewMat);
}



void camera::onGameObject_AddComponent() {
	//set up view matrix, based on the tranform.
	viewMat = mat4::viewMatrix( m_transform->getPos_world(),
								m_transform->getPitchYawRoll_world() );
	//update projMatrix re-shapes the frustum.
	updateProjMatrix(); //setup our projection matrix (to be persp or orthographic)
}



void camera::componentUpdate(float dt){
	//produce the view matrix based on the position and orientation of the transform
	//in WORLD. This transform is attached on the same gameObject as this camera.
	//This transform must be present by the time the camera component was added.
	//viewMat = mat4::viewMatrix( m_transform->getPos_world(),
	//										  m_transform->getPitchYawRoll_world());
	viewMat = mat4::viewMatrix(m_transform->getPos_world(),
								m_transform->getPitchYawRoll_world());
							
	//re-build frustum's planes, to update them, since the view matrix has changed
	//(position, rotation)
	this->m_frustum->reShape(projMat*viewMat);
	/*std::cout << "\n camera pos " << this->get_Transform()->getPosition().x << " "
		      << this->get_Transform()->getPosition().y << " "
		      << this->get_Transform()->getPosition().z << "\n ";*/

	//Renderer also updates the projection matrix when nessesary.
	//it happens during   renderer::renderRenderer(float dt)
	//One camera can be used by multiple renderers, hence it's best for them to 
	//update the aspect ratio as required.
}




void camera::updateProjMatrix(bool recompute_frust /* = true*/) {
	//TODO: when we have orthographic projection, we will need to alter 
	//different entries.
	
	//if our proj matrix is an orthographic one:
	if (this->proj_type == projection_type::ORTHOGRAPHIC) {
		  //if inv_aspect == 0.75,
		  //left_right will have to be multiplied by it,
		  //bot_top won't have to be, since  it would be multiplied by 1.
		  //(  height/width  is  how many heights PER width  )  
		  
		  vec2 left_right = vec2(	-this->ortho_inverseZoom*0.5,
		  							 this->ortho_inverseZoom*0.5  );

		  vec2 bot_top = vec2(-this->ortho_inverseZoom*last_inv_aspect*0.5,
							   this->ortho_inverseZoom*last_inv_aspect*0.5);
		  
		  this->projMat = mat4::ortho_mat(left_right, bot_top, this->near_far_clip);
	}

	else { //it is a perspective matrix    TODO: inverse perspective plz

		   //we will use the HEIGHT/width  ratio (the inverse of aspect ratio) of the
		   //latest window that is working with this camera.
			this->projMat = mat4::persp_mat(this->last_inv_aspect,
											this->near_far_clip,
											this->fov_deg);
	}
	//re-build frustum
	//TODO right now frustum contains 6 planes, which would be destroyed when 
	//re-shaping it. Howver, there might be additional info. 
	//If it's too big to re-create every time, check if it's null and re-shape 
	//otherwise.
	if(recompute_frust == true)
		this->m_frustum = new frustum(projMat*viewMat);
}



void camera::detached_cam_Update(	vec3 position_world, vec3 pitch_yaw_roll_world){

	viewMat = mat4::viewMatrix(position_world, pitch_yaw_roll_world);

	//re-build frustum's planes, to update them, since the view matrix has changed
	//(position, rotation)
	this->m_frustum->reShape(projMat*viewMat);
}