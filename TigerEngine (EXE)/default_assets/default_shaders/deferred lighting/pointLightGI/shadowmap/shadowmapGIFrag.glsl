#version 330

uniform vec4 material_color; //constant color of the  surface visible from this light



in vec3 norm_world; //normals in world space
in vec3 posRelToLight; //position RELATIVE to light, packed into 0-1 range.


out vec4 fragColor[3]; //we output screen positions as seen by this light, AND world normals of
						//objects in the scene. Ironically, we don't build a depth texture, since
						//we will have positions relative to this light anyway.
						//3rd entry is the Flux value of the objects visible by light.

void main(){
	
	//position (-1 to +1 range) relative to light must be packed into 0-1, texture range.
	//0 is position of this light, 1 at the radius of lightRange, touching the far plane.
	fragColor[0] = vec4(posRelToLight*0.5 + 0.5,  1.0);
	
	fragColor[1] = vec4(   normalize(norm_world)*0.5+0.5,  1.0   ); //pack world normals into texture
	
	//the color of the surface which gets hit by the light, 
	//+ flux (rate of emmission of secondary light) as the w component.
	fragColor[2] = vec4(material_color.xyz, 1); //flux is 1 for now
	
	//no depth is needed, since we will already have screen-space position
}