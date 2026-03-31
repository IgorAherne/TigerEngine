#version 330

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

uniform vec3 lightPos;
uniform float lightRange; //corresponds to far plane

layout(location = 0) in vec3 position;
layout(location = 2) in vec3 normal;


out vec3 norm_world;
out vec3 posRelToLight;


void main(){
	//output world space normals:  (we will pack them in fragment shader)
	norm_world =  transpose(inverse(mat3(modelMatrix))) * normal; 
	

		//re-specify the vertex position to be relative to light:
		posRelToLight = vec3(modelMatrix * vec4(position, 1)) - lightPos;
		posRelToLight = posRelToLight/lightRange;
		
		//pack this position into -1 to +1, ndc range:
		//don't allow greater than 1
		//don't allow less than 1.
		posRelToLight = clamp(posRelToLight, -1, 1); 
		 
		
		//we will pack this position in the pixel shader, just like we do with the normal.
	
	
	
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(position,1.0);
}