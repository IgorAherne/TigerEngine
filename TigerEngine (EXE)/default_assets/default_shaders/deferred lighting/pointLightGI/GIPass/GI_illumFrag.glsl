#version 330


uniform mat4  inverseVP;
uniform vec3  cameraPos;
uniform float lightRange;
uniform vec3  lightPos;

uniform vec2  pixelSize;
uniform float lightPixelSize; //a size of a pixel in light's cubemaps.


uniform sampler2D	 		scene_normalTex;
uniform sampler2D			scene_depthTex;
uniform samplerCube    		light_positionTex;
uniform samplerCube			light_normalTex;
uniform samplerCube			light_fluxTex;



out vec4 fragColor[2];   //output of the light's GI contribution


//forward declare the functions used by main():
 vec4 lightPovPixelGI(vec3, vec3, vec3);
 vec4 gatherBounces(vec3, vec3);


 

vec4 gatherBounces(vec3 sceneFragPos_world, vec3 sceneFragNorm_world){

	vec3 lightPovCoord = sceneFragPos_world - lightPos;

	
	//we now need to inspect all of the adjacent pixels in the light's point of view.
	//Such neighboring pixels will be nearby lightPovCoord,
	//and represent a pyramid-like deviation.
	//The number of x, y, z samplings of cubemap  will be kernel_GI*2, for x, y and z offsets,
	//because we will sample to the left and to the right of the lightPovCoord,  up, down, 
	//backwards, forwards,  - in a pyramid-like sampling pattern.
	
	lightPovCoord.xyz = normalize(lightPovCoord);
	const int kernel_GI = 3; 
	vec4 final_GI_color = vec4(0.0);  //here we will accomulate the color resulting from GI bounces
	
	for(int x = -kernel_GI; x < kernel_GI + 1; x++){
		for(int y = -kernel_GI; y < kernel_GI + 1;  y++){
			for(int z = -kernel_GI; z < kernel_GI + 1;  z++){
				
				  vec3 sampling_vec = lightPovCoord + vec3(x*lightPixelSize*32, 
														   y*lightPixelSize*32, 
														   z*lightPixelSize*32);
				  										 
				  final_GI_color +=	 lightPovPixelGI( sampling_vec, 
				  									  sceneFragPos_world, 
				  									  sceneFragNorm_world );
			}//end for z
		}//end for y
	}//end for x
	
	return final_GI_color / pow(float(kernel_GI) * 2 + 1, 3);
}





//determines the color contribution to the main, sceneFrag_world fragment from this 
//nearby fragment on which the light shines.
//Supply it the direction towards such nearby fragment. 
//It will be used to sample the light cubemaps

vec4 lightPovPixelGI(vec3 lightPovCoord, vec3 sceneFrag_world, vec3 sceneFragNorm_world){
	
	//get the normal of the pixel that light sees after sampling 
	//in the direction of the "main",  sceneFragPos coordinate.
	//The normal will be in world space after unpacking, because it was stored as such.
	//It also will be already normalized.
	vec3 lightPovNorm_world = texture(light_normalTex, lightPovCoord).rgb*2 - 1;
	
	//get the world position of that pixel too
	vec3 lightPovCoord_ndc =  texture(light_positionTex, lightPovCoord).rgb*2 - 1;
	//ndc position reflects precentage within the light range 
	//(100% = light range, 0% = world light position). To get world position of such fragment,
	//we add the lightPosition after it was unpacked.
	//Position is offsetted backwards by its normal. This solves the issue of GI being
	//too pale in the corner areas. Don't overdo it thought! It reduces the GI brigtness!
	vec3 lightPovFrag_world =  lightPovCoord_ndc*lightRange + lightPos - lightPovNorm_world;
	

	//if the length between world positions of our sceneFrag and the povFrag is less than
	//the set thershold - 
	//then they are the same pixel, and no value should be calculated from it.
	//In that cast pov_isNot_sceneFrag will become 0, otherwise, it's a different frament,
	//and pov_isNot_sceneFrag will become 1
	float pov_isNot_sceneFrag = step( 0.2, length(lightPovFrag_world - sceneFrag_world));
	
	
	//get the color of the surface whicht he light hits in the required direction.
	vec4 povColor_and_flux = texture(light_fluxTex, lightPovCoord);
	
	
	
	//see how much the fragment visible from light is facing towards light:
	float enter_dot =  dot(  normalize(-lightPovCoord), lightPovNorm_world);
	enter_dot = max(enter_dot, 0); //don't allow for negative dots
	
	float exit_dot = dot(normalize(sceneFrag_world - lightPovFrag_world), lightPovNorm_world);
	exit_dot = max(exit_dot,0); //don't allow for negative dots
	
	float arrive_dot = dot(normalize(lightPovFrag_world - sceneFrag_world), sceneFragNorm_world);
	arrive_dot = max(arrive_dot, 0);
	
	
	//we also know the range of light.
	//Let's finally calculate the contribution of the requested light fragment, to
	//the main, scene framgent, for which the whole hustle is happening:
	
	
		//calculate the attenuation of the main, sceneFrag, once the light travelled the
		//distance   from light -> to povFrag -> to sceneFrag
		
		//distance from light to the fragment visible from light:
		float dist_light_to_PovCoord = length(lightPovFrag_world - lightPos);
		
		//distance from fragment visible from light towards the main, scene fragment (in world space):
		float dist_PovFrag_to_sceneFragCoord = length(sceneFrag_world - lightPovFrag_world);
		
		
		float atten_sceneFrag = max(lightRange - (dist_light_to_PovCoord
											   + dist_PovFrag_to_sceneFragCoord),  0) / lightRange;   
		float sqr_atten = pow( atten_sceneFrag, 2);  //square falloff.
	
	
	//as you can see, final color of bounce depends on (is modified by): 
	// - color of the surface directly hit by light,
	// - flux of that surface (how much secondary light can escape the surface in the best scenario)
	// - pointedness of that surface  towards light
	// - how much that surface faces the main, scene fragment for which the whole hustle happens.
	//Notice that we subtract this dot from 1.  We need it to be at max when surfaces point 
	//towards each other, NOT when they are facing in the same direction
	//
	// - attenuation (how much light reached the main fragment after the total traveled distance)
	
	vec4 finalColor =     vec4(povColor_and_flux.rgb, 1)     * povColor_and_flux.a  
						  * enter_dot * exit_dot * arrive_dot * sqr_atten;
						
						
	return finalColor * 1.2;
}







void main(){
	vec2 screen_uv = gl_FragCoord.xy*pixelSize;
   
	vec4 sceneFrag_worldPos;
	float sceneFrag_z = texture(scene_depthTex, screen_uv).r;
	sceneFrag_worldPos = vec4(screen_uv,   sceneFrag_z,   1.0);
	sceneFrag_worldPos.xyz = sceneFrag_worldPos.xyz*2-1; //unpack position.
		
	sceneFrag_worldPos = inverseVP * sceneFrag_worldPos;
	sceneFrag_worldPos /= sceneFrag_worldPos.w;
	//now we have a world position of the fragment from the rendered scene.
	
	
	vec3 screenFragNormal_wrld = texture(scene_normalTex, screen_uv).rgb*2 - 1; //sample and unpack
	

	
	
	vec3 color_GI = gatherBounces(sceneFrag_worldPos.xyz, screenFragNormal_wrld).rgb;
	
	
	
	fragColor[0] = vec4(color_GI,  1.0); //diffuse color,  fragColor[1] would be specular
}