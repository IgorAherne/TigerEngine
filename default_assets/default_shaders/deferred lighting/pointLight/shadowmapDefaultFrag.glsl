#version 330

uniform vec3 lightPos;

in vec3 worldPos;
out vec4 fragColor;

void main(){
    //our texture is made of GL_R32F, meaning it has 1 non-normalized component.
	//we will store distance from light to this fragment into it:
	fragColor.r = length(lightPos - worldPos);
}