#include "pointLightGI.h"
#include "repositories.h"
#include "mesh.h"
#include "transform.h"
#include "renderer.h"
#include "window.h"
#include "texture.h"
#include "../gfxmath/vec2.h"




//hidden function, to reduce code  in this .cpp file
//ties the names of the textures in the given shader, with the 
//texture units.
void setup_illumGI_Shader_samplers(GLuint shaderProgram) {
	glUseProgram(shaderProgram);

	//we want to use the parent, pointLight's functionality of uploading uniforms.
	//That fucntion will bebinding scene's depth, normal and light's shadowmap to 
	//units 0,1,2 respectivelly, when the right time comes.
		GLuint loc = glGetUniformLocation(shaderProgram, "scene_normalTex");
		glUniform1i(loc, 0);

		loc = glGetUniformLocation(shaderProgram, "scene_depthTex");
		glUniform1i(loc, 1);


			//Our GI shader pass will NOT calculate any lambert or shadow.
			//That will be done via parent class "pointLight", during a separate pass
		
			/*loc = glGetUniformLocation(shaderProgram, "cubeShadowmapTex");
			glUniform1i(loc, 2);*/


		//after default point light's texture units were bound to the variable names,
		//we can bind our additional variable names to further texture units.
		 loc = glGetUniformLocation(shaderProgram, "light_positionTex");
		glUniform1i(loc, 3);

		loc = glGetUniformLocation(shaderProgram, "light_normalTex");
		glUniform1i(loc, 4);

		loc = glGetUniformLocation(shaderProgram, "light_fluxTex");
		glUniform1i(loc, 5);

	glUseProgram(0);
}





void setup_hq_shading_tex_samplers(GLuint shaderProgram) {
	glUseProgram(shaderProgram);

	GLuint loc = glGetUniformLocation(shaderProgram, "hq_shadingTex");
	glUniform1i(loc, 0);

	 loc = glGetUniformLocation(shaderProgram, "GI_illuminationTex");
	glUniform1i(loc, 1);

	glUseProgram(0);

}



pointLightGI::pointLightGI( GLuint shadowmap_resolution, GLuint GI_resolution,
							vec2 nearFarRange) : pointLight(shadowmap_resolution,
															nearFarRange){

	//generate the cube textures, pointLight already created depth cubemap for us.
	light_PosTex = new texture(GL_RGB16, false, GI_resolution);
	light_NormTex = new texture(GL_RGB8, false, GI_resolution);
	light_FluxTex = new texture(GL_RGBA16, false, GI_resolution); //colors + flux in .w
	light_GIdepth_Tex = new texture(GL_DEPTH_COMPONENT32, false, GI_resolution);


	//the light will "see" all the objects arround it using the shader shadowmapGI.
	//It assembles all the nessesary data to calculate GI in draw().
	shadowmapGI_shader = repositories::getShader("shadowmapGI");

	//helps to calculate the Global Illumination in low-resolution form.
	//It is also used for all the additional, problematic fragments that can't make 
	//use of this shader's outputs.
	illuminationGI_shader = repositories::getShader("GI_illum");



	//applies the avalable full-res lambertian shading and shadowmap texture
	//to the visible part of the scene
	hq_shading_shader = repositories::getShader("hq_shading");

		//setup texture-samplers in the shader, 
		//binding them to appropriate texture units:
		setup_illumGI_Shader_samplers(illuminationGI_shader->getShaderProgramID());
		
	
		setup_hq_shading_tex_samplers(hq_shading_shader->getShaderProgramID());


	
	//resolution won't matter, since it will be adjusted during draw(), as soon as
	//it won't match the current size of the window:
	low_res_GITex = new texture(GL_RGB16, false, 128, 128);
	low_res_GITex->setWrap(GL_CLAMP_TO_EDGE);
	low_res_GITex->setFiltering(GL_NEAREST);
	
	final_highResGITex = new texture(GL_RGB16, false, 128, 128);
	final_highResGITex->setWrap(GL_CLAMP_TO_EDGE);
	final_highResGITex->setFiltering(GL_NEAREST);
	
	high_res_depthStencilTex = new texture(GL_DEPTH24_STENCIL8, false, 128, 128);
	high_res_depthStencilTex->setWrap(GL_CLAMP_TO_EDGE);
	high_res_depthStencilTex->setFiltering(GL_NEAREST);
	
	hq_shading_tex = new texture(GL_RGB16, false, 128, 128);
	hq_shading_tex->setWrap(GL_CLAMP_TO_EDGE);
	hq_shading_tex->setFiltering(GL_NEAREST);


	//create a Framebuffer Object for internal rendering, within this light
	glCreateFramebuffers(1, &internalFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);

	//note that we wil be additing the color attachments and depth attachements
	//to our internalFBO as we need them. For now let's tell it how many color
	//attachments to expect at any instance of time:
	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 }; //TODO allow for sopecularity as well
	glDrawBuffers(1, attachments);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//create a quad that is used in the internal rendering of this light's 
	//GI contribution:
	screenQuad = mesh::createQuad();
}





pointLightGI::~pointLightGI(){
	//delete the textures belonging to THIS class.
	delete light_PosTex;
	delete light_NormTex;
	delete light_FluxTex;

	delete low_res_GITex;
	delete final_highResGITex;
	delete high_res_depthStencilTex;
	delete light_GIdepth_Tex;

	delete hq_shading_tex;

	glDeleteFramebuffers(1, &internalFBO);
}





void pointLightGI::updateShadowmaps(float dt) {
	std::vector<texture*> output_textures;

	//populate it with 3 of our color textures.
	//We WON'T use the shadowmap texture of our parent class, since we have
	//the texture with position relative to this light anyway
	//All of them must be cubemaps, just as they are:
	output_textures.push_back(light_PosTex);
	output_textures.push_back(light_NormTex);
	output_textures.push_back(light_FluxTex);
	//finally, add our own depth texture, whose only purpouse is to allow us to use
	//depth test when rendering out the cube sides. Otherwise the pointLight would
	//substitute its own shadowmap, which would have different sizes to the above 
	//textures. This would make FBO wrong.
	output_textures.push_back(light_GIdepth_Tex);


	GLuint prog = shadowmapGI_shader->getShaderProgramID();
	glUseProgram(prog);

		GLuint loc = glGetUniformLocation(prog, "lightRange");
		glUniform1f(loc, this->get_light_nearFar_range().y);

		 loc = glGetUniformLocation(prog, "lightPos");
		glUniform3fv(loc, 1, (GLfloat*)&this->m_transform->getPos_world());

	//use our parent's function, and our GI-shadowmap shader
	//to output everything into those textures:
	render_into_cube_textures(	output_textures,
								GL_BACK,//  <---cull back faces, NOT front ones
								prog ); // <-- global illumination shadowmap

	//allow the shadowmap that is actually useed for shadows to be re-computed.
	pointLight::updateShadowmaps(dt);

	//TODO remove the following:
	//outputted light's position to see how to remove laggin shadowmapping issue.
	//#include <fstream>
	/*std::ofstream out("lightPos.txt", std::ios::app);
	out << m_transform->getPosition().x << " " << m_transform->getPosition().y
	<< " " << m_transform->getPosition().z<<"\n";*/
}




void pointLightGI::draw( const renderer *curr_renderer, GLuint outputFBO,
						 texture* _renderTextures[renderTextures::MAX_SIZE] ){



	//accquire the lower-resolution global illumination, store it in our internal
	//texture and internal FBO.
	  //DO IT FIRST, since it checks if the window dimensions have changed, then 
	  //resizes ALL the textures belonging to this light.
	draw_low_rez_GIpass(curr_renderer, _renderTextures);

	//Force the parent, "pointLight" part of this class render the lambertian 
	//shading and shadowmap the visible portion of the screen. We will dump 
	//everything into our own, internal FBO's texture - the hq_shading_tex.
	//We will be reading it it much later though, - after we've got all the GI ready.
		glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);
		//to it attach our internal stencil buffer:
		glFramebufferTexture(	GL_FRAMEBUFFER,
								GL_COLOR_ATTACHMENT0,
								this->hq_shading_tex->get_OGL_id(),
								0);

		pointLight::draw(curr_renderer, internalFBO, _renderTextures);

	//output all the fragments (that can recieve interpolated color)  into the output
	//framebuffer. Use stencil test to mark the problematic fragments:
	draw_interpol_GIpass(curr_renderer, _renderTextures);

	//calculate the full-GI for the problematic fragments which couldn't recieve
	//interpolated color:
	draw_delayed_fullGI(curr_renderer, _renderTextures);

	apply_high_quality_shading(curr_renderer, outputFBO);
}





void pointLightGI::draw_low_rez_GIpass(const renderer *curr_renderer,
							texture* _renderTextures[renderTextures::MAX_SIZE]) {

	texture *highResTextures[3] = { final_highResGITex,
									high_res_depthStencilTex, 
									hq_shading_tex };

	vec2 lowResViewport = lowRes_pass_bufferSetup( internalFBO, curr_renderer, 
												  low_res_GITex, highResTextures, 3);

	//setup uniforms for the low-resolution GI shader. 
	//We will have to override a few uniforms afterwards as well, so don't worry.
	pointLightPreDraw_UniformSetup(curr_renderer, _renderTextures, 
									illuminationGI_shader->getShaderProgramID());


	//override pixelSize uniform (was setupt by the above function), 
	//since we want to apply "crude" version of GI to the scene first. 
	//This means producing lower size texture first, then
	//outputting, full-size interpolated version of it.
	//pointLightPreDraw_UniformSetup() already bound the shader program we passed it,
	//so we can keep using it.
	GLuint loc = glGetUniformLocation( illuminationGI_shader->getShaderProgramID(),
										"pixelSize");
	vec2 pixel_size = vec2( 1/lowResViewport.x,    1/lowResViewport.y );
	glUniform2fv(loc, 1, (float*)&pixel_size);

	loc = glGetUniformLocation(illuminationGI_shader->getShaderProgramID(),
					"LOOK HERE");
	//supply additional uniforms to the shader:

	//a size of a pixel in light's cubemaps (search in currently bound shader prog):
	 loc = glGetUniformLocation( illuminationGI_shader->getShaderProgramID(),
								 "lightPixelSize");
	 //cubemap resolution is equal on all dimensions, so we can pick x for example.
	 //Also, any cubemap texture will do, for example the position texture
	glUniform1f(loc, 1.f / light_PosTex->get_xRes());


	//bind additional texture units. Keep in mind that GL_TEXTURE0, GL_TEXTURE1
	//and GL_TEXTURE2 are occupied by scene depth, scene normals textures that
	//were attached a moment ago, when we envoked pointLightPreDraw_UniformSetup()

	//associate the texture of positions RELATIVE to light (in world space) 
	//to the texture unit 3.
	//To unpack them, bring them into NDC from texture space, then multiply 
	//the resulting vectors by lightRange. The outcome will be relative to lightPos.
	light_PosTex->associate_to_TextureUnit(3);
	//associate the scene normals, in the world space, as seen from light 
	//to the texture unit 4
	light_NormTex->associate_to_TextureUnit(4);
	//associate the flux texture (one with scene unlit colors 
	//+ flux value in w component) to the texture unit 5
	light_FluxTex->associate_to_TextureUnit(5);


	//draw the light mesh from the renderer camera's point of view,and work
	//with all the pixels that it covers on the renderer's provided textures.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	//draw the light mesh as seen by the viewr,
	light_mesh->draw_mesh();


	glBindFramebuffer(GL_FRAMEBUFFER, 0); //unbind our internal framebuffer.
	glUseProgram(0);
}





vec2 pointLightGI::lowRes_pass_bufferSetup( GLuint outputFBO,
											const renderer *curr_renderer,
											texture *low_res_outputTex,
											texture **array_of_highResTextures,
											int array_count,
											GLuint approximation/*=8*/){

	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);

	//we need to constantly re-bind our color attachment to this FBO, since
	//we attach other ones later on instead.
	glFramebufferTexture (GL_FRAMEBUFFER,
						  GL_COLOR_ATTACHMENT0,
						  low_res_outputTex->get_OGL_id(),
						  0 );
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST); //haven't got those attachments anyway.



	float viewport_width = (int)curr_renderer->get_window()->getWidth();
	float viewport_height = (int)curr_renderer->get_window()->getHeight();


	float viewport_scaled_width = viewport_width / approximation;
	float viewport_scaled_height = viewport_height / approximation;

	//check if the new dimensions don't fit the ones that were previously
	//recorded (the window size or the approximation coefficient have changed):
	if (    array_count == 0 
		 || low_res_outputTex->get_xRes() != viewport_scaled_width
		 || low_res_outputTex->get_yRes() != viewport_scaled_height) {

		//reload the low res GI texture to OpenGL with its new sizes.
		low_res_outputTex->resize(viewport_scaled_width, viewport_scaled_height, 0);


		if (array_count > 0) {
			for (int high_resTex_ix = 0;
				 high_resTex_ix < array_count; 
				 ++high_resTex_ix){

				array_of_highResTextures[high_resTex_ix]->resize(viewport_width,
																 viewport_height,
																 0);
			}//end for-loop
		}
	}//end if resoltutions must be changed

	//finally set up the viewport too.
	glViewport(0, 0, viewport_scaled_width, viewport_scaled_height);


	//DON'T UNBIND THE FBO 

	return vec2(viewport_scaled_width, viewport_scaled_height);
}





void pointLightGI::draw_interpol_GIpass(const renderer *curr_renderer,								
								texture* _renderTextures[renderTextures::MAX_SIZE]){


	
	interpol_pass_bufferSetup( internalFBO, 
							   this->final_highResGITex, 
							   this->high_res_depthStencilTex );
	

	mat4 lightModelMatrix; //we won't use m_transform, since we use range as scale
	float light_range = this->old_near_far_range.y;
	//orientation will always be (0,0,0) for point lights
	mat4 translation = mat4::translation(m_transform->getPos_world());
	mat4 scale = mat4::scale(vec3(light_range));
	lightModelMatrix = translation * scale;
	//TODO make the above Model matrix a member variable of point light class.

	mat4 lightMVP =    curr_renderer->get_curr_proj_mat4() 
					    * curr_renderer->get_curr_view_mat4()
						* lightModelMatrix;
	//supply all the uniforms to this shader.
	//Ipload light's mesh MVP matrix, bind appropriate textures and send 
	//uniforms that are needed to perform GI interpolation:

	interpol_uniforms_setup(lightMVP, low_res_GITex, curr_renderer, _renderTextures);


			 //the above call KEPT the interpolation shader bound,
			 //so we can continue working with it.


	//draw the light mesh as seen by the viewr,
	//interpolating the GI colors where possible.
	//Front faces are culled (remained switched on from
	//low-res GI pass stage)
	light_mesh->draw_mesh();

	//we will detach our stencil attachment from internal FBO in a later stage,
	//at the end of draw_delayed_fullGI().
	//It will still have to work with the stencil attachment so don't detach it yet.

	glUseProgram(0);
}



void pointLightGI::interpol_pass_bufferSetup(GLuint outputFBO, 
										     texture *highRes_outputTex,
											 texture *highRes_depthStencilTex) {

	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);

	//to it attach our internal stencil buffer (we will detach it later in this func)
	glFramebufferTexture( GL_FRAMEBUFFER,
						  GL_STENCIL_ATTACHMENT,
						  highRes_depthStencilTex->get_OGL_id(),
						  0 );
	//and the texture to output to:
	glFramebufferTexture( GL_FRAMEBUFFER,
						  GL_COLOR_ATTACHMENT0,
						  highRes_outputTex->get_OGL_id(),
						  0 );

	glViewport(0, 0, highRes_outputTex->get_xRes(), highRes_outputTex->get_yRes());



	glDisable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	//we are allowed to clear  only the stencil of the output buffer.
	//Do so, since our stencil will mark-out problematic fragments that need 
	//a full GI computation, since they can't accquire interpolated value from
	//the low-res GI texture:
	glClearStencil(0); //will be clearing all to zero.
	glClear(GL_STENCIL_BUFFER_BIT);

	//as we draw the high pass, we want the stencil to pass, marking the fragment,
	//unless the shader "discards" the fragment manually.
	//Later, all the fragments remaining at 0 will be re-drawn with the full
	//GI computation.
	glStencilFunc(GL_ALWAYS, 1, ~0);
	//tell what should happen when the stencil + depth fails, when the depth fails,
	//or when both stencil + depth pass the test.
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE); //replace with 1s.


	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	//DON'T UNBIND THE FBO
}





void pointLightGI::interpol_uniforms_setup(mat4 MVP, texture *low_res_texture,
											const renderer *curr_renderer, 
								texture* _renderTextures[renderTextures::MAX_SIZE]){


	shader *interpolationShader = repositories::getShader("screenInterpolation");
	//bind the GI-interpolation shader 
	glUseProgram(interpolationShader->getShaderProgramID());



	//upload MVP for this light
	GLint loc =glGetUniformLocation(interpolationShader->getShaderProgramID(),"MVP");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&MVP);


	//upload the width and height of the low-resolution GI texture:
	loc = glGetUniformLocation(interpolationShader->getShaderProgramID(),
								"lowResNumTexels");
	glUniform2fv( loc, 1,
				  (float*)&vec2(low_res_texture->get_xRes(),
								low_res_texture->get_yRes())  );



	//upload the dimensions of a single pixel of the high-resolution output textures:
	loc  = glGetUniformLocation(interpolationShader->getShaderProgramID(),
								"highRes_PixelSizes");
	vec2 high_rez_pixelSizes = vec2(1.0f / curr_renderer->get_window()->getWidth(),
							  1.0f / curr_renderer->get_window()->getHeight());
	glUniform2fv(loc, 1, (float*)&high_rez_pixelSizes);


	//upload the near & far clipping planes of the renderer's viewing frustum:
	loc = glGetUniformLocation(interpolationShader->getShaderProgramID(),
								"nearFarRange");
	glUniform2fv( loc,   1, 
				(float*)&curr_renderer->getCurrentCamera()->getNearFar_ClipPlanes());




	//setup texture-samplers in this light's GI INTERPOLATION shader, 
	//binding them to appropriate texture units:
	loc = glGetUniformLocation(interpolationShader->getShaderProgramID(),
										"scene_depthTex");
	glUniform1i(loc, 0);

	loc = glGetUniformLocation(interpolationShader->getShaderProgramID(), 
								"lowRes_Tex");
	glUniform1i(loc, 1);

	loc = glGetUniformLocation(interpolationShader->getShaderProgramID(), 
								"highRes_NormTex");
	glUniform1i(loc, 2);

	//setup the textures used in the interpolation process:

	//associate the provided texture of scene's depth with texture unit 0
	_renderTextures[renderTextures::DEPTH_INDX]->associate_to_TextureUnit(0);
	//associate low_res_GI to texture unit1
	low_res_texture->associate_to_TextureUnit(1);
	//normals and emission factor (self-glow) in alpha to texture unit 2:
	_renderTextures[renderTextures::NORMAL_AND_EMISSION_INDX]
													->associate_to_TextureUnit(2);

	//for the interpolation to work, filtering must be set to linear on our
	//low-resolution texture (since we will upsample it)
	low_res_texture->setFiltering(GL_LINEAR);



	//we DON'T unbind the interpolation shader, so it can be worked with 
	//straight after this funciton was called.
}






void pointLightGI::draw_delayed_fullGI(const renderer *curr_renderer,
								texture *_renderTextures[renderTextures::MAX_SIZE]) {
	//the output framebuffer is still bound from the interpolation pass, so there is 
	//no need to bind it once again.
	//Stencil test is still enabled from the draw_interpol_GIpass().
	//However, we need to change its comparison functions.
	glStencilFunc(GL_EQUAL, 0, ~0);
	//tell what should happen when the stencil + depth fails, when the depth fails,
	//or when both stencil + depth pass the test.
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE); //replace with 1s.

	 //so as soon as the stencil has the value of 0, the corresponding fragment
	 //will have it's value recomputed with the expensive GI.
	glUseProgram(illuminationGI_shader->getShaderProgramID());

	//viewport is also having the final dimensions, from the interpolation passs.


	//setup uniforms for the GI shader. 
	//We will have to override a few uniforms afterwards as well, so don't worry.
	pointLightPreDraw_UniformSetup(curr_renderer, _renderTextures,
									illuminationGI_shader->getShaderProgramID());


	//supply additional uniforms to the shader:

	//a size of a pixel in light's cubemaps (search in currently bound shader prog):
	GLuint loc = glGetUniformLocation(	illuminationGI_shader->getShaderProgramID(),
										"lightPixelSize");

	//cubemap resolution is equal on all dimensions, so we can pick x for example.
	//Also, any cubemap texture will do, for example the position texture
	glUniform1f(loc, 1.f / light_PosTex->get_xRes());


	//bind additional texture units. Keep in mind that GL_TEXTURE0, GL_TEXTURE1
	//and GL_TEXTURE2 are occupied by scene depth, scene normals textures that
	//were attached a moment ago, when we envoked pointLightPreDraw_UniformSetup()

	//associate the texture of positions RELATIVE to light (in world space) with 
	//texture unit 3.
	//To unpack them, bring them into NDC from texture space, then multiply 
	//the resulting vectors by lightRange. The outcome will be relative to lightPos.
	light_PosTex->associate_to_TextureUnit(3);
	//associate the scene normals, in the world space, as seen from light with 
	//texture unit 4
	light_NormTex->associate_to_TextureUnit(4);
	//associate the flux texture (one with scene unlit colors with texture unit 5
	// + flux value in w component)
	light_FluxTex->associate_to_TextureUnit(5);


	//draw the light mesh from the renderer camera's point of view,and work
	//with all the pixels that it covers on the renderer's provided textures.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glViewport(0, 0, curr_renderer->get_window()->getWidth(),
				curr_renderer->get_window()->getHeight());


	//draw the light mesh as seen by the viewr.
	//Front faces are culled (remained switched on from
	//low-res GI pass stage)
	light_mesh->draw_mesh();


	glDisable(GL_STENCIL_TEST);
	glUseProgram(0);


	//detach our stencil attachment from internalFBO (we bound it during
	//draw_low_rez_GIpass()  ), to avoid later confusions:
	glFramebufferTexture(	GL_FRAMEBUFFER,
							GL_STENCIL_ATTACHMENT,
							0,
							0);


	//unbind internalFBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}






void pointLightGI::apply_high_quality_shading(const renderer *curr_renderer, 
												GLuint outputFBO ) {
	
	//we no longer want to work with the internal framebuffer.
	//All of the outputs will be returned into the outputFBO, which belongs to 
	//the renderer.
	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);


	GLuint currProg = hq_shading_shader->getShaderProgramID();
	glUseProgram(currProg);


	
	//on the 0th sampler we will stick a texture with high-quality lambertian and 
	//shadowmapped visible portion of the scene:
	hq_shading_tex->associate_to_TextureUnit(0);

	final_highResGITex->associate_to_TextureUnit(1);

	//shader sampler variable names already were associated with a numerical value
	//during the constructor. We can safelly bind our hq_shading_tex and the light's
	//GI output texture to units Zero and One, just like we did above.
	

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
	GLint loc = glGetUniformLocation(currProg, "MVP");
	glUniformMatrix4fv(loc, 1, GL_FALSE,(float*)&(renderer_VP_mat*lightModelMatrix));


	//upload the size of one pixel (we want the dimensions to fit into 0-1 range)
	loc = glGetUniformLocation(currProg, "pixelSize");
	float w_width = curr_renderer->get_window()->getWidth();
	float w_height = curr_renderer->get_window()->getHeight();
	vec2 pixel_size = vec2(1 / w_width, 1 / w_height);
	glUniform2fv(loc, 1, (float*)&pixel_size);


	glCullFace(GL_FRONT); 

	glViewport(0, 0, curr_renderer->get_window()->getWidth(),
				curr_renderer->get_window()->getHeight());

	this->light_mesh->draw_mesh();


	glCullFace(GL_BACK); //revert to default face culling
	glUseProgram(0);
}