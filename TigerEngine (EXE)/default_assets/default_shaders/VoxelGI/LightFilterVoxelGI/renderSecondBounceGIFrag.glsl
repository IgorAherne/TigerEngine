#version 450
#pragma optionNV (unroll all) //make sure all loops are unrolled (must be based on constants though)


uniform sampler3D	diffuseTex;//illumination and shadows * basecolor, with transparency in alpha. 
							   //Recall that earlier, when diffuse was computed, if light saw that object was self-emissive
							   //it WOULD make diffuse == basecolor, without shading it or anything.
							//That's why we didn't include basecolorTex in this shader
					
							   
//texture with 1st bounce of light, that were ALREADY modulated by basecolors.
//In alpha it contains emissive (self-glow) brightness of a given voxel.
//since it's mipmapped, it can be used by nearby voxels during Cone Tracing, along with
//diffuseTex. That alpha should be used to increase the self-illumination brightness
uniform sampler3D	bounce_and_brightness_Tex; 


uniform sampler2D	scene_baseColorTex; //"raw" material color of surfaces seem by the camera.
uniform sampler2D	scene_depthTex; //depth of the scene, as seen by camera

//we will use this one to restore world pos and orientation of fragment.
//we can also get "self-glow factor", emission factor from alpha
uniform sampler2D 	scene_normal_and_emissionTex;

uniform sampler2D	scene_illumTex; //lights' shadows, attenuation, etc.  
								//to get diffuse of the visible fragments, we need   scene_baseColor * scene_illum

uniform sampler2D rgb_noiseTex; //allows us to get random rotation angle, via its noise.

uniform float control;
uniform float control2;
uniform float control3;//variables for debugging and finetuning the GI settings
uniform float control4;
uniform float control5;

uniform mat4 inverseVP; //inverse view projection matrix to bring coords from NDC to world space.

uniform float textureSize; //how big is texture in terms of world-space units.
//how large is each voxel of our 3d textures in terms of world-space size:
uniform float voxel_world_size; 

uniform int numVoxels; //the number of voxels in each dimention of our 3d textures:

uniform vec3 cameraPos; //location of camera in the world space.

uniform vec2 pixelSize; //a fraction, denoting how big is each texel in render textures.


out vec4 fragColor;


//forward-declare function signatures:


// 0.37 == 22 deg (full cone will be 44 deg)
vec4 sample_withCone(vec3 uv_world_pos, vec3 start_voxel_voxelCoord, vec3 coneDir, const int maxIter,  float cone_halfAngle = 0.37);



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
	 vec3 product = floor(0.9999 + max(vec3(0)+coord, -0.1) * max(vec3(1)-coord, -0.1));

	 return product.x*product.y*product.z;
}



void main() {
	//calculate world position that the fragment has, then use to perform conetracing
	//from this coordinate.
	vec2 screen_uv = gl_FragCoord.xy*pixelSize;

	vec4 screenFrag_worldPos;
	float screenFrag_z = texture(scene_depthTex, screen_uv).r;
	screenFrag_worldPos = vec4(screen_uv, screenFrag_z, 1.0);
	screenFrag_worldPos.xyz = screenFrag_worldPos.xyz * 2 - 1; //unpack position.

	screenFrag_worldPos = inverseVP * screenFrag_worldPos;
	screenFrag_worldPos /= screenFrag_worldPos.w;
	//now we have a world position of the fragment from the rendered scene.


	//similarly, get the normal (which is already encoded as world space, just need
	//to unpack it into "-1 to 1" range):
	//NOTICE we keep it as vec4 to access Alpha (self-glow emission) later on.
	vec4 normal = texture(scene_normal_and_emissionTex, screen_uv).rgba;
	normal.xyz = normalize(normal.xyz * 2 - 1);
//TODO if screen_normal texture has normalmapping, normal might 
//not actually correspond to geometry direction.

	//calculate direction to the Original fragment 
	//(this vector will be heavily used in reflection via voxel-cone tracing, a.k.a. VCT)
	vec3 camToFragDir = normalize(screenFrag_worldPos.xyz - cameraPos);


	//before we begin, there are two types of distance measuring:
	//world-units
	//voxel-units (1 means 1 voxel). This one is used to fetch appropriate texel when needed.

	//extend the normal to be outiside of its current voxel:
	vec3 uv_world_pos = (screenFrag_worldPos.xyz+normal.xyz*voxel_world_size*1.7) / textureSize + 0.5;
	

			//those are the normalized directions for NON-ROTATED CONE bouquet, looking upwards.
			vec3 allConeDirs[7] = vec3[7](	vec3(0, 1, 0.00001),
											vec3(0.766, 0.6428, 0),
											vec3(0.383, 0.6428, 0.6634),
											vec3(-0.383, 0.6428, 0.6634),
											vec3(-0.766, 0.6428, 0),
											vec3(-0.383, 0.6428, -0.6428),
											vec3(0.383, 0.6428, -0.6428)
										  );


			//we want to aling cones to the normal, and also spin it by random angle
			//arround that normal, once they were aligned to it.
			//Get the noise color for that matter:
			vec3 noiseColor = texture(rgb_noiseTex, vec2(uv_world_pos.x*400 + uv_world_pos.y*400,
														 uv_world_pos.y*400 + uv_world_pos.z*400)).rgb;

			float rand_angle = 0;

			rand_angle = (noiseColor.r + noiseColor.g + noiseColor.b)*0.33333*3.14159;

			float c = cos(rand_angle);
			float s = sin(rand_angle);

			mat3 randSpin_mat = mat3(vec3(c, 0, s), vec3(0, 1, 0), vec3(-s, 0, c));

			//build rotaion matrix, creating a rotation which will
			//aling our upwards-looking cone with the current normal.
			//THIS FUNCTION IS FASTER BY 28% than the following approach:
			//1) binorm = cross(normal, vec(1,0,0));
			//2) binorm = normalize(binorm);
			//3) cross(binorm, normal) to get the true, final tangent,
			//4) constructing mat3(tangent, normal, binormal)
			//notice how we first multiply by our randSpin matrix:
			 

			mat3 orient_mat = matrixFromAxisAngle(allConeDirs[0], normal.xyz)*randSpin_mat;

			allConeDirs[0] = orient_mat * allConeDirs[0];
			allConeDirs[1] = orient_mat * allConeDirs[1];
			allConeDirs[2] = orient_mat * allConeDirs[2];
			allConeDirs[3] = orient_mat * allConeDirs[3];
			allConeDirs[4] = orient_mat * allConeDirs[4];
			allConeDirs[5] = orient_mat * allConeDirs[5];
			allConeDirs[6] = orient_mat * allConeDirs[6];



		vec4 firstSecond_bounce_light = vec4(0, 0, 0, 0);

			//this is how many cones we will use:
			const int cone_num = 7;


			//perform voxel cone tracing with 7 cones, getting JUST THE FIRST BOUNCE for the visible fragment.
			//This means sampling nearby illumination*basecolor voxels:
			//TODO make sure offset position is supplied for this fragment, and it won't sample its voxel.

			vec3 start_voxel_Coord = floor(uv_world_pos * numVoxels);

			//cosine-weighted solid-angle weights: center cone ~0.206, side cones ~0.132
			float coneWeights[7] = float[7](0.206, 0.132, 0.132, 0.132, 0.132, 0.132, 0.132);

			for (int cone = 0; cone < cone_num; cone++) {
				//TODO make sure shader will unwind the forloop during compolation

				vec3 coneDir = allConeDirs[cone];

				//allow contribution from all cones to add up to 1
				//times (1/7) == 0.1429
				//uv_world_pos is world position relative to the width, height and depth of the voxel structure.
				//Hence, it's in 0-to-1 range and can be used for sampling.
				//will use default cone aperture of 0.37 == 22 degrees in radian. 
				//(full cone is 44 degrees in diameter)

				//max number of iterations is determined by the number mipmaps there will be in the bare minumum (log2),
				//and the cone size (how quickly we will be going towards next mipmap.
				//however, if we hardcode number by hand, shader will be able to unfold its for-loop.
				//So perhaps if we need to change this number we will need to modify source through C++ then re-compile
				const int numIter = 8; //int(log2(numVoxels + 1));
				firstSecond_bounce_light +=  coneWeights[cone] * sample_withCone(uv_world_pos, start_voxel_Coord, coneDir, numIter );
				
			}//end for each cone

			//reflection:
			firstSecond_bounce_light += 0.02 * control3 * sample_withCone(uv_world_pos, start_voxel_Coord,  reflect(camToFragDir, normal.xyz), 25, 0.1);

			//boost the Global illumination:
			firstSecond_bounce_light.rgb *= control5;
		
	

		//we will get the following fragment information from our renderer's deferred textures:

		//color of the starting voxel, will be modulating all of the light that's bounce
		//onto our surface by it.
		vec4 baseColor = texture(scene_baseColorTex, screen_uv);//also contains own transparency
		
		//diffuse of the fragment  = (MATERIAL BASECOLOR) * (ILLUMINATION COEFF FROM DEFERED PIPELINE)
		vec4 ownDiffuse = texture(scene_illumTex, screen_uv) * baseColor; 

		//apply ambient occlusion to direct light only.
		//Indirect light is already naturally occluded by alpha accumulation during cone marching.
		ownDiffuse.rgb *= firstSecond_bounce_light.a;

		//notice, we allowed ALL cone colors to ADD up, THEN multiply the result by our own color,
		//(non-altered by any light's lambert or shadows, - just our pure material color),
		//modulating it by this incoming, indirect illumination.
		firstSecond_bounce_light.rgb *= (baseColor.rgb*baseColor.a);

		 //after all the cones performed sampling we can return finalColor.
		float emission = normal.a;

		//if emission is close to 1, disregard light shading (own diffuse), but still
		//take into account illumination that might arrive from somewhere else (both bounces):
		//If emission is close to 0 disregard any "raw" basecolor, and consider 
		//own diffuse which is basecolor * illumination:
		fragColor.rgb =   (1-emission)*ownDiffuse.rgb + emission*baseColor.rgb
		 				+ firstSecond_bounce_light.rgb;//also add indirect illumination (first and second bounce)
		
		fragColor.a = 1;

		//visualize ambient occlusion for showreel:
		//NOTICE IT'S firstBounce.ALPHA, not the SECOND BOUNCE
		fragColor.rgb = fragColor.rgb *(1 - control4) + vec3(firstSecond_bounce_light.a)*control4;
		
		//dont't apply the color if the position was outside of the volume, since it's garbage:
		float within_bounds = isWithin01Bounds(uv_world_pos);
		fragColor *= within_bounds;
		//we didn't use 'if' statements, since it reduced performance.
}





vec4 sample_withCone(vec3 uv_world_pos, vec3 start_voxel_voxelCoord, vec3 coneDir, const int maxIter, float cone_halfAngle/* = 0.255*/) {

	//travelled_voxelDistance is what we start with as the "travelled_voxelDistance".
	//As we will get further and the cone will expand, we will 
	//have been travelling by the larger distance, hence this variable will grow:

	//Start at 1 voxel so each cone's first sample is at a distinct position.
	//At distance 0 all cones would sample the same point (the offset start),
	//wasting 6 of 7 texture fetches with no directional differentiation.
	float travelled_voxelDistance = 1;
	vec4 diffuse_sampledByCone = vec4(0, 0, 0, 0); //alpha is transparency
	vec4 bounce_sampledByCone = vec4(0, 0, 0, 0); //alpha is emissive (self-glow) brightness.
	float ambientFactor = 0; //the close to 1 the more in shadow the fragment is.

	//TODO since we will offset by conedir as soon as we enter forloop. maybe reduce this offset by normal * '2.2' down a little
	//to like 1.7 or so

	//we will be sampling textures in a moment, it is based on starting coord 
	//+ advancement along the cone direction.
	vec3 sampling_coord = uv_world_pos;


	//move along the cone direction:
	for (float i = 0; i < maxIter; i++) {

		//determine the value of mip from the current width of the cone at this 
		//sampling coord:
		//tan(theta) = opposite / adjacent  ==  half of mip texel size / distance traveled
		//opposite = tan(theta)*adjacent

		float expected_voxel_voxelSize = tan(cone_halfAngle) * travelled_voxelDistance * 2;

		//voxels have size 1x1x1 in mip lvl 0, 
		//size 2x2x2 in mip lvl 1,
		// 4x4x4 in mip lvl 2
		// 8x8x8 in mip lvl 3 etc
		float sampling_mip_lvl = log2(expected_voxel_voxelSize);
		//keep it as float, since we want to have interpolation between Mipmaps as well.
		//expected_voxel_voxelSize might be like 0.2101 etc, if the cone is that wide.
		//So it will take time to get to 1.000  (1st mip level) if cone is narrow.

		//we will sample our diffuse texture, to gather first bounce contribution for 
		//this fragment's cone:
		//Alpha contains transparency, will be useful to estimate when the cone should stop.
		vec4 foundDiffuse = vec4(textureLod(diffuseTex, sampling_coord, sampling_mip_lvl));
		  

		//we will sample 1st bounce texture to gather second bounce contribution for 
		//this fragment's cone:
		//recall that bounce_and_brightness_Tex contains emissive (self-glow) brightness in alpha.
		//We will use it to increase brightness of emissive objects.
		vec4 foundBounce = textureLod(bounce_and_brightness_Tex, sampling_coord, sampling_mip_lvl);
		//if sampling coord fell out from 0-to-1 range, we should discard any 
		//further encounted voxels:
		//this function is faster than the 'if' statement or the 2 step() functions.
		//We avoid using step() in the foorloops, since it provides great speed boosts.
		float within_bounds = isWithin01Bounds(sampling_coord);
		/*	if (within_bounds < 3) {
		break;
		}*/

		//see if the currently sampled value belongs to a voxel other than the starting voxel
		vec3 voxelDifference = start_voxel_voxelCoord - floor(sampling_coord*numVoxels);
		//voxelDifference will be 1 if the voxels are the same.
		voxelDifference = abs(voxelDifference);
		float isDifferentVoxel = clamp( max(voxelDifference.x, max(voxelDifference.y, voxelDifference.z)),  0,  1  );


		//accomulate diffuse encountered at this advancement along cone dir.
		//diffuse = basecolor*lighting. 
		//Recall that earlier, when diffuse was computed, if light saw that object was self-emissive
		//it would make diffuse == basecolor, without shading it or anything.
		//
		//1st bounce = sampled diffuses * own basecolor
		// 2nd bounce = sampled 1st bounces * own basecolor
		//total final color = own diffuse + 1st bounce + 2nd bounce.
		

		//We assume that correct (affected by their alpha) colors were plugged in from the very start.
		//Colors will lose their value as they will blend in with the black, empty voxels.
		//That's why we don't apply alpha from sampled diffuse onto that sampled diffuse.
		//Now, its alpha will only be used for opacity check, during this Voxel Cone Tracing.
		diffuse_sampledByCone.rgb +=  (1 - diffuse_sampledByCone.a)  //0-or-1
									* isDifferentVoxel
								    * vec3(foundDiffuse.rgb)
								    * within_bounds; // 0-or-1 //instead of 'if' (commented out above)
	
									

		//we still use alpha of diffuse here, since it contains transparency.
		//Once again, alpha of foundBounce contains emissive (self-glow) brightness in alpha.
		bounce_sampledByCone.rgb +=  (1 - diffuse_sampledByCone.a)
									* isDifferentVoxel
								    * vec3(foundBounce.rgb)
								    * within_bounds; //instead of 'if' (commented out above)
	
		ambientFactor += (1 - diffuse_sampledByCone.a)
						* within_bounds
						* isDifferentVoxel
						* clamp(5.5 - travelled_voxelDistance, 0, 1)
						* foundDiffuse.a;

		//CRITICAL: Alpha accumulated AFTER color:
		diffuse_sampledByCone.a += (1.0 - diffuse_sampledByCone.a) * isDifferentVoxel * foundDiffuse.a;

		//increase the travel distance by AT LEAST 1.3 voxel, else by the size
		//of the mipmap's expected voxel size
		travelled_voxelDistance += max(1.5, expected_voxel_voxelSize);
		
		//Advancement is brought back into  "0 to 1" range with division by  total number of texels
		//along the dimention of a texture: 
		sampling_coord = uv_world_pos + (coneDir*travelled_voxelDistance) / numVoxels;
	}

	//NOTICE, we will multiply by fragment's basecolor AFTER all the cones were collected. (AFTER this function)
	
	//boost second bounce a little (by 1.7)
	return vec4(diffuse_sampledByCone.rgb + control*bounce_sampledByCone.rgb,
				clamp(1 - ambientFactor, 0, 1));
}//end for each cone
