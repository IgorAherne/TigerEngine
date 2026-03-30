#include "light.h"
#include "scene.h"

light::light(GLuint shadowmap_resolution, vec2 near_far_range, vec3 light_color/*=vec3(1,1,1)*/){
	this->shadowmap_size = shadowmap_resolution;
	this->light_near_far_range = near_far_range;

	this->light_color = light_color;

	glGenFramebuffers(1, &light_frameBuffer);
}


light::~light(){
	//don't delete shader, since it's used by Repositories.
	//TODO make shaders returned from repositories constant.
	glDeleteFramebuffers(1, &light_frameBuffer);
}


void light::onGameObject_AddComponent() {
	scene::getCurrentScene()->registerLight(this);
}