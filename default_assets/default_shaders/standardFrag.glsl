#version 330


uniform vec4 material_color;
uniform float emissive_brightness;

in vec3 world_normal;
in vec3 packed_eye_pos;

out vec4 fragColor[4]; //0position 1difuse, 2normal, 3specular

void main(){
	vec3 packed_norm = normalize(world_normal)*0.5+0.5;  //normalize and pack it up.

	
	
	fragColor[0] = vec4(packed_eye_pos,1); //position
	fragColor[1] = material_color; 			//color
	
	//normal + emission factor (self-glow)in alpha. Will be clamped in 0-to-1 range
	fragColor[2] = vec4(packed_norm, emissive_brightness);  
}