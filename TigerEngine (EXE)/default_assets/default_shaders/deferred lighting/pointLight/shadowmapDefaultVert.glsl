#version 330

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

layout(location = 0) in vec3 position;

out vec3 worldPos;

void main(){
	worldPos = vec3(modelMatrix * vec4(position.xyz, 1.0)).xyz;
	gl_Position = projMatrix * viewMatrix * vec4(worldPos, 1);
}