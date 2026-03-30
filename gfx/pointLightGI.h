#pragma once
#include "pointLight.h"
#include "../gfxmath/vec2.h"

class mesh;


//this light extends the basic point light, by using a single-bounce global 
//illumination. It is a lot more expensive than the point lights. 
//Updating the light is ~3 times more expensive than the point light, but it's even
//more expenisve to draw(), applying the gathered illumination to the visible scene.
//Looks a lot more realistic than a simple point light.

class pointLightGI : public pointLight{

public:
	pointLightGI(GLuint shadowmap_resolution, GLuint GI_resolution,
				 vec2 nearFarRange);

	~pointLightGI();

	void componentUpdate(float dt) override {
		//DONT use this function to update shadowmaps. 
		//It has to be done after all other gameObjects have moved & recomputed their 
		//model matrices. Typically, the end of scene::update() will 
		//calculate shadowmaps
	};

	//Updates light  position texture, normal texture and flux texture.
	//It then envokes updateShadowmaps() of the parent class, so
	//that parent class can update its own shadowmap.
	//This function is typically called by the scene::update(), after all of the
	//other gameObjects were moved in this frame.
	void updateShadowmaps(float dt) override;



	//create illumination textures, based on provided textures 
	//of scene viewed by camera.
	//If any required texture is abscent from the _renderTextures, (is nullptr), 
	//light will still attempt to use it, IF shader requires this type of texture.
	//
	//For example, a usual point light will expect a normal (world-space) with
	//emission factor (self-glow) in alpha,
	//specularity and depth textures to calculate the attenuation and apply shadows.
	//provide it an FBO where to output the results, with 2 textures that were
	//already attached PRIOR to this function call.  
	//Those 2 textures must be:   where to ouput resulting illumination color
	//and illumination specularity. 
	//Just as other textures, those 2 will be queried from the array via 
	//renderer::renderTextures  and  renderer::lightRenderTextures enums.  
	//
	//The FrameBuffer must be cleared color-wise BEFORE this function 
	//gets called. Light's won't clear anything, they will just output into it.
	//This framebuffer must NOT have a stencil or depth attachments, because some 
	//lights might attach their own stencil.
	//Depth texture from _renderTextures will however be used.
	//
	//TODO if enum names changed, update them in this comment too.
	//
	//In addition to that, it requires the current renderer which issued this draw.
	void draw( const renderer *curr_renderer, GLuint outputFBO,
				texture *_renderTextures[renderTextures::MAX_SIZE] ) override;


	//Static function, since used also by other classes, when those have to render
	//at lower resolution, then upscale it onto the screen (for performance).
	//Supply it an ID of your output FBO to which the LOW_RES_OUTPUTTEX will be 
	//attached.
	//All other textures are full-screen sized ones, will be re-sized if they don't
	//match the size of the renderer window's dimentions.
	//
	//For screen size crunching, approximation 1 = no approximation    
	//2 and above = smaller size of screen.
	//Don't get greedy if the scene has high depth-variation.
	//Funciton will set glViewport to the  "(dimentions of window) / approximation"
	//
	//Outputs the dimentions of the viewport used in its rendering 
	//which will be  "(dimentions of window) / approximation"
	static vec2 lowRes_pass_bufferSetup(  GLuint outputFBO, 
										   const renderer *curr_renderer,
										   texture *low_res_outputTex,
										   texture **array_of_highResTextures,
										   int array_count,
										   GLuint approximation=8 );

	//simmilar to lowRes_pass_bufferSetup() function and typically goes hand-in-hand
	//with it. Supply two textures which will be attached into your outputFBO.
	//They should be full-scale, with the desired resolution.
	//
	//will also run glViewport() function, preparing for draw that you will issue 
	//in a moment.
	static void interpol_pass_bufferSetup(  GLuint outputFBO, 
											texture *highRes_outputTex,
											texture *highRes_depthStencilTex  );


	//Static function, since used also by other classes, when those have to render
	//at lower resolution, then upscale it onto the screen (for performance).
	// Typically goes hand-in-hand with  static interpol_pass_bufferSetup()
	//
	//uploads all the information to the interpolation shader.
	//It keeps the interpolation shader bound, so it can be worked with 
	//straight after this funciton was called.
	//Therefore, don't forget to unbind it when done working with it.
	//
	//MVP is model-view-projection matrix. It allows us to supply its value to the 
	//interpolation shader.
	//For instance, during deferred rendering, MVP must be the one of light's
	//sphere. During post processing effects, MVP should be identity, if the screen
	//quad will be used with this shader, etc.
	//
	//low_res_texture allows us to peek into the size of the texture that is
	//to be upscaled.
	static void interpol_uniforms_setup( mat4 MVP, texture *low_res_texture,
									const renderer *curr_renderer,
								  texture* _renderTextures[renderTextures::MAX_SIZE] );



private:
	//global illumination can be very expensive to calculate for each pixel, during 
	//draw(). Instead, we render a low-resolution GI illumination, and then "upscale"
	//it using interpolation pass.
	void draw_low_rez_GIpass( const renderer *curr_renderer,
							  texture* _renderTextures[renderTextures::MAX_SIZE]);

	//"upscales" the smaller GI texture into a full-size final texture.
	//Any fragments that don't have enough information for such interpolation will
	//be placed aside, and GI will be recalculated manually for them in a later stage
	void draw_interpol_GIpass(const renderer *curr_renderer,
							  texture* _renderTextures[renderTextures::MAX_SIZE]);



	//recalculates GI for the fragments that couldn't get the interpolated color 
	//during draw_interpol_GIpass().  Perhaps those fragments were in the corner or
	//the depth of the neighbor fragments varied alot.
	void draw_delayed_fullGI(const renderer *curr_renderer,
							 texture* _renderTextures[renderTextures::MAX_SIZE]);



	//envoked after the delayed_fullGI pass,
	//applies the avalable full-res lambertian shading and shadowmap texture
	//to the visible part of the scene
	void apply_high_quality_shading(const renderer *curr_renderer, GLuint outputFBO);



	//drawas the scene as seen from light, prepares shadowmap:
	shader *shadowmapGI_shader;

	//calculates Global Illumination for each visible fragment of the output texture:
	shader *illuminationGI_shader;


	shader *hq_shading_shader;



	GLuint internalFBO;//used for all render operations internal to this light.
	texture* low_res_GITex;//low-res GI texture2D contribution, as seen by the viewer

	//resulting GI, after the interpolation stages were done. It is the GI 
	//contribution, visible to the viewer.
	texture* final_highResGITex;

	//in high pass, if a fragment can't recieve a GI value interpolated from the 
	//low-res texture, it will be marked "0" in this stencil texture. A further pass 
	//will compute GI for all such maked fragments.
	texture* high_res_depthStencilTex;

	//positions of scene items, RELATIVE to the light (in world space):
	//To unpack them, bring them into NDC from texture space, then multiply 
	//the resulting vectors by lightRange. The outcome will be relative to lightPos.
	texture* light_PosTex;

	//normals of the scene, as seen by the light (in world space):
	texture* light_NormTex;

	//holds the rate of light emission of scene's surfaces in the 4th component,
	//and the color of surface which the light hits  - in xyz components.
	texture* light_FluxTex;

	//allows us to have depth test when rendering from light's POV.
	//We will still use pointLight::shadowmap_texture for shadows, but this one will
	//be used for intermediate depth tests.
	texture* light_GIdepth_Tex;

	//output texture, containing the full-res lambertian shading and shadowmaped
	//portion of the scene.
	texture* hq_shading_tex;

	//used in the internal rendering of this light's GI contribution.
	mesh *screenQuad;
};

