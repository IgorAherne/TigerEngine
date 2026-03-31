#version 430


layout(local_size_x = 1, local_size_y = 1,local_size_z = 1) in;

layout(rgba8, binding= 0) uniform image2D mip_outTex; //mip that we are creating
uniform sampler2D upper_mipTex; //higher-resolution tex, from which we are creating a mip

uniform int upper_mip_lvl; //the level at which a mip map that is 1 layer higher than the output one sits.
uniform float upper_mip_numTexels; //how many texels are there along one dimention of upper mip



void main() {
			ivec2 out_mip_coords = ivec2(gl_GlobalInvocationID.xy);

			//coordinates of the upper mip will now become within 0-1 range:
			vec2 upper_mip_coords = vec2(vec2(out_mip_coords*2) + 1.0) / (upper_mip_numTexels);

			vec4 color =      texture(upper_mipTex, upper_mip_coords, upper_mip_lvl  );

			imageStore(mip_outTex, out_mip_coords, color);
}