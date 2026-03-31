#version 450


layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(rgba8, binding = 0) restrict writeonly uniform image3D mip_outTex; //mip that we are creating
uniform sampler3D upper_mipTex; //higher-resolution tex, from which we are creating a mip

uniform int upper_mip_lvl; //the level at which a mip map that is 1 layer higher than the output one sits.
uniform float upper_mip_numTexels; //how many texels are there along one dimention of upper mip


void main() {
	ivec3 out_mip_coords = ivec3(gl_GlobalInvocationID);
	ivec3 upper_mip_coord = out_mip_coords * ivec3(2);

	

	//coordinates of the upper mip will now become within 0-1 range:
	ivec3 upper_mip_uv_coords1 = (upper_mip_coord);
	ivec3 upper_mip_uv_coords2 = (upper_mip_coord + ivec3(1, 0, 0));
	ivec3 upper_mip_uv_coords3 = (upper_mip_coord + ivec3(1, 0, 1));
	ivec3 upper_mip_uv_coords4 = (upper_mip_coord + ivec3(0, 0, 1));
	ivec3 upper_mip_uv_coords5 = (upper_mip_coord + ivec3(0, 1, 0));
	ivec3 upper_mip_uv_coords6 = (upper_mip_coord + ivec3(1, 1, 0));
	ivec3 upper_mip_uv_coords7 = (upper_mip_coord + ivec3(1, 1, 1));
	ivec3 upper_mip_uv_coords8 = (upper_mip_coord + ivec3(0, 1, 1));


	float numItems = 8;

	vec4 color1 = texelFetch(upper_mipTex, upper_mip_uv_coords1, upper_mip_lvl);
	vec4 color2 = texelFetch(upper_mipTex, upper_mip_uv_coords2, upper_mip_lvl);
	vec4 color3 = texelFetch(upper_mipTex, upper_mip_uv_coords3, upper_mip_lvl);
	vec4 color4 = texelFetch(upper_mipTex, upper_mip_uv_coords4, upper_mip_lvl);
	vec4 color5 = texelFetch(upper_mipTex, upper_mip_uv_coords5, upper_mip_lvl);
	vec4 color6 = texelFetch(upper_mipTex, upper_mip_uv_coords6, upper_mip_lvl);
	vec4 color7 = texelFetch(upper_mipTex, upper_mip_uv_coords7, upper_mip_lvl);
	vec4 color8 = texelFetch(upper_mipTex, upper_mip_uv_coords8, upper_mip_lvl);
	//We assume that correct (affected by their alpha) colors were plugged in from the very start.
	//Colors will lose their value as they will blend in with the black, empty voxels.
	//Afterwards, alpha will only be used for opacity check, during Voxel Cone Tracing.



	vec4 finColor = color1 + color2 + color3 + color4 + color5 + color6 + color7 + color8;

	imageStore(mip_outTex, out_mip_coords, finColor/8);
}