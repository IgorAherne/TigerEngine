#version 330

//full-resolution lambertian-shaded and shadowmapped, visible portion of the scene:
uniform sampler2D hq_shadingTex; 

//final GI contribution, after all the previous interpolation stages.
uniform sampler2D GI_illuminationTex;

uniform vec2 pixelSize;


out vec4 fragColor[2]; //0 - diffuse, 1 - specularity of the lights that output into these 2 textures.



void main(){
	vec2 screen_uv = vec2(gl_FragCoord.x, gl_FragCoord.y)*pixelSize;
	

	//perform slight gaussian blur on the global illumination
	vec3 GIcolor = vec3(0.0);
	const int blur_kernel = 1;
	for(int x = -blur_kernel; x < blur_kernel+1; ++x){
		for(int y = -blur_kernel; y < blur_kernel+1; ++y){
			GIcolor += texture(GI_illuminationTex, screen_uv  + pixelSize * vec2(float(x), float(y))).rgb;
		}
	}
	
	GIcolor /= pow(float(blur_kernel)*2 + 1, 2);
	
	
	
	vec3 shadingColor = texture(hq_shadingTex, screen_uv).rgb;
	
	//we want to provide the GI contribution to the existing lambertian shading.
	//But, we want to have a somewhat average of those 2 colors, hence *0.5 is included :
	vec3 finalColor = GIcolor + shadingColor*0.5;

	//tone mapping to preserve color hue in HDR regions:
	//Luminance-based Reinhard: preserves color saturation by scaling all channels equally.
	float lum = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
	float mappedLum = lum / (1.0 + lum);
	fragColor[0] = vec4( finalColor * (mappedLum / max(lum, 0.001)),   1.0);
}

