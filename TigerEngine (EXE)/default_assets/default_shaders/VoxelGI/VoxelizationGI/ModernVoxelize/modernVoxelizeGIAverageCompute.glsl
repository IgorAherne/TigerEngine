#version 450 core

//"raw" material color, no illumination, alpha stores transparency):
//RGBA16F is required by NVIDIA for atomic adds. (dictated in modernVoxelizeFrag.glsl)
layout(rgba16f) restrict uniform image3D basecolorTex;

//diffuse = Illumination lighting * raw material color  (a.k.a basecolor)
//alpha stores transparency of the color. 
layout(rgba16f) restrict uniform image3D diffuseTex;

//normal of the triangle (with self-glow brightness in alpha)
layout(rgba16f) restrict uniform image3D normal_and_brightness_Tex;

//counted the number of times a color was added to each voxel.
//max value is 2^32 in each texel.
uniform usampler3D countTex_inout;
uniform float control;
uniform float control2;


layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;



void main() {
	//This shader is only to be used for averaging of data.
	//Don't be consearned by alpha multiplications. It's all has been done previously.
	//Here we just divide by the count.

	ivec3 start_coord = ivec3(gl_GlobalInvocationID);


	//don't allow to divide by zero, since it will create nastly problems (proven to do so)
	float one_over_count = 1.0/max( vec4(texelFetch(countTex_inout, start_coord, 0)).r, 1.0 );


	vec4 color = vec4(imageLoad(basecolorTex, start_coord));
	imageStore( basecolorTex, 
				start_coord, 
				color*one_over_count);

	//diffuse and transparency:
	color = vec4(imageLoad(diffuseTex, start_coord));
	imageStore( diffuseTex, 
				start_coord, 
				color*one_over_count);

	//normal and emissive brightness:
	color = vec4(imageLoad(normal_and_brightness_Tex, start_coord));
	imageStore( normal_and_brightness_Tex, 
				start_coord, 
				color*one_over_count);
}