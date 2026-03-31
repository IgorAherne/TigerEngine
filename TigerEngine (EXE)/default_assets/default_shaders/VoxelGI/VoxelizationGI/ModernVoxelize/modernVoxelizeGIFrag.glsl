#version 450 
#extension GL_NV_shader_atomic_fp16_vector : enable
#extension GL_NV_gpu_shader5 : enable



uniform vec4 material_color; //base color of the object's material.

 //should the object "glow" on its own and how bright.
//At 1 it becomes unresponsive to lambert and shadows of lights,
// at 2 (maximum value) it shines brightest on surrounding objects.
uniform float emissive_brightness;
 
uniform vec4[6] light_pos_andRange;  //light positions in world space (maximum 6 is supported).
						//last component (alpha) stores range of the light.
						//as soon as attenuation radius of "-1" is detetected, or the last
						// index is reached, we know that there are no more lights supplied.

uniform samplerCube[6] light_cube_distTex; //as soon as -1 is fetched or the end of the
						//array is reached, we know that there are no more lights supplied

uniform vec3[6] light_colors; //NOTICE, it's vec3, not vec4

uniform float textureSize; //how large each texture is in each dimension
uniform float texelSize; //how large is each "3d pixel" in the texture

in vec3 world_norm; 
in vec3 world_pos;
 
out vec4 fragColor;   

uniform float control;

//Since we are adding color, it will grow. We are using 16-bit floats vec4, since it's required by NVIDIA. 
//We will need a separate pass which will divide collected values in voxels by the count (total number
//of times this voxel was written to). It will average out the color.


//"raw" material color, no illumination, alpha stores transparency):
layout(rgba16f)  restrict  uniform image3D basecolorTex_output;

//diffuse = Illumination lighting * raw material color  (a.k.a basecolor)
//alpha stores transparency of the color. 
layout(rgba16f)  restrict  uniform image3D diffuseTex_output;

//normal of the triangle (with self-glow brightness in alpha)
layout(rgba16f)  restrict  uniform image3D normal_and_brightness_Tex_output;

layout(r32ui)  restrict  uniform uimage3D countTex_inout;



//declare signatures of further functions:
float Tell_if_in_shadow(vec3 fragPos, int light_indx);



void main(){

	//see how many small texels fit into  texture measured in world units:
	int numTexels = int(textureSize / texelSize);
	
	vec3 texture3Dcoords = clamp(world_pos, vec3(-textureSize*0.5), vec3(textureSize*0.5));
	texture3Dcoords /= textureSize; //see how much of texture size (this size is in world units) we have covered
	texture3Dcoords = texture3Dcoords + 0.5; //offset from -0.5 to 0.5 range into  0 to 1
	texture3Dcoords *= numTexels; //use this percentage to calculate the texel.

	//increment the counter for this voxel. It depicts how many similar "fragments" requested
	//to work with it. Later we will use this count to divide gather values, averaging them out.
	imageAtomicAdd(countTex_inout, ivec3(texture3Dcoords), uint(1));

	//ADD original, "raw" material color + transparency to the voxel.
	//Since we are adding color, it will grow. 
	//Later we will need a separate pass which will divide collected values in voxels by the count 
	//(total number of times this voxel was written to). It will average out the color.
	imageAtomicAdd(basecolorTex_output, ivec3(texture3Dcoords), f16vec4(material_color.rgb*material_color.a, material_color.a));
	//if alpha is anything lower than one - we applied effect now.
	//Later, during mipmapping process we will not affect generated colors by alpha, - 
	//We will plug correct (affected by their alpha) from the very start.
	//Colors will lose their value as they will blend in with the black, empty voxels.
	//Afterwards, alpha will only be used for opacity check, during Voxel Cone Tracing.


	//we won't pack normal, since it's stored in signed 16-bit float, by NVIDIA.
	//We shouldn't pack it anyway, to avoid messing up averaging.
	imageAtomicAdd( normal_and_brightness_Tex_output, ivec3(texture3Dcoords),
					f16vec4(  normalize(world_norm)*0.5+0.5, clamp( emissive_brightness-1, 0, 1)  )
				  );
	
	
	//shading and light colors comming from all light sources:
	vec3 illumination = vec3(0);

	//iterate through all the supplied lights
	for (int light_indx = 0; light_indx < 6; light_indx++) {
		//see if the light attenuation radius indicates end of the lightss
		if (light_pos_andRange[light_indx].a < 0) {
			break;
		}
		else{//sometimes shaders don't want to break. It's best to put 'else' here so that they won't execute what's after.
			float dist = length(world_pos - light_pos_andRange[light_indx].xyz);
			float atten = (light_pos_andRange[light_indx].a - dist) / light_pos_andRange[light_indx].a; //linear falloff
			float sqr_atten = pow(atten, 2); //square falloff

			float shadow = Tell_if_in_shadow(world_pos, light_indx);

			//supply light colors modified by shadow factor and distance attenuation:
			//TODO increment the current value instead of re-writing it:
			float lambert = max(0,   dot( normalize(world_norm),  
										  normalize(light_pos_andRange[light_indx].xyz - world_pos) )  );

			//add up light contribution to the total illumination from all light sources:
			illumination += lambert * sqr_atten * shadow * light_colors[light_indx].rgb;
		}
	}//end light-forloop

	
	 //as the emissive_brightness approaches '1', object will become less and less afected by lights.
	vec3 diffuse =  material_color.rgb*illumination * (1 - clamp(emissive_brightness, 0, 1))//shaded by light if brightness is 0-to-1
				    + material_color.rgb * clamp(emissive_brightness, 0, 1);//or will be material color if brightness is '1' or above.

	//if alpha is anything lower than one - apply effect now.
	//later, during mipmapping process we will not affect generated colors by alpha, - 
	//We will plug correct (affected by their alpha) from the very start.
	//Colors will lose their value as they will blend in with the black, empty voxels.
	//Afterwards, alpha will only be used for opacity check, during Voxel Cone Tracing.
	 diffuse *= material_color.a;

	 //material color's transparency of "greater than zero" is very important, as it will be "blurred out" during 
	 //mip mapping process into the surroudning empty black voxels (whose alpha is 0). 
	 //This is then useful to estimate the accomulating opacity during voxel cone tracing.
	 //Notice, even if voxel ends up "black", it's alpha could still be greater than zero.
	imageAtomicAdd( diffuseTex_output,
					ivec3(texture3Dcoords),
					f16vec4(diffuse, material_color.a)
				   );

	//we already stored all voxel values in our textures. Just so that our FBO doesn't flip out,
	//let's supply a dark red color into its texture
	fragColor = vec4(0.4, 0, 0, 1);
}




//returns 1-0 value if the fragment is in   not/is in   shadow.  1st arg is cubemap, 2nd arg is world pos relative to light
float Tell_if_in_shadow(vec3 fragPos, int light_indx) {

	vec3 light_to_frag_wpos = fragPos - light_pos_andRange[light_indx].xyz;

	float sampledDistance = texture(light_cube_distTex[light_indx], light_to_frag_wpos).r;

	//increase the distance from shadowmap, since we've offseted the voxels of this 
	//triangle during the previous stage (geometry shader).
	//They are further than what shadowmap thinks.
	sampledDistance += length(vec3(texelSize))*0.3;

	/*//simple distance to a fragment from the light's point of view is not enough.
	//we need to determine which voxel does such fragment correspond to:
	vec3 sampledLit_voxel = sampledDistance * normalize(light_to_frag_wpos);
	sampledLit_voxel = floor(sampledLit_voxel / texelSize)* texelSize + 0.5*texelSize;
	//now we will calculate distance to that voxel's centre, from light:
	float sampledDist_towards_litVoxel = length(sampledLit_voxel - light_pos_andRange[light_indx].xyz);
	*/

	//this is how far our original (CURRENT) voxel is located away from the light:
	float distance = length(light_to_frag_wpos);

	
	//we use negative peter-panning "-texelSize*0.2" offset since if the CURRENT voxel == lit voxel,
	//it must not be in shadow:
	return step(distance + texelSize, sampledDistance);
	//1 if voxel is not in shadow, 0 otherwise.
	//1 = (distance < sampledDistance)
}
