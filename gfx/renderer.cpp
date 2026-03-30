#include "renderer.h"
#include "window.h"
#include "camera.h"
#include "mesh.h"
#include "input.h"
#include "frustum.h"
#include "gameObject.h"
#include "transform.h"
#include "material.h"
#include "scene.h"
#include "repositories.h"
#include <sstream>
#include "light.h"
#include "texture.h"
#include "voxelGICore.h"
#include "../gfxmath/globals.h"


//initializing the static variables:
std::map<size_t, renderer*>  renderer::rendererInstances;





void renderer::onWindowResize() {
	this->resize_all_screen_textures();

	glUseProgram(lightCommitShader->getShaderProgramID());
	//recalculate pixel size for screen-textures.
	//re-upload it to the correct shaders.
	vec2 pixel_size = vec2(1.f/m_window->getWidth(),  1.f/m_window->getHeight());

	GLuint loc = glGetUniformLocation(lightCommitShader->getShaderProgramID(),
										"pixelSize");
	glUniform2fv(loc, 1, (GLfloat*)&pixel_size);
	glUseProgram(0);
}



void renderer::initFrameBuffers() {
	glCreateFramebuffers(1, &gFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gFBO);

	//attach all the screen-render textures into gFBO, as color attachments,  or
	//as depth. The indexation of GL_COLOR_ATTACHMENT0+t will work, since DEPTH is
	//the last value in the  renderTextures  enum.
	for (int t = 0; t < renderTextures::MAX_SIZE; ++t) {
		if (t == renderTextures::DEPTH_INDX) {
			glFramebufferTexture(	GL_FRAMEBUFFER,
									GL_DEPTH_ATTACHMENT,
									_renderTextures[t]->get_OGL_id(),
									0);
		}
		else {
			glFramebufferTexture(	GL_FRAMEBUFFER,
									GL_COLOR_ATTACHMENT0 + t,
									_renderTextures[t]->get_OGL_id(),
									0);
		}
		
	}
	//define what attachments the framebuffer can output to:
	//TODO try to give the range, not hardcode those 4 values
	GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
							  GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, };
	glDrawBuffers(4, attachments);

	//check if all attachments went through successfully:
	framework::test_if_framebuffer_complete();


	glCreateFramebuffers(1, &lightFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);

	//attach all the screen-render textures into gFBO, as color attachments,  or
	//as depth. The indexation of GL_COLOR_ATTACHMENT0+t will work, since DEPTH is
	//the last value in the  renderTextures  enum.
	for (int t = 0; t < lightRenderTextures::MAX_SIZE; ++t) {
		glFramebufferTexture(	GL_FRAMEBUFFER,
								GL_COLOR_ATTACHMENT0 + t,
								_light_renderTextures[t]->get_OGL_id(),
								0	);
	}
	//define what attachments the framebuffer can output to:
	//TODO try to give the range, not hardcode those 2 values
	GLuint attachments_light[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments_light);

	//check if all attachments went through successfully:
	framework::test_if_framebuffer_complete();
	glBindFramebuffer(GL_FRAMEBUFFER, 0); //unbind FBO after renderer was created.
}






void renderer::resize_all_screen_textures() {
	//TODO: perhaps some textures are not used, so no point in setting (and 
	//uploading them anywhere)
	for (int t = 0; t < renderTextures::MAX_SIZE; ++t) {
		_renderTextures[t]->resize(m_window->m_width, m_window->m_height, 0);
	}
	//resize light-contribution screen textures:
	for (int t = 0; t < lightRenderTextures::MAX_SIZE; ++t) {
		_light_renderTextures[t]->resize( m_window->m_width, m_window->m_height,
										  0);
	}
}



renderer::renderer( const std::string window_name, size_t w_width,
					size_t w_height, size_t thisRenderer_id, 
					renderer *shared_renderer/* = nullptr*/) {

	//id in rendererInstances map of renderers:
	this->renderer_id = thisRenderer_id;

	//we will use this variable as we begin to iterate through all of the cameras
	//that belong to this renderer, later:
	this->curr_camera = nullptr;


		window *shared_window = shared_renderer == nullptr ? nullptr 
											             : shared_renderer->m_window;
		//shared window can point to an instance, or be nullptr at this point.
		//create our window for this renderer, use shared_window in this process:
		m_window = new window( this, window_name,  w_width, w_height, shared_window);
	
		glfwSetKeyCallback( m_window->glfw_window, 
							input::primary_KeyInputCallback);
		glfwSetCursorPosCallback( m_window->glfw_window,
								  input::primary_cursorInputCallback);	
		glfwSetInputMode(m_window->glfw_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		glfwSetInputMode(m_window->glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);



	for (int t = 0; t < renderTextures::MAX_SIZE; ++t) {
		//create textures and store them in our array:
		//TODO not all of those textures are needed. Don't create the entire range.
		//TODO perhaps GL_RGB8? instead of GL_RGBA8.
		_renderTextures[t] = new texture(t == renderTextures::DEPTH_INDX ? 
													 GL_DEPTH_COMPONENT32 : GL_RGBA8,
										 false,
										 m_window->m_width, m_window->m_height, 0 );

		//set wrap, filtering, 
		_renderTextures[t]->setWrap(GL_CLAMP);
		_renderTextures[t]->setFiltering(GL_NEAREST);
	}


	for (int t = 0; t < lightRenderTextures::MAX_SIZE; ++t) {
		//create light-contribution textures and store them in our array:
		//todo perhaps GL_RGB8? instead of GL_RGBA8.
		_light_renderTextures[t] = new texture( GL_RGBA8, false,
										  m_window->m_width, m_window->m_height, 0 );

		//set wrap, filtering, 
		_light_renderTextures[t]->setWrap(GL_CLAMP);
		_light_renderTextures[t]->setFiltering(GL_NEAREST);
	}
	//generates gFBO, lightFBO and attaches the previously generated textures.
	this->initFrameBuffers();

	//framework::fetch_OGL_errors();

	lightCommitShader = repositories::getShader("lightCommit");
	//setup the uniforms of the shader which will combine light contribution
	//and the colors of the visible scene.
	this->setup_lightCommit_uniforms();

	gizmo_shader = repositories::getShader("standard"); //TODO make debug one


	screenQuad = mesh::createQuad();
	sphereMesh = repositories::getMesh("geosphere");

	_voxelGICore = new voxelGICore(128, 50);
}



renderer::~renderer(){
	delete m_window; //no callbacks will be called on that window.

	////detach attachments of this FBO
	//for (int t = 0; t < renderTextures::MAX_SIZE; ++t) {
	//	glFramebufferTexture(GL_FRAMEBUFFER,
	//		t == renderTextures::DEPTH_INDX ?
	//		GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0 + t,
	//		0, //detach the texture
	//		0  );
	//}
	//glDeleteFramebuffers(1, (GLuint*)&gFBO); //dispose of the frameBufferObject

	//dispose of the screen-render textures
	for (texture *t : _renderTextures) {
		delete t;
	}
	for (texture *t : _light_renderTextures) {
		delete t;
	}

	
	delete screenQuad;
	glDeleteFramebuffers(1, &gFBO);
	glDeleteFramebuffers(1, &lightFBO);

	//don't modify renderInstances map, since it's being cleaned up after this
	//constructor is over.
}


bool renderer::renderAllrenderers(float dt) {
	//TODO check which window has focus with GLFW, render only that window.
	//Render other renderers if you MUST do so (if they are marked like that).
	//Add to camera.h's constructor to make the currently focused renderer 
	//introduced to camera. That way we won't need renderer ids. It would just assign
	//currently focused window's renderer.

	if (rendererInstances.size() == 0) {
		return false;
	}

	//envoke render() on each renderer
	for (auto r = rendererInstances.begin();
								 r != rendererInstances.end();  /*dont++ here*/){
			window *renderer_window = r->second->m_window;

			if(renderer_window){
			//does this renderer have to be deleted?
			if (renderer_window->closed()) {
					//delete this renderer and its window
					int renderer_id = r->second->renderer_id;
					++r; //before we erase from map, let's increment to next content.
					renderer::destroyRenderer(renderer_id);
					continue; //we erased from map, but iterator should be fine.
			}
			
			r->second->renderRenderer(dt);
			}
			++r; //increse iterator manually, as we don't r++. in loop's signature
	}//end for each pair in rendererInstances
	return true;
}


//TODO perhaps this function can be re-used by light's shadowmaps?
void renderer::render_gObject(gameObject *go) { //TODO check if const can be added to argument.
	
	material *_material = go->getMaterial();

	if (!_material) //if there is no material - no point in rendering.
		return;    //return, skipping the drawing of this gameObject.

	_material->draw(this); //checks if it has a mesh, only then draws it.
	
	//bind shader, load uniforms, load lights associated with this gameObject and 
	//it's proximity (light pointers stored in the material)

	//bind mesh
	//render basecolor,  normal+emission,  specular,  depth,  velocity  textures.
	
	//return
}


void renderer::renderRenderer(float dt) {

	//with this renrerer, we will be outputting to its window.
	//TODO: uncomment when doing multi-windows:
	//glfwMakeContextCurrent(m_window->glfw_window);


	//update voxel global illumination structures.
	_voxelGICore->clearVoxels();

	//turn all the triangles into voxels. Notice, the following function requires
	//us to supply all the lights with their shadowmaps ALREADY updated and rdy 2 use
	_voxelGICore->voxelizeMeshes( scene::getCurrentScene()->get_scene_GameObjects(), 
								  scene::getCurrentScene()->get_scene_lights());


	for (camera *cam : cameras) {
		//set the current camera of this renderer to cam:
		this->curr_camera = cam;

		//adjust the aspect according to our window's dimensions:
		vec4 viewport = this->update_camera_aspect(cam);

		//copy the matrices from the camera into the current proj and view matrices
		//of this renderer:
		set_curr_matrices(cam);
			//setup frustum and identify potentially visible objects, which are 
			//in front of the camera:
			frustum *frust = cam->getFrustum();
			//grab all the gameObjects seen by this frustum.True to ignore invisible.
			//Gathers all objects that are inside of the frustum, then sorts the list
			//Only captures the ones which have a bounding shape.
			frust->frustumCapture( cam->m_transform->getPos_world(),  true);
		
			auto frust_contents = frust->get_opaqueContents_asc(); //ascending order

			if (input::getKeyDown(key::KEY_SPACE))
				globals::visualize_voxels = !globals::visualize_voxels;

			if (! globals::visualize_voxels) {
				opaque_geom_pass(viewport, frust_contents);
				lighting_pass();//lighting the gFramebuffer using lights in the scene
			}
			else {
				opaque_geom_pass(viewport, frust_contents);
				//fetch indirect illumination from our voxel global illumination 
				//structure, then output all of it into the back buffer, lighting
				//the visible geometry:
				lighting_voxelGI_pass();
				if (_voxelGICore->shader_control2_value > 0.1) {
					_voxelGICore->displayVoxels(cam, viewport);
				}
			}

			gizmo_pass(viewport); //for debugging of light positions, etc.
		//TODO:
		//now  basecolor,  normal + emission,  specular,  depth,  velocity  textures
		//are ready. Light Up the scene
		
		//do the same for transparent

	}//end for each camera of this renderer

	 //reset the current camera of this renderer, since we've finished iterating 
	//through them:
	this->curr_camera = nullptr;
	//for (int t = 0; t < renderTextures::MAX_SIZE-1; t++) {
	//	_renderTextures[t]->generateMips();
	//} //TODO was generating mipmaps for debugging purpouse
	m_window->output(); //swap buffers and clear COLOR and DEPTH//TODO clear stencil?
}	




vec4 renderer::update_camera_aspect(camera *cam) {

	vec4 out_viewport = cam->viewport_xy_wh;
	//we will either modify the viewport value to fit to the one of the window,
	// or will use the one demanded by the camera.
	
	//if camera didn't want to impose its viewport dimensions - it is fine with us
	//using our own:
	if (!(int)out_viewport.z || !(int)out_viewport.w) {
			out_viewport = vec4(0, 0, m_window->m_width, m_window->m_height);

			// height/width of our viewport:
			float curr_inv_aspect = out_viewport.w / out_viewport.z;

			if (curr_inv_aspect != cam->last_inv_aspect) {
				//update the projection matrix, and re-compute the frustum.
				//Notice, it's important to first set up the last_inv_aspect,
				//because updating proj matrix relies on it.
				cam->last_inv_aspect = curr_inv_aspect;
				cam->updateProjMatrix();
				//this is usefull when the same camera is used to output into 
				//different windows, with varying dimensions.
			}
	}//end if width or height are zero pixels

	return out_viewport;
}



void renderer::set_curr_matrices(camera *cam) {
	this->curr_view_mat4 = cam->viewMat;
	this->curr_proj_mat4 = cam->projMat;
}




void renderer::opaque_geom_pass( vec4 viewport,
								 std::vector<distanceSortedObj> &frust_contents){
	glBindFramebuffer(GL_FRAMEBUFFER, gFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND); //disable alpha blending (drawing opaque objects first)
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	 //TODO check what happens if moved above glBindFramebuffer :)
	glViewport(	(GLint)viewport.x, (GLint)viewport.y,
				(GLsizei)viewport.z, (GLsizei)viewport.w  );


	//render opaque objects FRONT-to-back. (closest are drawn first)
	//for (int  brut = 0; brut < 200; ++brut)
	for (auto captured_obj_itrtr = frust_contents.begin();
		captured_obj_itrtr != frust_contents.end();
		++captured_obj_itrtr) {
				
				render_gObject(captured_obj_itrtr->_gameObject);
	}
}



void renderer::lighting_pass() {
	//establish a shortcut to the lights of the current scene:
	std::vector<light*> lights = scene::getCurrentScene()->get_scene_lights();

	//bind the framebuffer into which we will be outputting all lights contributions:
	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glClearColor(0, 0, 0, 0); //clear empty light-texture with the black color.
	glClear(GL_COLOR_BUFFER_BIT);
	glCullFace(GL_FRONT); //we need to draw the back of light meshes.
	
	glDisable(GL_BLEND);
	//ask each light to assemble its contribution, storing it into 
	//one of our FBO's attachments, which was ALREADY "bound" and "cleared" above.
	//lights might bind their own FBOs for intermediate steps, but will always
	//bind the lightFBO (which we will supply to them), when outputting the final
	//results.
	for (light *_light : lights) {
		_light->draw(this, lightFBO, this->_renderTextures);
	}
	
	glCullFace(GL_BACK);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(lightCommitShader->getShaderProgramID());

	//plug correct textures into the texture units.
	//Texture units were provided during setup_lightCommit_uniforms() function.
	//recall,to get diffuse color we will later have to do:  BASECOLOR*ILLUM
//TODO we probably don't need specular. Especially with how voxels work, it won't be
	//outputted into, most of the time :/
	_light_renderTextures[lightRenderTextures::ILLUM_INDX]->
														 associate_to_TextureUnit(0);
	_light_renderTextures[lightRenderTextures::SPEC_ILLUM_INDX]->
														 associate_to_TextureUnit(1);

	_renderTextures[renderTextures::BASECOLOR_INDX]->associate_to_TextureUnit(2);

	screenQuad->draw_mesh(); //draw mesh as is, in the screen coordinates

	glUseProgram(0);
}



void renderer::lighting_voxelGI_pass() {

	//establish a shortcut to the lights of the current scene:
	std::vector<light*> lights = scene::getCurrentScene()->get_scene_lights();

	//bind the framebuffer into which we will be outputting all lights contributions:
	glBindFramebuffer(GL_FRAMEBUFFER, lightFBO);
	glClearColor(0, 0, 0, 0); //clear empty light-texture with the black color.
	glClear(GL_COLOR_BUFFER_BIT);
	glCullFace(GL_FRONT); //we need to draw the back of light meshes.

	glDisable(GL_BLEND);
	//ask each light to assemble its contribution, storing it into 
	//one of our FBO's attachments, which was ALREADY "bound" and "cleared" above.
	//lights might bind their own FBOs for intermediate steps, but will always
	//bind the lightFBO (which we will supply to them), when outputting the final
	//results.
	for (light *_light : lights) {
		_light->draw(this, lightFBO, this->_renderTextures);
	}



	//bind the framebuffer into which we will be outputting all lights contributions:
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	_voxelGICore->renderSecondBounce(this, 
									 0, 
									 _renderTextures, 
									 _light_renderTextures);


	glUseProgram(0);
}




//typically called once, at the creation of the renderer, telling the 
//lightCommitShader which texture units to use, for which textures.
//However, as the window is resized, renderer might call it to update uniforms like
//"pixelSize"
void renderer::setup_lightCommit_uniforms() {
	glUseProgram(lightCommitShader->getShaderProgramID());


		GLuint loc = glGetUniformLocation(lightCommitShader->getShaderProgramID(),
											"lightDiffuseTex");
		glUniform1i(loc, 0);


		loc = glGetUniformLocation(lightCommitShader->getShaderProgramID(),
									"lightSpecTex");
		glUniform1i(loc, 1);


		loc = glGetUniformLocation(lightCommitShader->getShaderProgramID(),
									"sceneBasecolorTex");
		glUniform1i(loc, 2);


		loc = glGetUniformLocation(lightCommitShader->getShaderProgramID(),
									"pixelSize");
		vec2 pixel_size = vec2(1.f/m_window->getWidth(), 1.f/m_window->getHeight());
		glUniform2fv(loc, 1, (float*)&pixel_size);


	glUseProgram(0);
}



void renderer::gizmo_pass(vec4 viewport) {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);

	glViewport((GLint)viewport.x, (GLint)viewport.y,
			   (GLsizei)viewport.z, (GLsizei)viewport.w);

	GLuint shader_id = gizmo_shader->getShaderProgramID();
	glUseProgram(shader_id);

	std::vector<light*> scene_lights = scene::getCurrentScene()->get_scene_lights();

	for (const light* _light : scene_lights) {
		vec3 pos_world = _light->get_Transform()->getPos_world();
		vec3 scale = vec3(0.25, 0.25, 0.25);

		mat4 model = mat4::translation(pos_world) * mat4::scale(scale);

		GLuint loc = glGetUniformLocation(shader_id, "modelMatrix");
		glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&model);

		loc = glGetUniformLocation(shader_id, "viewMatrix");
		glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&curr_view_mat4);

		loc = glGetUniformLocation(shader_id, "projMatrix");
		glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)&curr_proj_mat4);

		loc = glGetUniformLocation(shader_id, "material_color");
		glUniform4fv(loc, 1, (float*)&vec4(1,1,1,1));

		sphereMesh->draw_mesh();
	}

	glEnable(GL_DEPTH_TEST);

	glUseProgram(0);
}