#version 330


uniform mat4  inverseVP;
uniform vec3  cameraPos;
uniform float lightRange;
uniform vec3  lightPos;
uniform vec2  pixelSize;

uniform sampler2D 			scene_normal_and_emissionTex;//rgb = normal a =emission
uniform sampler2D			scene_depthTex;
uniform samplerCube			lightDist_cubeTex;



out vec4 fragColor[2];



vec3 directIllumAtten(vec3 sceneFragPos, vec3 sceneFragNorm){
	float frag_dist = length(sceneFragPos.xyz - lightPos);

	vec3 to_light = normalize(lightPos - sceneFragPos.xyz);
	vec3 to_cam = normalize(cameraPos - sceneFragPos.xyz);
	
	float lambert = dot(to_light, sceneFragNorm);
	lambert = max(0, lambert);
	
	
	
	float atten = (lightRange - frag_dist) / lightRange;   //linear falloff
	float sqr_atten = pow(atten, 2);
	
	vec3 illum_color = vec3(lambert*sqr_atten);
	return illum_color;
}




float Tell_if_in_shadow(samplerCube distCubeTex, vec3 light_to_fragVector) {

	float sampledDistance =  texture(distCubeTex, light_to_fragVector).r;

	float distance = length(light_to_fragVector);

	//1 if fragment is not in shadow, 0 otherwise.
	//1 = (distance < sampledDistance)
	return step(distance + 0.01, sampledDistance);
}




void main(){
	vec2 screen_uv = gl_FragCoord.xy*pixelSize;
   
		vec4 screenFrag_worldPos;
		float screenFrag_z = texture(scene_depthTex, screen_uv).r;
		screenFrag_worldPos = vec4(screen_uv,   screenFrag_z,   1.0);
	screenFrag_worldPos.xyz = screenFrag_worldPos.xyz*2-1; //unpack position.
		
	screenFrag_worldPos = inverseVP * screenFrag_worldPos;
	screenFrag_worldPos /= screenFrag_worldPos.w;
	//now we have a world position of the fragment from the rendered scene.
	
	//sample and unpack:
	vec4 screenFragNormal_wrld = texture(scene_normal_and_emissionTex, screen_uv)*2 - 1;
	
	
	vec3 illum_color = directIllumAtten( screenFrag_worldPos.xyz, 
										 screenFragNormal_wrld.xyz);
	
	float shadow = Tell_if_in_shadow(lightDist_cubeTex, screenFrag_worldPos.xyz-lightPos);
	
	
	//diffuse color,  fragColor[1] would be specular
	fragColor[0] = vec4(illum_color*shadow, 1);
}