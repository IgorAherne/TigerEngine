#version 450
#pragma optionNV (unroll all) //make sure all loops are unrolled (must be based on constants though)

 

uniform sampler3D basecolorTex; //used to modify the incoming indirect illumination light.
uniform sampler3D diffuseTex;//light illumination * basecolor     Alpha stores transparency. 
uniform sampler3D normal_and_brightnessTex; //normals + emissive_brightness in alpha

uniform sampler2D rgb_noiseTex;//allows us to get random rotation angle, via its noise.

uniform float control;
uniform float control2;
uniform  int numTexels;//how many texels there is along 1 dimention of a texture:

layout(rgba16f) writeonly uniform image3D lightBounceTex_output;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;




//axis must be normalized
mat3 matrixFromAxisAngle(vec3 axisFrom, vec3 axisTo) {

	float radian_angle = acos(dot(axisFrom, axisTo));

	vec3 rawPivot = cross(axisTo, axisFrom);
	float pivotLen = length(rawPivot);
	vec3 pivot = pivotLen > 0.001
	             ? rawPivot / pivotLen
	             : normalize(cross(axisTo, vec3(0.0, 0.0, 1.0)));

	float c = cos(radian_angle);
	float s = sin(radian_angle);
	float t = 1.0 - c;

	mat3 rotMat = mat3(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));

	rotMat[0].x = c + pivot.x*pivot.x*t;
	rotMat[1].y = c + pivot.y*pivot.y*t;
	rotMat[2].z = c + pivot.z*pivot.z*t;


	float tmp1 = pivot.x*pivot.y*t;
	float tmp2 = pivot.z*s;

	rotMat[1].x = tmp1 + tmp2;
	rotMat[0].y = tmp1 - tmp2;

	tmp1 = pivot.x*pivot.z*t;
	tmp2 = pivot.y*s;

	rotMat[2].x = tmp1 - tmp2;
	rotMat[0].z = tmp1 + tmp2;    tmp1 = pivot.y*pivot.z*t;

	tmp2 = pivot.x*s;

	rotMat[2].y = tmp1 + tmp2;
	rotMat[1].z = tmp1 - tmp2;

	return rotMat;
}


//returns 1 if inside the cube volume (defined by vec3(0) as minimum coord and vec3(1) as maximum)
//returns 0 if outside the volume.
float isWithin01Bounds(vec3 coord) {
	vec3 product = floor(0.9999 + max(vec3(0) + coord, -0.1) * max(vec3(1) - coord, -0.1));
	return product.x*product.y*product.z;
}



void main() {
	ivec3 start_coord = ivec3(gl_GlobalInvocationID);

	//get the direction of the current, main voxel:
	float voxelTranspar = texelFetch(diffuseTex, start_coord, 0).a;
	//TODO check if normal is empty - hence we could return straight away...
	if (voxelTranspar == 0) {
		return;
	}
	//on some drivers "return" won't work, hence we need "else"  
	else {
		//get the direction of the current, main voxel:
		vec4 voxelNormal = texelFetch(normal_and_brightnessTex, start_coord, 0).rgba;

		//unpack the normal, make sure NOT to modify Alpha!
		voxelNormal = vec4(normalize(voxelNormal.xyz * 2 - 1), voxelNormal.a); 


		//we will still use start_coord for final output into texture, but will 
		//pretend that we started from the "offset_start_coord" 
		//(this offsets the cones along the normal & avoids voxel self-illumination artifact):
		vec3 offset_start_coord = vec3(start_coord) + vec3(voxelNormal.xyz * 2.1 );

		//0-to-1 ranged coordinate for sampling from textures:
		vec3 sampling_coord = vec3(start_coord) / numTexels;
		//those are the normalized directions for NON-ROTATED CONE bouquet, looking upwards.
		vec3 noiseColor = texture(rgb_noiseTex, vec2(sampling_coord.x * 400 + sampling_coord.y * 400,
											         sampling_coord.y * 400 + sampling_coord.z * 400)).rgb;

		float rand_angle = (noiseColor.r + noiseColor.g + noiseColor.b)*0.33333*3.14159;

		float c = cos(rand_angle);
		float s = sin(rand_angle);

		mat3 randSpin_mat = mat3(vec3(c, 0, s), vec3(0, 1, 0), vec3(-s, 0, c));

		vec3 allConeDirs[7] = vec3[7](	vec3(0.00001, 1, 0),
										vec3(0.766, 0.6428, 0),
										vec3(0.383, 0.6428, 0.6634),
										vec3(-0.383, 0.6428, 0.6634),
										vec3(-0.766, 0.6428, 0),
										vec3(-0.383, 0.6428, -0.6428),
										vec3(0.383, 0.6428, -0.6428)
									 );

		//build rotaion matrix, creating a rotation which will
		//aling our upwards-looking cone with the current normal.
		//THIS FUNCTION IS FASTER BY 28% than the following approach:
		//1) binorm = cross(normal, vec(1,0,0));
		//2) binorm = normalize(binorm);
		//3) cross(binorm, normal) to get the true, final tangent,
		//4) constructing mat3(tangent, normal, binormal)
		//notice how we first multiply by our randSpin matrix:
		mat3 orient_mat = matrixFromAxisAngle(allConeDirs[0], voxelNormal.xyz) *randSpin_mat;
		//re-orient "allConeDirs" with it.

		allConeDirs[0] = orient_mat * allConeDirs[0];
		allConeDirs[1] = orient_mat * allConeDirs[1];
		allConeDirs[2] = orient_mat * allConeDirs[2];
		allConeDirs[3] = orient_mat * allConeDirs[3];
		allConeDirs[4] = orient_mat * allConeDirs[4];
		allConeDirs[5] = orient_mat * allConeDirs[5];
		allConeDirs[6] = orient_mat * allConeDirs[6];

		float cone_halfAngle = 0.37;		 //22 deg (full cone will be 44 deg)
											
											//this is where the incomming light will be stored for this voxel:
		vec4 finalColor = vec4(0, 0, 0, 0);


		//this is how many cones we will use:
		const int cone_num = 7;

		//cosine-weighted solid-angle weights: center cone ~0.206, side cones ~0.132
		float coneWeights[7] = float[7](0.206, 0.132, 0.132, 0.132, 0.132, 0.132, 0.132);

		//perform voxel cone tracing with 7 cones:
		for (int cone = 0; cone < cone_num; cone++) {

			vec3 coneDir = allConeDirs[cone];

			//sampling_maxNum is determined by the number mipmaps there will be in the bare minumum (log2),
			//and the cone size (how quickly we will be going towards next mipmap.
			//however, if we hardcode number by hand, shader will be able to unfold its for-loop.
			//So perhaps if we need to change this number we will need to modify source through C++ then re-compile
			const int sampling_maxNum = 8; //int(log2(numTexels) + 1s);


			//0 voxel is what we start with as the "step along the cone dir".
			//As we will get further and the cone will expand, we will 
			//be stepping by larger distances, hence this variable will grow.

			//Start at 1 voxel so each cone's first sample is at a distinct position.
			//At distance 0 all cones would sample the same point (the offset start),
			//wasting 6 of 7 texture fetches with no directional differentiation.
			float travelled_voxelDistance = 1;
			vec4 color_sampledByCone = vec4(0, 0, 0, 0);

			//move along the cone direction:
			for (float i = 0; i < sampling_maxNum; i++) {

				//we will be sampling textures in a moment, it is based on starting coord 
				//+ advancement along the cone direction.
				//All of that is brought back into  "0 to 1" range with division by  total number of texels
				//along the dimention of a texture: 
				sampling_coord = (vec3(offset_start_coord + 0.5) + coneDir*travelled_voxelDistance) / numTexels;


				//determine the value of mip from the current width of the cone at this 
				//sampling coord:
				//tan(theta) = opposite / adjacent  ==  half of mip texel size / distance traveled
				float expected_voxel_voxelSize = tan(cone_halfAngle) * travelled_voxelDistance * 2;

				//voxels have size 1x1x1 in mip lvl 0, 
				//size 2x2x2 in mip lvl 1,
				// 4x4x4 in mip lvl 2
				// 8x8x8 in mip lvl 3 etc
				float sampling_mip_lvl = log2(expected_voxel_voxelSize);
				//keep it as float, since we want to have interpolation between Mipmaps as well.
				//expected_voxel_voxelSize might be like 0.2101 etc, if the cone is that wide.
				//So it will take time to get to 1.000  (1st mip level) if cone is narrow.

				vec4 foundDiffuse = textureLod(diffuseTex, sampling_coord, sampling_mip_lvl);

				//if sampling coord fell out from 0-to-1 range, we should discard any 
				//further encounted voxels:
				//this function is faster than the 'if' statement or the 2 step() functions.
				//We avoid using step() in the foorloops, since it provides great speed boosts.
				float within_bounds = isWithin01Bounds(sampling_coord);

				/*	if (within_bounds.x or y or z == 0) {
				break;
				}*/

				
				//accomulate diffuse encountered at this advance.
				//Concatenate colors as long as the incomming alpha can fit into the color_sampledByCone's:  
				//And as long as the new colors are not out of datastructure's bounds
				//for some wierd reason floor() improves performance:
				color_sampledByCone.rgb +=	(1 - color_sampledByCone.a) 
											*vec3(foundDiffuse.rgb) //TODO we can store emissiveness brightness here (in alpha), since transparency is already in diffuseTex
											*within_bounds;  //instead of 'if' (commented out above)

				//CRITICAL: Alpha accumulated AFTER color:
				color_sampledByCone.a += (1.0 - color_sampledByCone.a) * foundDiffuse.a;

				//START CONE DEBUGGING
			/*		vec4 mcol = vec4(0);
					if (int(sampling_mip_lvl) >= 0 )
						mcol = vec4(1, 1, 0, 1); 

					if (int(sampling_mip_lvl) >= 1 )
						mcol = vec4(0, 1, 1, 1);

					if (int(sampling_mip_lvl) >= 2 )
						mcol = vec4(1, 0, 1, 1);
					if (int(sampling_mip_lvl) >= 3)
						mcol = vec4(0.8, 0.5, 0, 1);

					if (int(sampling_mip_lvl) >= 4)
						mcol = vec4(0.32, 0.9, 1, 1);

					if (int(sampling_mip_lvl) >= 5)
						mcol = vec4(0.9, 0.3, 1, 1);

					if (int(sampling_mip_lvl) >= 6)
						mcol = vec4(0.9, 0.3, 0.71, 1);

					mcol.rg *=  mod(i, 2) == 0 ? 1 : 1.2;


				if (int(sampling_mip_lvl) >= 0) {
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance), mcol);
				}
				if (int(sampling_mip_lvl) >= 1) {
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance)+ivec3(1,0,0),  mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance)+ivec3(0,1,0),  mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance)+ivec3(0,0,1),  mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance)+ivec3(1,1,0),  mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance)+ivec3(0,1,1),  mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance)+ivec3(1,1,1),  mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance)+ivec3(1,0,1),  mcol);
				}
				if (int(sampling_mip_lvl) >= 2) {
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(3, 0, 0)-ivec3(2,2,2), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 3, 0)-ivec3(2,2,2), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 0, 3)-ivec3(2,2,2), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(3, 3, 0)-ivec3(2,2,2), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 3, 3)-ivec3(2,2,2), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(3, 3, 3)-ivec3(2,2,2), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(3, 0, 3)-ivec3(2,2,2), mcol);
				}
				if (int(sampling_mip_lvl) >= 3) {
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(7, 0, 0)-ivec3(4,4,4), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 7, 0)-ivec3(4,4,4), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 0, 7)-ivec3(4,4,4), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(7, 7, 0)-ivec3(4,4,4), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 7, 7)-ivec3(4,4,4), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(7, 7, 7)-ivec3(4,4,4), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(7, 0, 7)-ivec3(4,4,4), mcol);
				}
				if (int(sampling_mip_lvl) >= 4) {
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(15, 0, 0)  -ivec3(8,8,8), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 15, 0)  -ivec3(8,8,8), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 0, 15)  -ivec3(8,8,8), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(15, 15, 0) -ivec3(8,8,8), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 15, 15) -ivec3(8,8,8), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(15, 15, 15)-ivec3(8,8,8), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(15, 0, 15) -ivec3(8,8,8), mcol);
				}

				if (int(sampling_mip_lvl) >= 5) {
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(31, 0, 0)  -ivec3(16,16,16), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 31, 0)  -ivec3(16,16,16), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 0, 31)  -ivec3(16,16,16), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(31, 31, 0) -ivec3(16,16,16), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 31, 31) -ivec3(16,16,16), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(31, 31, 31)-ivec3(16,16,16), mcol);
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(31, 0, 31) -ivec3(16,16,16), mcol);
				}

				if (int(sampling_mip_lvl) >= 6) {
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(63, 0, 0) -  ivec3(32,32,32), mcol );
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 63, 0) -  ivec3(32,32,32), mcol );
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 0, 63) -  ivec3(32,32,32), mcol );
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(63, 63, 0) - ivec3(32,32,32), mcol );
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(0, 63, 63) - ivec3(32,32,32), mcol );
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(63, 63, 63) -ivec3(32,32,32), mcol );
					imageStore(lightBounceTex_output, ivec3(offset_start_coord) + ivec3(coneDir *travelled_voxelDistance) + ivec3(63, 0, 63) - ivec3(32,32,32), mcol );
				}*/

				//END CONE DEBUGGING

				//increase the travel distance by AT LEAST 1.3, else by the size
				//of the mipmap's expected size
				travelled_voxelDistance += max(1.3, expected_voxel_voxelSize); //times 2 for further GI spread
			}

			//TODO how about doing each cone with a separate work unit?

			//allow contribution from all cones to add up to 1
			//times (1/7) == 0.1429
			finalColor += (coneWeights[cone] * color_sampledByCone);
		}//end for each cone


		//color of the starting voxel, will be modulating all of the light that's bounce
		//onto our surface by it.
		//NOTICE we are using start_coord, and NOT the offset_start_coord, since we need to
		//get the value at the starting voxel.
		vec4 ownBasecolor = textureLod( basecolorTex,
										vec3(start_coord + 0.5) / numTexels,  0 );

		//notice, we allowed ALL cone colors to ADD up, THEN multiply the result by our own color,
		//(non-altered by any light's lambert or shadows, - just our pure material color).
		//It's modulated by this incoming, indirect illumination originating from diffuse colors of neighbors.
		finalColor *= (ownBasecolor.rgba);

		//after all the cones performed sampling we can store finalColor in place of our
		//voxel, for the 1st-bounce GI output texture:
		//recall that our normal texture stores emisive (self-glow) brightness of a starting voxel.
		//We won't mipmap normal texture, but will mipmap lightBounceTex. Hence,
		//make sure to copy brightness into lightBounceTex's alpha!
		imageStore(lightBounceTex_output, start_coord, vec4(finalColor.rgb, voxelNormal.a));

	}//end else if normal wasn't zero (we've sampled empty voxel

	//imageStore(lightBounceTex_output, start_coord, vec4(0));
	//imageStore(lightBounceTex_output, start_coord,   vec4( mod(vec3(start_coord), 128)/ 128, 1) );
		
}//end main()



