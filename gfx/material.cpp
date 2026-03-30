#include "gameObject.h"
#include "material.h"
#include "mesh.h"
#include "shader.h"
#include "repositories.h"
#include "renderer.h"


material::material(const std::string &repository_shaderName){
	shader *s =repositories::getShader(repository_shaderName);
	m_shader = s;
	m_color = color(0.5f, 0.5f, 0.5f, 1.0f); //default uniform color of mesh.
	emissive_brightness = 0.f;

	is_transparent = false;

	m_mesh = nullptr; //gets set-up once this material is bound to a gameObject.
	//shader could still be nullptr
}


material::~material(){

}


bool material::SetShader_from_repository(std::string shader_name){
	//return true if the retrieved shader wasn't a nullptr
	if (m_shader = repositories::getShader(shader_name))
		return true; 

	return false;//otherwise, it couldn't find shader with this name
}



void material::componentUpdate(float dt) {

}



void material::onGameObject_AddComponent() {
	mesh *m_mesh = m_gameObject->getComponent<mesh>();
	//However, our material's mesh-shortcut could still be nullptr.

	//establish a shortcut link for the gameObject, so it doesn't have to 
	//getComponent every time it needs its material (for rendering for example).
	m_gameObject->m_material = this;
}





bool material::draw(const renderer *currRenderer, GLuint shader_prog_to_use/*=0*/, 
					bool drawInColor/*=true*/){

	if ( !m_mesh  ||  (!m_shader && !shader_prog_to_use)  )
		return false;


			//if we were not using a previously bound shader, 
			//then we have a right to use our own.
			GLuint shader_id; 

			if (!shader_prog_to_use) {
				shader_id = m_shader->getShaderProgramID();
				glUseProgram(shader_id);
			}
			else {
				shader_id = shader_prog_to_use;
			}


	//regardless of whether we are using previously bound shader, or our own shader,
	//we will supply the modelMatrix.
	GLuint loc = glGetUniformLocation(shader_id, "modelMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&m_transform->getTransf());


	//if usePrevShader is true, then whoever called this function must have
	//setup and uploaded the viewMatrix and projMatrix
	if (!shader_prog_to_use && currRenderer != nullptr) {
		
		loc = glGetUniformLocation(shader_id, "viewMatrix");
		glUniformMatrix4fv(loc, 1, GL_FALSE,  
										(float*)&currRenderer->get_curr_view_mat4());

		loc = glGetUniformLocation(shader_id, "projMatrix");
		glUniformMatrix4fv(loc, 1, GL_FALSE,
										(float*)&currRenderer->get_curr_proj_mat4());
	}

	if (drawInColor) {
		//if the user wants this model to be rendered in color, let it supply its
		//material color, textures, etc:
		loc = glGetUniformLocation(shader_id, "material_color");
		glUniform4fv(loc, 1, (float*)&m_color.get_color_vec4());

		//supply if the object is emissive and how bright should glow on its own.
		//This means it's less susceptible to lambertian shading, shadows etc.
		loc = glGetUniformLocation(shader_id, "emissive_brightness");
		glUniform1f(loc, (float)emissive_brightness);
	}
	


	m_mesh->draw_mesh();

	//if we were not using a previously bound shader, then we have a right to 
	//unbind it.
	if(!shader_prog_to_use)
		glUseProgram(0);

	//TODO setup and use  renderer::currentShader  when want to optimize the engine.

	return true;
}







bool material::draw_custom(	GLuint shader_prog_to_use,
							mat4 *modelMatrix/* = nullptr*/, 
							mat4 viewMatrix/* = mat4::identity()*/,
							mat4 projMatrix/* = mat4::identity()*/,
							bool drawInColor/* = true*/) {

	if (!m_mesh || (!m_shader && !shader_prog_to_use))
		return false;


	//if we were not using a previously bound shader, 
	//then we have a right to use our own.
	GLuint shader_id;

	if (!shader_prog_to_use) {
		shader_id = m_shader->getShaderProgramID();
		glUseProgram(shader_id);
	} else {
		shader_id = shader_prog_to_use;
	}


	//if the model matrix was not supplied into the function, we will use the one 
	//of our transform.
	GLuint loc = glGetUniformLocation(shader_id, "modelMatrix");
	if (!modelMatrix) {
		glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&m_transform->getTransf());
	}
	else { //else, just upload the supplied model matrix:
		glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)modelMatrix);
	}


	//if usePrevShader is true, then whoever called this function must have
	//setup and uploaded the viewMatrix and projMatrix
	if (!shader_prog_to_use ) {
		loc = glGetUniformLocation(shader_id, "viewMatrix");
		glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&viewMatrix);

		loc = glGetUniformLocation(shader_id, "projMatrix");
		glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&projMatrix);
	}


	if (drawInColor) {
		//if the user wants this model to be rendered in color, let it supply its
		//material color, textures, etc:
		loc = glGetUniformLocation(shader_id, "material_color");
		glUniform4fv(loc, 1, (float*)&m_color.get_color_vec4());

		//supply if the object is emissive and how bright should glow on its own.
		//This means it's less susceptible to lambertian shading, shadows etc.
		loc = glGetUniformLocation(shader_id, "emissive_brightness");
		glUniform1f(loc, (float)emissive_brightness);
	}



	m_mesh->draw_mesh();

	//if we were not using a previously bound shader, then we have a right to 
	//unbind it.
	if (!shader_prog_to_use)
		glUseProgram(0);

	//TODO setup and use  renderer::currentShader  when want to optimize the engine.

	return true;
}