#pragma once
#include "light.h"
#include "camera.h"

class mesh;
class texture;



class pointLight : public light{

public:
	//shadowmap size of 0 will result in NO shadows computed, 
	//reducing the number of computations drastically for this light.
	pointLight(GLuint shadowmap_resolution, vec2 nearFarRange);
	~pointLight();

	void componentUpdate(float dt) override {
		//DONT use this function to update shadowmaps. 
		//It has to be done after all other gameObjects have moved & recomputed their 
		//model matrices. Typically, the end of scene::update() will 
		//calculate shadowmaps
	};

	//This function is typically called by the scene::update(), after all of the
	//other gameObjects were moved in this frame.
	void updateShadowmaps(float dt) override; 

	
	//sends the projection matrix data to the shader.
	void upload_proj_mat(GLuint shader_programID);


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


protected:
	//Checks if the recorded nearFar range doesn't correspond 
	//to the parent's one (it was changed).
	//If yes - update's light camera's projection matrix
	void updateCameraProj();

	//provide CUBE textures to be used as outputs of color or depth via
	//"output_textures".
	//They will temporarily become attachments of the lightFBO, which will use them 
	//as outputs. Textures MUST HAVE the same resolution.
	//Textures area cleared with the most recently set glClearColor, prior to render.
	//
	//Renders scene objects using the provided "shaderID" program, or if it's left 0, 
	// - with the "shadowmapShader" member variable. 
	//With glCullMode specify if back faces should be culled, front or none.
	//Supply GL_NONE and the faceculling will be disabled.
	void render_into_cube_textures(	std::vector<texture*>output_textures,
									GLuint glCullMode=GL_BACK,  
									GLuint shaderID=0  );

	//uses provided shader, binds the scene's normal, depth and light's shadowmap 
	//textures to texture units 0,1,2 respectivelly.
	//Binds light::illumination shader if "shaderToUse" is left zero.
	//Shader remains bound, so you can keep using it. 
	//So don't forget to glUseProgram(0) when done.
	//
	//Make sure your inherriting class ties "scene_normalTex", "scene_depthTex" and
	//"shadowmapTex" to those texture units at some point of time.
	//
	//The function uploads "lightMVP" (matrix describing this light's mesh relative
	//to the render's camera),
	//"inverseVP" (brings light from that camera's screen space into world space),
	//"cameraPos" vec3, "lightPos" vec3,
	//"lightRange" which is a float, "pixelSize" which is a vec2, to this shader.
	void pointLightPreDraw_UniformSetup(const renderer *curr_renderer,
								texture* _renderTextures[renderTextures::MAX_SIZE],
								GLuint shaderToUse = 0) const;


protected:
	mesh *light_mesh; //used in defferred rendering. PointLight has it as sphere.
	vec2 old_near_far_range;

private:
	//setup texture-samplers in the shader, binding each one
	//to its appropriate texture unit.
	void setup_illumShader_samplers();

	//the follwoing is only used as an FBO depth attachment in the various render 
	//processes (IT's NOT A SHADOWMAP, for latter - we use light::shadowmap_texture)
	texture *depth_texture;

	//sides can be 0-5.
	//This function clears depth buffer and color buffer bits before it renders 
	//the side of the scene into the appropriate face of the provided cubemaps. 
	//It also uploads the view matrix to shader.
	//It uses the shader program that you provide.
	void render_Cube_side( int side, vec3 pitch_yaw_roll_world,
						   std::vector<texture*> out_textures,
						   GLuint shaderID,
						   bool renderObjectsInColor = true);




	//used in rendering of the scene from the light's pov.
	//Although it's a component, we never attach it to a gameobject, but
	//carefully use the view and projection matrices of the camera.
	//
	//lightCamera->m_gameObject and lightCamera->m_transform will NOT be setup,
	//use the ones from this light class instead.
	camera *lightCamera; 
};

