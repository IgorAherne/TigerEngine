#pragma once
#include <string>
#include "component.h"
#include "mesh.h"
#include "../gfxmath/color.h"
#include "../gfxmath/mat4.h"

class shader;
class gameObject;
class renderer;



			

class material : public component{
	friend class mesh;
	friend class gameObject;

public:
	//create a material, using a shader from the repository of shaders.
	material(const std::string &repository_shaderName);
	~material();

	//unchanging color, across the entire mesh's vertices.
	inline void set_Color(color col) {
		m_color = col;
	}

	//unchanging color, across the entire mesh's vertices.
	inline color getColor() {
		return m_color;
	}

	//allow the object to glow on its own. This will make it less susceptible to
	//shading like lambert and shadows. Minimum value = 0;
	//Maximum value = 2
	//At value of 1 object becomes completely unnafected by light 
	//shading like lambert, etc. At brightnesses higher than 1 shaders might
	//enhance its "glow" if the shaders allow it.
	inline void set_emissive(float brightness) {
		emissive_brightness = std::fmaxf(0, std::fminf(brightness, 2));
	}

	inline float get_emissive() const {
		return emissive_brightness;
	}


	//works with the current context, assigned in renderer::renderRenderer()
	//Returns true if the mesh was present on this gameObject and Draw() completed.
	//Returns false if the mesh was absent or if Draw() could not be completed.
	//
	//currentRenderer represents the renderer instance which ordered this material
	//to draw().  Query it for the current view, projection matrices, etc.
	//
	//shader_to_use, if a shader is passed - draw() will be using THE LATEST BOUND
	//shader (although you TELL which program to use, the function WON'T re-bind the 
	//shader, it must have been done manually by user, before calling this function)
	//and won't re-upload view or projection matrices BUT will still 
	//upload model matrix. 
	//Leave shader_prog_to_use = 0  to make material use its own personal shader.
	//
	//if drawInColor == false, the model will not supply any data about
	//material_color, textures, etc.
	bool draw(const renderer *currRenderer, GLuint shader_prog_to_use = 0,
											bool drawInColor = true);


	
	//If model matrix is not supplied, will use the one of the gameObject's.
	//
	//shader_to_use tells what shader program was BOUND THE LATEST.
	//Although you TELL which program to use, the function WON'T re-bind the 
	//shader, it must have been done manually by user, before calling this function.
	//So, if shader_to_use=someID, funciton won't re-upload view or projection 
	//matrices BUT will still upload model matrix IF it's nullptr. 
	//Additionally, whoever called this function must have already
	//setup and uploaded the viewMatrix and projMatrix to openGL to the bound shader.
	//
	//Assign  shader_prog_to_use = 0  to make material use its own personal shader.
	//You can then provide the desired view and projection matrix, as this time they
	//will be used.
	//
	//if drawInColor == false, the model will not supply any data about
	//material_color, textures, etc.
	bool draw_custom(	GLuint shader_prog_to_use,
						mat4 *modelMatrix = nullptr, 
						mat4 viewMatrix = mat4::identity(),
						mat4 projMatrix = mat4::identity(),
						bool drawInColor = true);


	//fetches an appropriate shader from the Shader Repository, and assigns it 
	//as this material's shader.
	//True is returned if appropriate shader was found and assigned. False otherwise.
	//TODO: perhaps add multiple shaders (various meshes might use different 
	//shaders)
	bool SetShader_from_repository(std::string shader_name);

	//check if this object should be drawn in a more expensive, translucent pass
	inline bool get_isTransparent() {
		return is_transparent;
	}


protected:
	void componentUpdate(float dt) override;
	void onGameObject_AddComponent() override;
	
private:
	//TODO: perhaps add multiple shaders (various meshes might use different 
	//shaders)
	shader *m_shader; //if null - draw() won't occur
	mesh *m_mesh;  //if null - draw() won't occur
	color m_color; //unchanging color, used as uniform in the shader.

	//should this object be drawn in a more expensive, translucent pass:
	bool is_transparent; 
	float emissive_brightness; //should the object "glow" on its own and how bright.
						//At '1' the object becomes unnafected by lights and shadows
						//At '2' (which is maximum) it shines the brightest on nearby
						//objects
	//TODO both terms can be discribed by vertex colors or textures.


	//parent class, "component" has  m_gameObject and  m_transform variables.
};

