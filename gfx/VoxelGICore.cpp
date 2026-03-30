#include "VoxelGICore.h"
#include "texture.h"
#include "repositories.h"
#include "gameObject.h"
#include "shader.h"
#include "material.h"
#include "mesh.h"
#include "camera.h"
#include "light.h"
#include "../gfxmath/mat4.h"
#include "renderer.h"
#include "window.h"
#include "pointLightGI.h" //for re-using its interpolation functions


voxelGICore::voxelGICore(size_t GI_textures_res, float world_size){
	
	//create 4 textures that will be used by the voxel GI algorithm.
	//TODO make the textures sparse.
	basecolor_tex3D = new texture(GL_RGBA16F, false, GI_textures_res,
								  GI_textures_res, GI_textures_res);

	diffuse_tex3D = new texture(GL_RGBA16F, true, GI_textures_res,
										GI_textures_res, GI_textures_res);
	normal_and_brighness_tex3D = new texture(GL_RGBA16F, false, GI_textures_res,
											 GI_textures_res, GI_textures_res);

	bounce_and_brightness_tex3D = new texture(GL_RGBA16F, true, GI_textures_res,
											  GI_textures_res, GI_textures_res);

	//texture used for averaging the voxel colors.
	//Notice it's unsigned integer, used for counting how many triangle fell into
	//the voxel.
	count_texture = new texture( GL_R32UI, false, GI_textures_res, GI_textures_res,
								 GI_textures_res );


	//create a texture which is rendered at the correct resolution from one
	//of the points of view, during voxelization stage.
	//Triangles are then restored and are inserted in one of our 3d texture, 
	//in exact correct position:
	raster_tex2D = new texture(GL_R8, false, GI_textures_res, GI_textures_res);

	//To speed up rendering, GI will be computed for smaller resolution and
	//interpolation will take place. The following 3 textures will be used for that:
	low_res_GITex = new texture(GL_RGBA8, false, 1, 1);
	final_highResGITex = new texture(GL_RGBA8, false, 1, 1);
	high_res_depthStencilTex = new texture(GL_DEPTH24_STENCIL8, false, 1, 1);



	basecolor_tex3D->setFiltering(GL_LINEAR_MIPMAP_LINEAR);//filtering will change
	diffuse_tex3D->setFiltering(GL_LINEAR_MIPMAP_LINEAR); //later depending on
	normal_and_brighness_tex3D->setFiltering(GL_NEAREST); //circumstances
	bounce_and_brightness_tex3D->setFiltering(GL_LINEAR_MIPMAP_LINEAR);
	count_texture->setFiltering(GL_NEAREST);//required for clearing function to work.

	raster_tex2D->setFiltering(GL_NEAREST);
	low_res_GITex->setFiltering(GL_NEAREST);
	final_highResGITex->setFiltering(GL_NEAREST);
	high_res_depthStencilTex->setFiltering(GL_NEAREST);


	basecolor_tex3D->setWrap(GL_CLAMP_TO_EDGE);
	diffuse_tex3D->setWrap(GL_CLAMP_TO_EDGE);
	normal_and_brighness_tex3D->setWrap(GL_CLAMP_TO_EDGE);
	bounce_and_brightness_tex3D->setWrap(GL_CLAMP_TO_EDGE);

	raster_tex2D->setWrap(GL_CLAMP_TO_EDGE);
	low_res_GITex->setWrap(GL_CLAMP_TO_EDGE);
	final_highResGITex->setWrap(GL_CLAMP_TO_EDGE);
	high_res_depthStencilTex->setWrap(GL_CLAMP_TO_EDGE);


	//record world-size of the texture set:
	datastructure_size_world = world_size;
	

	//fetch the shader that will be used in outputting voxels in 3D texture.
	//It will be working with base color, 
	//diffuse
	//as well as with normals:
	voxelization_shader = repositories::getShader("voxelizationGI");
	modern_voxelization_shader = repositories::getShader("modernVoxelizeGI");
	modern_voxelizeAverage_shader=repositories::getShader("modernVoxelizeGIAverage");

	displayVoxel_shader = repositories::getShader("voxelGI_marching_visualize");
	filter_1st_bounce_shader = repositories::getShader("lightFilterGI");
	renderSecondBounce_shader = repositories::getShader("renderSecondBounceGI");


	glCreateFramebuffers(1, &internalFBO);

	screen_quad = mesh::createQuad();

	//associate the variable names inside the displaying shader program, to the 
	//corresponding texture units:
	glUseProgram(displayVoxel_shader->getShaderProgramID());
	GLuint loc = glGetUniformLocation(displayVoxel_shader->getShaderProgramID(),
										"voxelVolumeTex");
	glUniform1i(loc, 0);


	//and for the modern voxelize shader, which outputs voxels into a 3D texture 
	glUseProgram(modern_voxelization_shader->getShaderProgramID());
	loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
								"basecolorTex_output");
	glUniform1i(loc, 0);
	 
	loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
								"diffuseTex_output"); 
	glUniform1i(loc, 1);

	loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
								"normal_and_brightness_Tex_output");
	glUniform1i(loc, 2);

	loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
								"countTex_inout");
	glUniform1i(loc, 3);



	//color should be averaged. rgba8 
	glUseProgram(modern_voxelizeAverage_shader->getShaderProgramID());
	loc = glGetUniformLocation(modern_voxelizeAverage_shader->getShaderProgramID(),
								"basecolorTex");
	glUniform1i(loc, 0);

	loc = glGetUniformLocation(modern_voxelizeAverage_shader->getShaderProgramID(),
								"diffuseTex");
	glUniform1i(loc, 1);

	loc = glGetUniformLocation(modern_voxelizeAverage_shader->getShaderProgramID(),
								"normal_and_brightness_Tex");
	glUniform1i(loc, 2);

	loc = glGetUniformLocation(modern_voxelizeAverage_shader->getShaderProgramID(),
								"countTex_inout");
	glUniform1i(loc, 3);



	//associate the variable names with texture units inside 1st-bounce light filter
	//shader:
	glUseProgram(filter_1st_bounce_shader->getShaderProgramID());

	loc = glGetUniformLocation(filter_1st_bounce_shader->getShaderProgramID(),
								"basecolorTex");
	glUniform1i(loc, 0);

	loc = glGetUniformLocation(filter_1st_bounce_shader->getShaderProgramID(),
							   "diffuseTex");
	glUniform1i(loc, 1);


	loc = glGetUniformLocation(filter_1st_bounce_shader->getShaderProgramID(),
								"normal_and_brightnessTex");
	glUniform1i(loc, 2);

	//setup "texture unit 3" to be for the texture into which 1-st bounce GI light 
	//gets outputted:
	loc = glGetUniformLocation(filter_1st_bounce_shader->getShaderProgramID(),
								"lightBounceTex_output");
	glUniform1i(loc, 3);

	//setup texture unit 4 to be for the red-green-blue noise 2D texture in the 
	//1-st bounce GI light shader:
	loc = glGetUniformLocation(filter_1st_bounce_shader->getShaderProgramID(),
								"rgb_noiseTex");
	glUniform1i(loc, 4);




	glUseProgram(renderSecondBounce_shader->getShaderProgramID());
	//setupt "texture unit 3" to be for the texture into which 1-st bounce GI light 
	//gets outputted:
	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"basecolorTex");
	glUniform1i(loc, 0);

	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"diffuseTex");
	glUniform1i(loc, 1);

	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"normalTex");
	glUniform1i(loc, 2);

	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"bounce_and_brightness_Tex");
	glUniform1i(loc, 3);


	//also associate "render screen deferred texture" variable names to 4,5,6,7
	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"scene_baseColorTex");
	glUniform1i(loc, 4);

	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"scene_depthTex");
	glUniform1i(loc, 5);

	//screen space normals with "self-glow" factor in alpha.
	//Associate them to texture unit 6
	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"scene_normal_and_emissionTex");
	glUniform1i(loc, 6);

	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"scene_illumTex");
	glUniform1i(loc, 7);

	loc = glGetUniformLocation(renderSecondBounce_shader->getShaderProgramID(),
								"rgb_noiseTex");
	glUniform1i(loc, 8);



	shader_control_value = 1;
	shader_control2_value = 0;
	shader_control3_value = 0;
	shader_control4_value = 0;
	shader_control5_value = 12;

	glUseProgram(0);
}





voxelGICore::~voxelGICore(){
	delete basecolor_tex3D;
	delete diffuse_tex3D;
	delete normal_and_brighness_tex3D;
	delete bounce_and_brightness_tex3D;
	delete count_texture;

	delete raster_tex2D;

	delete low_res_GITex;
	delete final_highResGITex;
	delete high_res_depthStencilTex;

	//TODO detach any textures
	glDeleteFramebuffers(1, &internalFBO);

	delete screen_quad;
}




void voxelGICore::voxelizeMeshes(std::vector<gameObject*> gobjects, 
								 std::vector<light*> lights_with_shadowmaps) {

	if(GLEW_VERSION_4_3){
		//TODO check if extension is supported:
		modernVoxelize(gobjects, lights_with_shadowmaps);
	}
	//TODO code for previous OGL versions?
}




void voxelGICore::modernVoxelize( std::vector<gameObject*> gobjects,
								  std::vector<light*> lights_with_shadowmaps) {


	glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);

	//attach the rasterization texture. It's size will determine resolution of voxels
	//that we will be later outputting into our 3D texture via OGL's 4.3 scattered
	//image writes.
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						  raster_tex2D->get_OGL_id(), 0 );

	//tell where the frambuffer should output (even though we will not care about
	//output to raster texture, it's only used to generate voxels):
	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	raster_tex2D->setFiltering(GL_NEAREST);

	//clear our raster texture:
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport( 0, 0, //bot left corner
			   (GLint)raster_tex2D->get_xRes(), //width
			   (GLint)raster_tex2D->get_yRes()  ); //height
			   

	 //NOTICE that we don't clear 3D textures here. For that to happen user has to 
	 //explicitly call  VoxelGICore::clearGeometryVoxels() 


	 //VERY IMPORTANT TO SUPPLY the 3d "textureSize" (how large is the volume in 
	 // the world and "texelSize" to the shader.
	 //This will allow us to calculate the texel coordinate in our ouput 3d texture.
	glUseProgram(modern_voxelization_shader->getShaderProgramID());
	GLuint loc=glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
									 "textureSize");
	glUniform1f(loc, datastructure_size_world);


	loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
								"texelSize");
	glUniform1f(loc, datastructure_size_world / diffuse_tex3D->get_zRes());

	loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
								"control");
	glUniform1f(loc, shader_control_value);

		loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
								"control2");
	glUniform1f(loc, shader_control2_value);

	//Let's sort out uniforms with lights in the folowing 20 lines:

	//extract all the required information from the supplied light sources, so we
	//can use it inside the shader:
	std::vector<vec4> light_pos_andRange; //position and radius of each light
	std::vector<vec3> light_colors;
	 
	 
	for (light *_light : lights_with_shadowmaps) {
		light_pos_andRange.push_back(vec4( _light->get_Transform()->getPosition(),
										   _light->get_light_nearFar_range().y)    );

		light_colors.push_back(vec3(_light->get_color()));
	}
	//make sure to stick a final vector with a range of "-1". 
	//as soon as attenuation radius of "-1" is detetected, or the last (6th)
	//index is reached by the shader, we know that there are no more lights supplied.
	light_pos_andRange.push_back(vec4(0, 0, 0, -1));
	light_colors.push_back(vec3(0));

		//now, supply the data from those three collections into the shader
		loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
									"light_pos_andRange");
		//we are allowed to supply a maximum of 6 lights:
		//(we will add +1 if there were 5 or less lights, to allow for 1 extra entry,
		//which has "-1" attenuation radius, marking the end of array.
		int uniformArrayLength = lights_with_shadowmaps.size() < 6 ?
													lights_with_shadowmaps.size()+1
													: 6;
		//TODO check if no lights would be supplied, then 0th index would throw error
		glUniform4fv(loc, uniformArrayLength, (float*)&light_pos_andRange[0]);


	//supply a sequence of colors for our lights:
	loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
							   "light_colors");
	glUniform3fv(loc, uniformArrayLength, (float*)&light_colors[0]);

		//supply a sequence of shadowmaps for our lights:
		loc = glGetUniformLocation(modern_voxelization_shader->getShaderProgramID(),
			                       "light_cube_distTex");
		std::vector<int> texture_units = { 5,6,7,8,9,10 };
		glUniform1iv(loc, uniformArrayLength, (GLint*)&texture_units[0]);

	//associate each shadowmap texture to the correct texture unit (5,6,7 ... 10)
	for (int t = 0; t < lights_with_shadowmaps.size(); ++t) {
		lights_with_shadowmaps[t]->get_shadowmapTexture()
													 ->associate_to_TextureUnit(t+5);
	}

	//ok, done with setting up light-related uniforms
	basecolor_tex3D->setFiltering(GL_NEAREST);
	diffuse_tex3D->setFiltering(GL_NEAREST);
	normal_and_brighness_tex3D->setFiltering(GL_NEAREST);
	//bind output 3d textures to texture units 0,1,2:
	//since we will be performing atomip "add" operations, we should set
	//GL_READ_WRITE flags on all of them.
	//Afterwards, we will have to average them out by dividing by "count" - the
	//number of times each voxel was written to.
			glBindImageTexture(0, basecolor_tex3D->get_OGL_id(), 0, GL_TRUE,
						   0, GL_READ_WRITE, basecolor_tex3D->get_internal_format());

			glBindImageTexture(1, diffuse_tex3D->get_OGL_id(), 0, GL_TRUE,
							0, GL_READ_WRITE, diffuse_tex3D->get_internal_format());

			glBindImageTexture(2, normal_and_brighness_tex3D->get_OGL_id(),0,GL_TRUE,
							   0, GL_READ_WRITE,
							   normal_and_brighness_tex3D->get_internal_format());
			
			glBindImageTexture(3, count_texture->get_OGL_id(), 0, GL_TRUE,
							   0, GL_READ_WRITE,
							   count_texture->get_internal_format());
	
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND); //TODO later allow for averaging of incoming colors. 
						 //Disabled for now
	
	//make sure we generate voxel even if the pixel center was not covered by 
	//triangle. It will still generate a fragment (makes our geometry look fatter):
	glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
	

	for (gameObject *go : gobjects) {
		material *_material = go->getMaterial();

		if (!_material) {//if there is no material - no point in rendering.
			continue;    //skip the drawing of this gameObject.
		}
			//we don't construct or upload view + projection matrices since they
			//are created within geometry shader (for each triangle).

			//checks if it has a mesh, only then draws it:
			//TODO currently this list includes game objects without meshes 
			//like cameras, etc.
		_material->draw_custom(modern_voxelization_shader->getShaderProgramID());
	}
	
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//clean-up:
	glEnable(GL_CULL_FACE); //clean-up and re-enable everything
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
	//except for blending (TODO)

	glUseProgram(0);

	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						  0, 0); //detach rasterization texture attachment
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//average out the added colors:
	voxelizeAverage();

		//diffuse_tex3D contains "light colors and shadowing" in rgb and 
		//transparency in its alpha 
	
		diffuse_tex3D->generateMips(shader_control_value);
		//no need to generate mipmaps for normal maps or basecolor
		//since we will be calculating 1st bounce only needing their 0th mip level. 
		//We'll only be interested in diffuse of neighbors, and that one is mipmepped
		//For 2nd bounce we will not be using mips of voxels 
		//corresponding to the visible pixel either.  TODO is this true?

	//re-enable writing/reading from the mipmaps for appropriate textures:
	basecolor_tex3D->setFiltering(GL_LINEAR_MIPMAP_LINEAR);
	diffuse_tex3D->setFiltering(GL_LINEAR_MIPMAP_LINEAR);
	normal_and_brighness_tex3D->setFiltering(GL_NEAREST);

	//calculate 1st GI bounce and store it in a 3d texture. 
	//this function will mip map it as well, "filtering" the light "upwards" along
	//the mip chain:
	filterInto_1st_bounce();
}



void voxelGICore::voxelizeAverage() {
	
	glUseProgram(modern_voxelizeAverage_shader->getShaderProgramID());

	GLuint loc = glGetUniformLocation(
								modern_voxelizeAverage_shader->getShaderProgramID(),
								"control");
	glUniform1f(loc, shader_control_value);

	loc = glGetUniformLocation(modern_voxelizeAverage_shader->getShaderProgramID(),
								"control2");
	glUniform1f(loc, shader_control2_value);

	//bind output 3d textures to texture units 0,1,2:
	//we will have to average them out by dividing by "count" - the
	//number of times each voxel was written to.
			glBindImageTexture(0, basecolor_tex3D->get_OGL_id(), 0, GL_TRUE,
							0, GL_READ_WRITE, basecolor_tex3D->get_internal_format());

			glBindImageTexture(1, diffuse_tex3D->get_OGL_id(), 0, GL_TRUE,
							0, GL_READ_WRITE, diffuse_tex3D->get_internal_format());

			glBindImageTexture(2, normal_and_brighness_tex3D->get_OGL_id(),0,GL_TRUE,
							   0, GL_READ_WRITE,
							   normal_and_brighness_tex3D->get_internal_format());
			
			count_texture->associate_to_TextureUnit(3);
			
		//TODO allow for non-cube dimentions?
		glDispatchCompute(	count_texture->get_xRes(),//that many work 
							count_texture->get_xRes(),//units, as much as 
							count_texture->get_xRes() //there  are voxels 
						 );								//in our 3d volume
	
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


	glUseProgram(0);
}





void voxelGICore::filterInto_1st_bounce() {
	//TODO what if not supported?
	//TODO move it to happen earierl - basically all of voxel GI should be disabled
	//if 4.3 is not supported. not just this function.
	if (GLEW_VERSION_4_3) {

		glUseProgram(filter_1st_bounce_shader->getShaderProgramID());

		if (input::getKey(key::KEY_1)) { //starts from 1
			shader_control_value -= 0.1;
			shader_control_value = std::fmaxf(shader_control_value, 0);
		}
		if (input::getKey(key::KEY_2)) {//starts from 1
			shader_control_value += 0.1;
			//shader_control_value = std::fminf(shader_control_value, 1);
		}
		if (input::getKey(key::KEY_3)) {
			shader_control2_value -= 0.01;
		}
		if (input::getKey(key::KEY_4)) {
			shader_control2_value += 0.01;
		}

		if (input::getKey(key::KEY_5)) {
			shader_control3_value -= 0.1;
			shader_control3_value = std::fmaxf(shader_control3_value, 0);
		}
		if (input::getKey(key::KEY_6)) {
			shader_control3_value += 0.1;
		}
		if (input::getKey(key::KEY_7)) {
			shader_control4_value -= 0.1;
			shader_control4_value = std::fmaxf(shader_control4_value, 0);
		}
		if (input::getKey(key::KEY_8)) {  //starts from 12
			shader_control4_value += 0.1;
			shader_control4_value = std::fminf(shader_control4_value, 1);
		}
		if (input::getKey(key::KEY_9)) { //starts from 12
			shader_control5_value -= 0.1;
			shader_control5_value = std::fmaxf(shader_control5_value, 0);
		}
		if (input::getKey(key::KEY_0)) {
			shader_control5_value += 0.1;
		}

		GLuint loc = glGetUniformLocation(filter_1st_bounce_shader
												->getShaderProgramID(), "numTexels");
		glUniform1i(loc, bounce_and_brightness_tex3D->get_xRes());

		 loc = glGetUniformLocation(filter_1st_bounce_shader
												->getShaderProgramID(), "control");
		glUniform1f(loc, shader_control_value);

		 loc = glGetUniformLocation(filter_1st_bounce_shader
												->getShaderProgramID(), "control2");
		glUniform1f(loc, shader_control2_value);

		std::cout << "\n shader_control_value " << shader_control_value << " \n";
		std::cout << " shader_control_2_value " << shader_control2_value << " \n";
		std::cout << " shader_control_3_value " << shader_control3_value << " \n";
		std::cout << " shader_control_4_value " << shader_control4_value << " \n";
		std::cout << " shader_control_5_value " << shader_control5_value << " \n";
		
		//make sure DIFFUSE and basecolor filtering is "linear" and
		//"linear mipmap linear" respectively.
		//This will allow for a trilinear and quadralinear filtering to occur:
		basecolor_tex3D->setFiltering(GL_LINEAR);
		diffuse_tex3D->setFiltering(GL_LINEAR_MIPMAP_LINEAR);
		//all the variable names in shader were already associated with the texture
		//units (during voxelGICOre constructor). We can safely bind our textures
		//to those units.
		//Notice that normal and basecolor are NEAREST, since we only need to
		//sample normal for the current voxel. 
		//During cone tracing we don't care about other normals,
		//at least at this stage:
		normal_and_brighness_tex3D->setFiltering(GL_LINEAR);

		//bind all the texture samplers 
		basecolor_tex3D->associate_to_TextureUnit(0);//"raw" material color +transpar
		diffuse_tex3D->associate_to_TextureUnit(1);//shadowing and transparency
		normal_and_brighness_tex3D->associate_to_TextureUnit(2);
		

		//our output texture as well:
		bounce_and_brightness_tex3D->setFiltering(GL_NEAREST);
		//as for our output texture, we need to disable any filtering or mipmaps
		//we bind to texture unit 3, as it was set up 
		//inside boxelGICore's constructor):
		glBindImageTexture( 3, bounce_and_brightness_tex3D->get_OGL_id(),  0,  GL_TRUE,
							0, GL_WRITE_ONLY, 
							bounce_and_brightness_tex3D->get_internal_format());

		//associate our 2D color noise texture to unit 4:
		//it will allow us to get random rotation angle.
		repositories::getTexture("rgb_noise")->associate_to_TextureUnit(4);


		//TODO allow for non-cube dimentions?
		glDispatchCompute(	bounce_and_brightness_tex3D->get_xRes(),//that many work 
							bounce_and_brightness_tex3D->get_xRes(),//units, as much as 
							bounce_and_brightness_tex3D->get_xRes() //there  are voxels 
						 );								//in our 3d volume

		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		//that's a second (and last) texture that has to be mip-mapped:
		bounce_and_brightness_tex3D->generateMips(shader_control_value );
		//clean up:
		glUseProgram(0);
	}//end if OGL 4.3 is supported
}





void voxelGICore::clearVoxels() {
	//reset all the texels in  3Dtextures to zeros.

	glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						  basecolor_tex3D->get_OGL_id(), 0);
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
						  diffuse_tex3D->get_OGL_id(), 0);
	
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
						  normal_and_brighness_tex3D->get_OGL_id(), 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
						  bounce_and_brightness_tex3D->get_OGL_id(), 0);
	//TODO we only cleared level 0. Need to clear all mip levels though

	//TODO attachments might not be required just for clearing the color.
		GLuint attachments[4] = { GL_COLOR_ATTACHMENT0,
								  GL_COLOR_ATTACHMENT1,
								  GL_COLOR_ATTACHMENT2,
								  GL_COLOR_ATTACHMENT3};
		glDrawBuffers(4, attachments);

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						  0, 0); //detach basecolor attachment
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
						  0, 0); //detach difffuse attachment
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
						  0, 0); //detach normal attachment
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
						  0, 0); //detach 1st light bounce attachment

	//make sure our uint texture has NEAREST filtering.
	//Otherwise clearing it might produce undefined results
	count_texture->setFiltering(GL_NEAREST);
	//attach counter texture:
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						 count_texture->get_OGL_id(), 0);
	GLuint clearcolor[] = { 0,0,0,0 };
	glClearBufferuiv(GL_COLOR, 0, clearcolor);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}




void voxelGICore::displayVoxels(const camera *cam, vec4 viewport) {
	//the framebuffer is already bound for us. 
	//It contains at least 1 color attachment.

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport((GLint)viewport.x, (GLint)viewport.y,
				(GLsizei)viewport.z, (GLsizei)viewport.w);

	glUseProgram(displayVoxel_shader->getShaderProgramID());
	

	//Bind 3d texture sampler, make sure to associate texunit0 with the variable name
	

	//Supply uniforms (just the view and projection matrices)
	GLuint loc = glGetUniformLocation(displayVoxel_shader->getShaderProgramID(), 
																		  "viewMat");
	glUniformMatrix4fv(loc, 1, false, (float*)&cam->getViewMatrix());
	
	loc = glGetUniformLocation(displayVoxel_shader->getShaderProgramID(), "projMat");
	glUniformMatrix4fv(loc, 1, false, (float*)&cam->getProjMatrix());
	
	//supply position of camera in the world
	loc = glGetUniformLocation(displayVoxel_shader->getShaderProgramID(), 
								"cameraPos");
	glUniform3fv(loc, 1, (float*)& cam->get_Transform()->getPos_world());

	//give texture size and texel sizes.
	loc = glGetUniformLocation(displayVoxel_shader->getShaderProgramID(),
								"textureSize");
	glUniform1f(loc, datastructure_size_world);

	loc = glGetUniformLocation(displayVoxel_shader->getShaderProgramID(), 
								"texelSize");
	glUniform1f(loc, datastructure_size_world / diffuse_tex3D->get_xRes());


	loc = glGetUniformLocation(displayVoxel_shader->getShaderProgramID(),
								"control");
	glUniform1f(loc, shader_control_value );

	loc = glGetUniformLocation(displayVoxel_shader->getShaderProgramID(),
								"control2");
	glUniform1f(loc, shader_control2_value);


	//enable reading from mipmaps :
	basecolor_tex3D->setFiltering(GL_NEAREST_MIPMAP_NEAREST);
	normal_and_brighness_tex3D->setFiltering(GL_NEAREST_MIPMAP_NEAREST);
	diffuse_tex3D->setFiltering(GL_NEAREST_MIPMAP_NEAREST);
	bounce_and_brightness_tex3D->setFiltering(GL_NEAREST_MIPMAP_NEAREST);

	//we've already associated the texture name in this shader with the texture unit
	//(during the constructor function). 
	//now, just bind the texture data to that texture unit.
	diffuse_tex3D->associate_to_TextureUnit(0);
	//TODO allow other textures to use texture unit zero.

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	
	//Draw screen-space quad.
	screen_quad->draw_mesh();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);			

	//TODO use alpha blending. Don't forget to disable it when done.

	glUseProgram(0);//unbind the visualization shader.
	//we didn't bind a framebuffer, so we have no right of unbinding it.
}




void voxelGICore::renderSecondBounce( const renderer *curr_renderer, 
									  GLuint outputFBO,
									  texture *_renderTextures[renderTextures::MAX_SIZE],
									  texture *_light_renderTextures[lightRenderTextures::MAX_SIZE]){

	texture *highResTextures[2] = { final_highResGITex,
									high_res_depthStencilTex };

	//attach OUR low res to OUR internal buffer. Resizes OUR 2 high res screen 
	//texures, if resolution of renderer's window doesn't match with their 
	//resoltutions.
	//Sets up viewport to be equal to the size of our low res GI texture.
	vec2 lowResViewport = pointLightGI::lowRes_pass_bufferSetup(this->internalFBO, 
																curr_renderer,
																this->low_res_GITex,
										 highResTextures,  2,  shader_control_value);

	//now, the viewport was already set, internalFBO was bound, with a low-res
	//destination render texture. Let's simply render into this low-res texture.
	renderScreenGI(	lowResViewport, internalFBO,   curr_renderer,
					_renderTextures,  _light_renderTextures);


	//now, setupt the interpolation stage. Connect correct textures  to the output
	//buffer (the high-res output texture  and  highRes depthStencil texture).
	//They will be used in interpolation process.
	//InternalFBO remains bound, so we can use it it for drawing, immediately.
	pointLightGI::interpol_pass_bufferSetup(this->internalFBO,
											this->final_highResGITex, 
											this->high_res_depthStencilTex);
	//now, perform actual interpolation. (viewport was already set and FBO is 
	//already bound.
	
	//upload values to the interpolation shader:
	pointLightGI::interpol_uniforms_setup(mat4::identity(), low_res_GITex,
										  curr_renderer, _renderTextures);
	
	//perform interpolation:
	screen_quad->draw_mesh();
	
	//Stencil test is still enabled from the interpolation stage above().
	//However, we need to change its comparison functions.
	glStencilFunc(GL_EQUAL, 0, ~0);
	//tell what should happen when the stencil + depth fails, when the depth fails,
	//or when both stencil + depth pass the test.
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE); //replace with 1s.
	
	vec2 highRes_viewport = vec2(curr_renderer->get_window()->getWidth(), curr_renderer->get_window()->getHeight());
	
	renderScreenGI(	highRes_viewport, internalFBO, curr_renderer,
					_renderTextures, _light_renderTextures	);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, internalFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outputFBO);
	glBlitFramebuffer(0, 0, highRes_viewport.x, highRes_viewport.y, 0, 0, highRes_viewport.x, highRes_viewport.y,  GL_COLOR_BUFFER_BIT, GL_NEAREST);


	glBindFramebuffer(GL_FRAMEBUFFER, internalFBO);
	glFramebufferTexture( GL_FRAMEBUFFER,
						  GL_STENCIL_ATTACHMENT,
						  0,
						  0 );

	glDisable(GL_STENCIL_TEST);
}




void voxelGICore::renderScreenGI( vec2 viewportSize,
								  GLuint outputFBO,
								  const renderer *curr_renderer,
								  texture *_renderTextures[renderTextures::MAX_SIZE],
								  texture *_light_renderTextures[lightRenderTextures::MAX_SIZE]){

	GLuint currProg = renderSecondBounce_shader->getShaderProgramID();
	glUseProgram(currProg);


	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
	glViewport(0, 0, viewportSize.x, viewportSize.y);

	//All of the outputs will be returned into the outputFBO, which belongs to 
	//the renderer. We will output the double bounce GI result into it.
	//It must contain 2 attachments.
	//We don't have rights to bind or unbind anything from it. We can't glClear()
	//it either. 
	

	//on the 0th sampler we will stick a 3d texture
	basecolor_tex3D->associate_to_TextureUnit(0);//non-mipmapped 
	diffuse_tex3D->associate_to_TextureUnit(1);//Mipmapped 
	normal_and_brighness_tex3D->associate_to_TextureUnit(2);//non-mipmapped
	bounce_and_brightness_tex3D->associate_to_TextureUnit(3);//Mipmapped
	 //associate the provided texture of scene's depth with texture unit 3

	 //recall,to get final, screen DIFFUSE color we will later have to do:  
	 //  BASECOLOR*ILLUM
	 //ILLUM is illumination factor that is derived after the deferred rendering 
	 //pipeline:
	_renderTextures[renderTextures::BASECOLOR_INDX]->associate_to_TextureUnit(4);
	_renderTextures[renderTextures::DEPTH_INDX]->associate_to_TextureUnit(5);
	_renderTextures[renderTextures::NORMAL_AND_EMISSION_INDX]
													->associate_to_TextureUnit(6);
	_light_renderTextures[lightRenderTextures::ILLUM_INDX]
													->associate_to_TextureUnit(7);

	//allows us to get random rotation angle, via its noise:
	repositories::getTexture("rgb_noise")->associate_to_TextureUnit(8);



	//shader sampler variable names were already associated with a numerical value
	//during the constructor.


	//construct a view-projection from renderer's matrices.
	mat4 renderer_view_mat = curr_renderer->get_curr_view_mat4();
	mat4 renderer_proj_mat = curr_renderer->get_curr_proj_mat4();
	mat4 renderer_VP_mat = renderer_proj_mat * renderer_view_mat;


	//upload the inverse of ViewProjection matrix used by current renderer.
	//With it we will be able to restore world space position of the fragments
	//visible to the viewer.
	GLint loc = glGetUniformLocation(currProg, "inverseVP");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&renderer_VP_mat.inverse());

	//give texture size (in terms of world units):
	loc = glGetUniformLocation(currProg, "textureSize");
	glUniform1f(loc, datastructure_size_world);

	//upload the size (in terms of world units) of each voxel of our 3d textures:
	loc = glGetUniformLocation(currProg, "voxel_world_size");
	glUniform1f(loc, (float)datastructure_size_world / diffuse_tex3D->get_xRes());

	//upload the number of voxels in each dimention of our 3d textures:
	loc = glGetUniformLocation(currProg, "numVoxels");
	glUniform1i(loc, (GLuint)diffuse_tex3D->get_xRes());


	loc = glGetUniformLocation(currProg, "cameraPos");
	glUniform3fv(loc, 1, (float*)&curr_renderer->getCurrentCamera()
		->get_Transform()->getPos_world());
	//TODO check that pos_world is used everywhere where it's needed.
	//some places might still be using getPos()  (which is local)
	//also check that it's computed correctly (world position)

	//upload the size of one pixel (we want the dimensions to fit into 0-1 range)
	loc = glGetUniformLocation(currProg, "pixelSize");
	vec2 pixel_size = vec2(1/viewportSize.x, 1/viewportSize.y);
	glUniform2fv(loc, 1, (float*)&pixel_size);


	loc = glGetUniformLocation(currProg, "control");
	glUniform1f(loc, 1);

	loc = glGetUniformLocation(currProg, "control2");
	glUniform1f(loc, shader_control2_value);

	loc = glGetUniformLocation(currProg, "control3");
	glUniform1f(loc, shader_control3_value);

	loc = glGetUniformLocation(currProg, "control4");
	glUniform1f(loc, shader_control4_value);

	loc = glGetUniformLocation(currProg, "control5");
	glUniform1f(loc, shader_control5_value);


	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	//DON'T DISABLE STENCIL TEST, since it will be used if inteprolation is used,
	//rendering this global illumination at lower resolutions, then upscaling it!


	//enable reading from mipmaps (quadralinear filtering):
	diffuse_tex3D->setFiltering(GL_LINEAR_MIPMAP_LINEAR);
	bounce_and_brightness_tex3D->setFiltering(GL_LINEAR_MIPMAP_LINEAR);

	//disable filtering for deferred screen textures:
	_renderTextures[renderTextures::BASECOLOR_INDX]->setFiltering(GL_NEAREST);
	_renderTextures[renderTextures::DEPTH_INDX]->setFiltering(GL_NEAREST);
	_renderTextures[renderTextures::NORMAL_AND_EMISSION_INDX]
														->setFiltering(GL_NEAREST);
	_light_renderTextures[lightRenderTextures::ILLUM_INDX]->setFiltering(GL_NEAREST);


	this->screen_quad->draw_mesh();


	//clean-up:

	//we don't have the right to unbind any attachments from the FBO that
	//the renderer supplied to us. 

	//Don't enable GL_BLEND since it's tricky. if someone wants to use it later,
	//they will have to enable it manually for themselves.
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK); //revert to default face culling
	glUseProgram(0);
}