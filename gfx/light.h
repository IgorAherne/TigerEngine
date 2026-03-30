#pragma once
#include "component.h"
#include "shader.h"
#include "../gfxmath/vec2.h"
#include "renderer.h" //need it for enums declared in renderer.
#include "../gfxmath/color.h"

class texture;



class light : public component{
public:

	//size of  0 will result in no shadows computed for this light at all,
	//reducing the number of computations drastically.
	light(GLuint shadowmap_resolution, vec2 near_far_range, vec3 light_color=vec3(1,1,1));

	~light();

	inline GLuint getShadowmap_size() const {
		return shadowmap_size;
	}

	inline GLuint getLight_frameBuffer_id() const {
		return light_frameBuffer;
	}

	inline const shader *get_shadowmap_ShaderProgram_id() const {
		return shadowmap_shader;
	}

	inline vec2 get_light_nearFar_range() const{
		return this->light_near_far_range;
	}

	inline vec2 set_light_nearFar_range(vec2 nearFar) {
		this->light_near_far_range = nearFar;
	}

	inline void set_intensity(float new_intensity) {
		this->intensity = new_intensity;
	}

	//add an additional value to intencity. 
	inline void add_intensity(float extra_intensity) {
		this->intensity += extra_intensity;
	}
	//determine how bright does the light illuminate its given range.
	inline float get_intensity() const {
		return intensity;
	}

	inline vec3 get_color() const {
		return light_color;
	}

	inline void set_color(vec3 col) {
		light_color = col;
	}
	
	inline void set_color(vec4 col) {
		light_color = vec3(col.x, col.y, col.z);
	}

	inline void set_color(color col) {
		light_color = vec3(col.r, col.g, col.b);
	}

	inline texture *get_shadowmapTexture() const {
		return shadowmap_texture;
	}

	//DONT use this function to update shadowmaps. 
	//It has to be done after all other gameObjects have moved & recomputed their 
	//model matrices. Typically, the end of scene::update() will calculate shadowmaps
	void componentUpdate(float dt) override = 0;

	//tell the extending class to draw the scene into the shadowmaps of this light.
	//It does it using  this->shadowmap_shader
	virtual void updateShadowmaps(float dt) = 0;

	void onGameObject_AddComponent() override  sealed;


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
	virtual void draw(const renderer *curr_renderer, GLuint outputFBO,
					   texture *_renderTextures[renderTextures::MAX_SIZE] ) = 0;


protected:
	texture* shadowmap_texture = nullptr; //shadowmap depth texture.

	 //shader used to generate our shadowmap for this light
	const shader* shadowmap_shader = nullptr; 

	//light's up the visible screen-texture with illumination colors.
	const shader* illumination_shader = nullptr;


private:
	//size of  0 will result in no shadows computed at all,
	//reducing the number of computations drastically.
	GLuint shadowmap_size = 0;

	//can be used to attach shadowmaps (for instance if they are GL_TEXTURE_CUBE_MAP)
	//or any color attachments.  This will render the scene as the light sees it, 
	//into this FBO's depth. 
	//when working with framebuffer, make sure to setup correct color 
	//masks are set, (to avoid corrupting color textures when making depth, etc),
	GLuint light_frameBuffer = 0;

	float intensity = 0;  //how bright does the light illuminate its given range

	//where does light start to shine and how far it extends
	vec2 light_near_far_range; 

	vec3 light_color;
};

