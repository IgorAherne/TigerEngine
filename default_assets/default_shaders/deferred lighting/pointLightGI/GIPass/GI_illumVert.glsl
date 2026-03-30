#version 330

uniform mat4 MVP;


layout(location = 0) in vec4 position;


void main(){
	//Draw the light object. Anything that it covers will be attenuated and lit
	//by Global Illumination
	gl_Position = MVP * position;
}