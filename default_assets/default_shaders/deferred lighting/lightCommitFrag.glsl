#version 330


uniform sampler2D	lightDiffuseTex;
uniform sampler2D	sceneBasecolorTex;
uniform vec2 		pixelSize; 

out vec4 fragColor;


void main(){
	vec2 screen_uv = gl_FragCoord.xy * pixelSize;
   
	vec4 sceneBasecolor = texture(sceneBasecolorTex, screen_uv);
	vec4 lightDiffuseTex = texture(lightDiffuseTex, screen_uv);
	
	sceneBasecolor *= (lightDiffuseTex);
	
	
	fragColor = vec4(sceneBasecolor.rgb,  1.0);
}