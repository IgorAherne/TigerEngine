#version 330

uniform sampler2D scene_depthTex; //depth as seen by the viewer
uniform sampler2D lowRes_Tex; //texture seen by the viewer, to be upscled.
uniform sampler2D highRes_NormTex; //Normals of the scene, as seen by viewer.

uniform vec2 lowResNumTexels;  //width and height in terms of num of pixels for low-res Global Illumination tex.
uniform vec2 highRes_PixelSizes; //Dimensions of A SINGLE PIXEL on the texture (1 is the full texture size).

uniform vec2 nearFarRange; //distances to the near and far planes in the view frustum.


out vec4 fragColor[2]; //0 - diffuse, 1 - specularity of the lights that output into these 2 textures.




//takes 0-1 depth value, then outputs the distance to that fragment, in world units.
float z_to_worldDistance(float z_depth){

    float z_unpacked = 2.0 * z_depth - 1.0;
	
    float dist =    2.0 * nearFarRange.x * nearFarRange.y 
					/  (  nearFarRange.y + nearFarRange.x
						  - z_unpacked*(nearFarRange.y - nearFarRange.x)  );
	return dist;
}





void main(){
	//get current screen fragment coordinates in 0-1 uv range:
	vec2 screen_uv = gl_FragCoord.xy*highRes_PixelSizes;
	
	//actual coordinates of the fragment centre (the fragment from high-resolution texture)
	vec2 high_fragCenter_uv = (gl_FragCoord.xy+0.5)*highRes_PixelSizes;
	
	// we need to pick 4 fagments from the low-res texture.
	//Using them, we will perform interpolation if the certain contidions are met.
	
		//find out the main, low-res texture fragment to which this, current high-res
		//fragment corresponds:
		
			//find how many pixels it takes to get to the corresponding low-res fragment's center:
		vec2 lowResfrag_CenterCoord = floor(lowResNumTexels * screen_uv) + 0.5;
		
		vec2 lowPixelSizes = 1 / lowResNumTexels;
		//determine the uv_coordinate of such center:
		vec2 low_fragCenter_uv = lowResfrag_CenterCoord * lowPixelSizes;

	//see if the uv coords of the main, high-res fragment lie  leftwards/rightwards, above/below,
	//of the corresponding low-fragment's center:
	
			float horizontality = 1;   //1.0 = lies to the right of the center,   -1.0 to the left
			float verticality = 1;     //1.0 = lies above,    					-1.0 = lies below
			
			vec2 difference = high_fragCenter_uv - low_fragCenter_uv;
			
			//will be either +1 or -1
			//We cannot use sign() since it might return zero.
			horizontality = step(0, difference.x) * 2 - 1;
			verticality = step(0, difference.y) * 2 - 1;
	
	//get uv coordinates of the centers of neighboring texels on the low-Res texture:
	 vec2 main_lowCoord_uv = low_fragCenter_uv;
	 vec2 horiz_low_neighborCoord_uv = main_lowCoord_uv + lowPixelSizes*vec2(horizontality, 0);
	 vec2 vert_low_neighborCoord_uv = main_lowCoord_uv + lowPixelSizes*vec2(0, verticality);
	 vec2 diag_low_neighborCoord_uv = main_lowCoord_uv + lowPixelSizes*vec2(horizontality, verticality);
	//now that we have uv-coordinates of 4 low-fragment centers, we can get 4 colors and 4 normals!
	
	vec3 normMain = normalize(  texture(highRes_NormTex, main_lowCoord_uv).rgb * 2.0 - 1.0    );
	vec3 normHoriz = normalize(	texture(highRes_NormTex, horiz_low_neighborCoord_uv.xy).rgb * 2.0 - 1.0    );
	vec3 normVert = normalize(	texture(highRes_NormTex, vert_low_neighborCoord_uv.xy).rgb * 2.0 - 1.0    );
	vec3 normDiag = normalize(	texture(highRes_NormTex, diag_low_neighborCoord_uv.xy).rgb * 2.0 - 1.0	  );
	
	
	//In order to be able to interpolate, the low-res normals must be pointing in the same direction
	float should_continue =   max(0,  dot(normMain,normHoriz))
							* max(0,  dot(normMain,normVert))
							* max(0,  dot(normHoriz, normVert))
							 * max(0,  dot(normDiag, normMain))
							 * max(0,  dot(normDiag, normVert))
							 * max(0,  dot(normDiag, normHoriz)); //all vectors are normalized


	should_continue = step(0.8, should_continue); //pass if should_continue is greater than threshold
	
	//In addition, we need to see if their depths are reasonably close to each other:

	float dist_to_mainLow = z_to_worldDistance(texture(scene_depthTex, main_lowCoord_uv).r);
	float dist_to_horizLow = z_to_worldDistance(texture(scene_depthTex, horiz_low_neighborCoord_uv).r);
	float dist_to_vertLow = z_to_worldDistance(texture(scene_depthTex, vert_low_neighborCoord_uv).r);
	float dist_to_diagLow = z_to_worldDistance(texture(scene_depthTex, diag_low_neighborCoord_uv).r);

	float thresh = 0.05; //world units

	should_continue     *=        step(abs(dist_to_horizLow - dist_to_mainLow), thresh)
						        * step(abs(dist_to_vertLow - dist_to_mainLow), thresh)
						        * step(abs(dist_to_horizLow - dist_to_vertLow), thresh)
								 * step(abs(dist_to_diagLow - dist_to_mainLow), thresh)
						         * step(abs(dist_to_diagLow - dist_to_vertLow), thresh)
						         * step(abs(dist_to_diagLow - dist_to_horizLow), thresh);
	//if any of those 4 distances are not close enough - don't allow for interpolation 
    //for this 	fragment. It will have to recieve the full re-computation at full resolution on the final pass.
	if( should_continue == 0.0){
		discard;
	}
	//otherwise, this fragment can be recieve interpolated colors from 4, low-res-texture texels.
	

	fragColor[0] = texture(lowRes_Tex, (gl_FragCoord.xy + 0.5)*highRes_PixelSizes
							+ highRes_PixelSizes*vec2(horizontality, verticality)
							);
}

