#include "pointLight.h"
#include "repositories.h"
#include "transform.h"
#include "frustum.h"
#include "gameObject.h"
#include "material.h"
#include "mesh.h"
#include "window.h"
#include "texture.h"


void pointLight::setup_illumShader_samplers() {
	glUseProgram(illumination_shader->getShaderProgramID());
		GLuint loc =  glGetUniformLocation(illumination_shader->getShaderProgramID(),
											 "scene_normalTex");
		glUniform1i(loc, 0);

		loc = glGetUniformLocation(illumination_shader->getShaderProgramID(),
								   "scene_depthTex");
		glUniform1i(loc, 1);

		loc = glGetUniformLocation(illumination_shader->getShaderProgramID(),
								   "lightDist_cubeTex");
		glUniform1i(loc, 2);
	glUseProgram(0);
}


pointLight::pointLight(GLuint shadowmap_resolution, vec2 nearFarRange)
										: light(shadowmap_resolution, nearFarRange){
	
	//generate the cube shadowmap (automatically picks it up, 
	//due to the number arguments):
	shadowmap_texture = new texture(GL_R32F, false,	shadowmap_resolution);
	depth_texture = new texture(GL_DEPTH_COMPONENT32, false, shadowmap_resolution);

	//setup the shader used in shadowmap generation  and in scene-lighting:
	this->shadowmap_shader = repositories::getShader("shadowmapDefault");
	this->illumination_shader = repositories::getShader("pointLight_illum");
	//setup texture-samplers in the shader, binding them to appropriate texture unit:
	setup_illumShader_samplers();

	this->lightCamera = new camera( get_light_nearFar_range(),
									std::vector<renderer*>() ); //empty renderers.

	//influence the aspect ratio of the camera.
	//It infulences the projectionMatrix generation:
	//TODO why force the viewport if we will use the one of renderer anyay:
	//better pass vec4(0,0,  0,0) to it in order NOT to override the renderer viewport
	this->lightCamera->set_viewportArea( vec4(0,0, shadowmap_resolution, shadowmap_resolution) );
	light_mesh = repositories::getMesh("geosphere");
}



pointLight::~pointLight(){
	delete shadowmap_texture;
	//TODO delete color and normal texture (From GI)

	delete lightCamera;
	//framebuffer is destroyed by the parent class "light"
}



//TODO check if const can be added to argument.
//if the user wants everything visible to be rendered in color, drawInColor = true.
//Otherwise, object won't supply its material color, textures, etc.
void render_gObject(gameObject *go, GLuint shader_prog_to_use,  bool drawInColor){
	
	material *_material = go->getMaterial();

	if (!_material) //if there is no material - no point in rendering.
		return;    //return, skipping the drawing of this gameObject.
	
	//nullptr for current renderer (so it doens't attempt to update matrix uniforms).
	//True to make it use our currently bound shader:
	//checks if it has a mesh, only then draws it.
	//If the user wants everything visible to be rendered in color, drawInColor=true.
	//Otherwise, object won't supply its material color, textures, etc.
	_material->draw(nullptr, shader_prog_to_use, drawInColor);

	//TODO check that it INDEED uses prev bound shader AND doesn't send matrices.
}





void pointLight::render_Cube_side(	int side, vec3 pitch_yaw_roll_world,
									std::vector<texture*> out_textures,
									GLuint shaderID,
									bool renderObjectsInColor/*=true*/) {

		//will re-compute frustum's planes:
	lightCamera->detached_cam_Update( this->m_transform->getPos_world(),
									   pitch_yaw_roll_world);

	glUseProgram(shaderID);
	//upload the viewMatrix for this orientation of the cubemap's side:
	GLuint loc = glGetUniformLocation(shaderID,   "viewMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&lightCamera->getViewMatrix());
	//TODO ignore back faces?

	//setup frustum and identify potentially visible objects, which are 
	//in front of the light's camera:
	frustum *frust = lightCamera->getFrustum();
	//grab all the gameObjects seen by this frustum. True to ignore invisible.
	//Gathers all objects that are inside of the frustum, then sorts the list.
	//Only captures the ones which have a bounding shape.
	frust->frustumCapture(this->m_transform->getPos_world(), true);

		 //if color textures are supplied
		 //then we can set color masks to true.
		bool color_present= false; 
		bool depth_present = false;

		for (int i = 0; i < out_textures.size(); ++i) {

			//check if the currently inspected cube texture is depth (or shadow):
			bool is_depth = out_textures[i]->is_depth();

			if (is_depth) {
				depth_present = true;
			}
			else {
				color_present = true;
			}
			//attach a side of the corresponding  cube texture to be 
			//used as framebuffer's output:
			glFramebufferTexture2D(GL_FRAMEBUFFER,
							 is_depth ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0+i,
							 GL_TEXTURE_CUBE_MAP_POSITIVE_X + side,
							 out_textures[i]->get_OGL_id(), 0);
			//TODO add support for multiple depth attachments, not just one.
			//framework::fetch_OGL_errors();
		}					 
		color_present ? glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE) : 
					    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		//if it turns out that there is no depth attachment, - because we want
		//to have depth test, we need to attach our depth_texture (NOT THE SHADOWMAP)
		//Make sure it's the same resolution ans the other color attachments. TODO
		if (!depth_present) {
			glFramebufferTexture2D(  GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
									 GL_TEXTURE_CUBE_MAP_POSITIVE_X + side,
									 depth_texture->get_OGL_id(), 0  );
		}
		//framework::test_if_framebuffer_complete();
	
	//clear contents of the newly attached color/depth side of each texture:
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto all_captured = frust->get_opaqueContents_asc();

	for (auto captured : all_captured) {
		render_gObject( captured._gameObject,  shaderID,   renderObjectsInColor);
	}
	//TODO make transparent object cast shadow as well
	//TODO check that frustum is no longer accepting all objects in by default
	//(was set for testing purpouses)

	//cleanup
	for (int i = 0; i < out_textures.size(); ++i) {
		//check if the currently inspected cube texture is depth (or shadow):
		bool is_depth = out_textures[i]->is_depth();

		//DETACH the side of the corresponding  cube texture from
		//the framebuffer:
		glFramebufferTexture2D(GL_FRAMEBUFFER,
						    is_depth ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0 +i,
						    GL_TEXTURE_CUBE_MAP_POSITIVE_X + side,
							0,  0);
	}
	glUseProgram(0);
}



void pointLight::render_into_cube_textures( std::vector<texture*>output_textures,
							GLuint glCullMode/*=GL_BACK*/,  GLuint shaderID/*= 0*/){

	size_t res = output_textures.at(0)->get_xRes();
	glBindFramebuffer(GL_FRAMEBUFFER, getLight_frameBuffer_id());
	glViewport(0, 0, res, res);

	  //tell the framebuffer into which attachements we can render:
	  std::vector<GLuint> drawBuffers;
	  for (texture *tex : output_textures) {
			  if (!tex->is_depth() && !tex->is_stencil()) {
				  drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + drawBuffers.size());
			  }
	  }//tell the framebuffer about all of the attachments that it can render into:
	  if(drawBuffers.size() > 0){
	  	  glDrawBuffers(drawBuffers.size(), &drawBuffers[0]);
	  }

	glEnable(GL_DEPTH_TEST);
	//shadowmaps should only recieve back faces to minimize z-fighting:
	if (glCullMode == GL_NONE) {
		glDisable(GL_CULL_FACE);
	}
	else {
		glEnable(GL_CULL_FACE);
		glCullFace(glCullMode);
	}

	//upload projection uniform, since view matrix will be uploaded
	//with every different cube face:
	updateCameraProj();
	

	if (shaderID == 0) {
		shaderID = shadowmap_shader->getShaderProgramID();

		glUseProgram(shaderID);

		GLuint loc = glGetUniformLocation(shaderID, "lightPos");
		vec3 lightPos = this->get_Transform()->getPosition();
		glUniform3fv(loc, 1, (float*)&lightPos);
	}

	upload_proj_mat(shaderID);

	//framework::fetch_OGL_errors();

	//render scene from 6 points of view, using our lightCamera's matrices.
	//setup the orientation for the first view (pitch / yaw / roll):
	vec3 p_y_r_world;
	//pos_x, neg_g,   pos_y, neg_y,  pos_z, neg_z
	for (int i = 0; i < 6; ++i) { //TODO rotate the view 6 directions
		switch (i) {
			case 0:
			case 1: {
				p_y_r_world = vec3(0, 90 + 180 * (i % 2), 180);
			}break;
			case 2:
			case 3: {
				p_y_r_world = vec3(90 + 180 * (i % 2), 0, 0);
			}break;
			case 4:
			case 5: {
				p_y_r_world = vec3(180, 0 + 180 * (i % 2), 0);
			}break;
		}
		//render the side of the scene as seen from light, using the
		//desired shader.
		render_Cube_side(i, p_y_r_world, output_textures, shaderID);
		//framework::fetch_OGL_errors();
	}
	//now our framebuffer contains 6 rendered depth textures.
	//The shadowmap is updated.

	glUseProgram(0); //unbind the shadowmap shader.
	glBindFramebuffer(GL_FRAMEBUFFER, 0); //unbind our light's framebuffer.
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glCullFace(GL_BACK);
}





void pointLight::updateCameraProj() {
	//let's see if the recorded nearFar range doesn't correspond 
	//to the parent's one (it was changed):
	if (old_near_far_range != get_light_nearFar_range()) {

		lightCamera->set_nearFar_clipPlanes(get_light_nearFar_range());

		old_near_far_range = get_light_nearFar_range();

		//false to prevent frustum planes of being recomputed.
		//We'll soon do it anyway
		lightCamera->updateProjMatrix(false);
	}
}



void pointLight::upload_proj_mat(GLuint shaderID) {
	glUseProgram(shaderID);
	//TODO supply light position into shader.

	//supply projection matrix value only once to the program.
	//It will remain the same as we draw all the objects.
	//The viewmatrix however, will change 6 times, while we render the cube shadowmap 
	GLuint loc = glGetUniformLocation(shaderID,  "projMatrix");
	mat4 lightProjMat = lightCamera->getProjMatrix();
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&lightProjMat);

	glUseProgram(0);
}


void pointLight::updateShadowmaps(float dt) {
	
	//we will check what the light sees in all 6 directions arround itself,
	//and will store it in this texture:
	std::vector<texture*> output_textures;
	output_textures.push_back(	(this->shadowmap_texture));

	glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
	// we cull GL_FRONT faces, NOT the back ones.
	//we do this to minimize further z-fighting, since we generate shadowmap.
	this->render_into_cube_textures( output_textures, GL_FRONT); 
	glClearColor(0, 0, 0, 0);
}


void pointLight::pointLightPreDraw_UniformSetup(const renderer *curr_renderer,
							texture *_renderTextures[renderTextures::MAX_SIZE],
							GLuint shaderToUse/*=0*/) const{
	GLuint currProg;
	if (!shaderToUse) {
		//use our illumination shader
		currProg = illumination_shader->getShaderProgramID();
		glUseProgram(illumination_shader->getShaderProgramID());
	}
	else {//else use the provided shader
		currProg = shaderToUse;
		glUseProgram(shaderToUse);
	}
	//TODO we use program before we start to supply uniform values.
	//check what happens when we move it after glActiveTexture() calls.
	//will textures still work? :D  Or do they have to be related to a bound shader?

	//you can also get the dimensions of the window from the renderer to know 
	//the resolution of the texel, etc.
	//simmilarly, you can get the viewMatrix->world position to get pos of the camera

	//associate the provided texture of scene's normals (with emission in alpha)
	//to texture unit 0:
	_renderTextures[renderTextures::NORMAL_AND_EMISSION_INDX]
														->associate_to_TextureUnit(0);
	//associate the provided texture of scene's depth to texture unit 1:
	_renderTextures[renderTextures::DEPTH_INDX]->associate_to_TextureUnit(1);
	//associate light's shadowmap tex (the one from this light) to texture unit 2:
	shadowmap_texture->associate_to_TextureUnit(2);

	//don't worry, texture samplers were already setup in pointLight's constructor.
	//They point to the appropriate texture units. 
	//TODO check if in pointLights constructor, maybe it was moved to light's one.

	//construct a view-projection from renderer's matrices.
	mat4 renderer_view_mat = curr_renderer->get_curr_view_mat4();
	mat4 renderer_proj_mat = curr_renderer->get_curr_proj_mat4();
	mat4 renderer_VP_mat = renderer_proj_mat * renderer_view_mat;


	mat4 lightModelMatrix; //we won't use m_transform, since we use range as scale
	float light_range = this->old_near_far_range.y;
	//orientation will always be (0,0,0) for point lights
	mat4 translation = mat4::translation(m_transform->getPos_world());
	mat4 scale = mat4::scale(vec3(light_range));
	lightModelMatrix = translation * scale;

	//upload MVP for this light, as well as inverse of ViewProjection, 
	//used by currentr renderer
	GLint loc = glGetUniformLocation(currProg,	"MVP");
	glUniformMatrix4fv(loc, 1, GL_FALSE,(float*)&(renderer_VP_mat*lightModelMatrix));


	loc = glGetUniformLocation(currProg, "inverseVP");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&renderer_VP_mat.inverse());


	//upload camera position (need for specularity)
	//TODO check that it actually returns a correct translation:
	vec3 cameraPos_world = renderer_view_mat.get_translation();

	loc = glGetUniformLocation(currProg, "cameraPos");
	glUniform3fv(loc, 1, (GLfloat*)&cameraPos_world);

	//upload the world position of light:
	vec3 lightPos_world = m_transform->getPos_world();
	loc = glGetUniformLocation(currProg, "lightPos");
	glUniform3fv(loc, 1, (GLfloat*)&lightPos_world);


	//upload light range
	loc = glGetUniformLocation(currProg,"lightRange");
	glUniform1f(loc, light_range); //we calculated this value previously in this func

								 
	loc = glGetUniformLocation(currProg, "nearFarRange");
	glUniform2fv(loc, 1, (float*)&this->lightCamera->getNearFar_ClipPlanes()); 


    //upload the size of one pixel (we want the dimensions to fit into 0-1 range)
	loc = glGetUniformLocation(currProg,"pixelSize");
	float w_width = curr_renderer->get_window()->getWidth();
	float w_height = curr_renderer->get_window()->getHeight();
	vec2 pixel_size = vec2(1 / w_width, 1 / w_height);
	glUniform2fv(loc, 1, (float*)&pixel_size);

	loc = glGetUniformLocation(currProg, "lightIntensity");
	glUniform1f(loc, get_intensity());

	loc = glGetUniformLocation(currProg, "lightColor");
	glUniform3fv(loc, 1, (float*)&get_color());
}



void  pointLight::draw( const renderer *curr_renderer, GLuint outputFBO,
						texture *_renderTextures[renderTextures::MAX_SIZE] ) {
	
	this->pointLightPreDraw_UniformSetup(curr_renderer, _renderTextures);

	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
	
	glViewport(0,0, curr_renderer->get_window()->getWidth(), 
					curr_renderer->get_window()->getHeight());

	glCullFace(GL_FRONT); //render back faces of the light sphere-mesh.
	//framework::test_if_framebuffer_complete();

	light_mesh->draw_mesh();

	//TODO renderer can bind it once, can't it?  (watch out for GI lights though)

	//now we've outputted into the color attachments of outputFBO, supplied
	//as an argument to this function.
	//Whoever called it might now use this buffer containing fresh 
	//illumination results further on.
}