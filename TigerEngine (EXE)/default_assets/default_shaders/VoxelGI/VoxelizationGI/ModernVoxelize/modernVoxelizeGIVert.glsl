#version 450 


uniform mat4 modelMatrix;

layout(location = 0) in vec3 position;
layout(location = 2) in vec3 normal;

out vec3 world_position;
out vec3 world_normal;


void main(){
	world_position = vec3(modelMatrix * vec4(position, 1.0));
	world_normal = transpose( inverse(mat3(modelMatrix)) ) *normal;
	gl_Position = vec4(world_position, 1); //no need to divide by w, since we didn't use projMatrix 
}