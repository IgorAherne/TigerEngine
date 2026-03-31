#version 330

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

layout(location = 0) in vec4 position;
layout(location = 2) in vec3 in_normal;

out vec3 world_normal;
out vec3 packed_eye_pos;

void main(){
	world_normal =   transpose(inverse(mat3(modelMatrix)))  *in_normal;
	vec4 eye_pos = projMatrix * viewMatrix * modelMatrix * position;
	packed_eye_pos = vec3(eye_pos.xyz*0.5 + 0.5);
	
	gl_Position = eye_pos;
}