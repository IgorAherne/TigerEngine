#version 330  

layout(location = 0) in vec3 position;
uniform mat4 viewMat; //world (NOT local) ->camera space
uniform mat4 projMat;

out vec3 eyeDir;

void main(){

	//get the direction (in the world space) to the vertex:
	eyeDir = transpose(mat3(viewMat)) * (inverse(projMat) * vec4(position, 1.0) ).xyz;
	
	//output screen quad
	gl_Position = vec4(position, 1.0); 
}