#version 450

layout(triangles) in;

layout(triangle_strip, max_vertices=9) out;
uniform float textureSize; //how large each texture is in each dimension
uniform float texelSize; //how large is voxel of our 3d texture (measured in world units)

in vec3 world_position[]; //an array of world positions (1 per incoming vertex of triangle
in vec3 world_normal[]; 

out vec3 world_norm; 
out vec3 world_pos;
uniform float control;
uniform float control2;


void main(){

			vec2 nf_planes = vec2( -textureSize*0.5,  textureSize*0.5);
			//now we have near-far planes stored in "nf_planes" vector, let's make our projection

			
			mat4 orthoMat = mat4(	vec4(2/textureSize,  0, 0, 0),//distance between right-left planes
									vec4(0, 2/textureSize,  0, 0), //distance between bottom-top
									vec4(0, 0,    2/textureSize, 0), //distance between far-near planes
									vec4(0, 0,	0, 1)  
								 );

			//find out the direction a triangle is facing.
			//We shouldn't use world_normal input, because normals might not be perpendicular 
			//to the actual surface (cortesy of 3D artist)
			vec3 crossVec = cross(world_position[1] - world_position[0], world_position[2] - world_position[0]);
			crossVec = normalize(crossVec);

			vec3 absCrossVec = abs(crossVec);

			//determine the most dominant axis of a triangle, to rasterize it in the appropriate direction:
		
		const mat3 xViewMat = mat3(	vec3(0.000001,0,1), //looking in negative x-direction
									vec3(0,1,0), 
									vec3(-1,0, 0.000001));
		
		const mat3 yViewMat = mat3(	vec3(1, 0, 0), //looking in negative y-direction
									vec3(0, 0.000001,1), 
									vec3(0, -1, 0.000001));
		//for z matrix we will keep viewport unchanged, since it's already z-aligned:
		const mat3 zViewMat = mat3(	vec3(1, 0, 0),
									vec3(0, 1, 0),
									vec3(0, 0, 1));

		//permutations. Will contain 1,1,0 if  x > y,   x > z   y < z
		vec3 XvsY_XvsZ_YvsZ = vec3( step(absCrossVec.y, absCrossVec.x),
									step(absCrossVec.z, absCrossVec.x),
									step(absCrossVec.z, absCrossVec.y) );

		


								 //x vs z (if  XvsY_XvsZ_YvsZ.b == 1, xViewMat will be selected, since x was greater than z):
		mat4 finalViewMat =	 mat4(	 (((zViewMat * (1 - XvsY_XvsZ_YvsZ.g) + xViewMat * XvsY_XvsZ_YvsZ.g)))  * (1 - XvsY_XvsZ_YvsZ.b) 
								   + (((yViewMat * (1 - XvsY_XvsZ_YvsZ.r) + xViewMat * XvsY_XvsZ_YvsZ.r)))  * XvsY_XvsZ_YvsZ.b
								 );
		finalViewMat[3].w = 1;





		//both triangles would be offset backwards (2nd will be offset more).
		float quarter_voxel_diagonal = length(vec3(texelSize))*0.3;
		//for greater offset ONLY multiply this 'offset':
		//DON'T TOUCH quarter_voxel_diagonal or offset2.
		vec3 offset = vec3(0);// quarter_voxel_diagonal*crossVec;

		//vec3 offset2 = offset + texelSize*crossVec;


		//this first one will stay as is
		gl_Position = orthoMat * finalViewMat * vec4(world_position[0]- offset, 1.0);
		world_norm = world_normal[0];
		world_pos = world_position[0] - offset;
		EmitVertex();
		 
		gl_Position = orthoMat * finalViewMat * vec4(world_position[1]- offset, 1.0);
		world_norm = world_normal[1];
		world_pos = world_position[1] - offset;
		EmitVertex();

		gl_Position = orthoMat * finalViewMat * vec4(world_position[2]- offset, 1.0);
		world_norm = world_normal[2];
		world_pos = world_position[2] - offset;
		EmitVertex();

		EndPrimitive();


		//now we need a second triangle, which will be offset backwards.
		//this will solve an issue where "voxel staircase" is too thin and voxels end up
		//touching each other with only 1 edge. Now we will have a propper, conservative 
		//span of voxels, a "thick staircase" which the triangle generates.


		/*gl_Position = orthoMat * finalViewMat *  vec4(world_position[0], 1.0);
		world_norm = world_normal[0];
		world_pos = world_position[0];
		EmitVertex();

		gl_Position = orthoMat * finalViewMat *  vec4(world_position[1], 1.0);
		world_norm = world_normal[1];
		world_pos = world_position[1];
		EmitVertex();

		gl_Position = orthoMat * finalViewMat *  vec4(world_position[2], 1.0);
		world_norm = world_normal[2];
		world_pos = world_position[2];
		EmitVertex();

		EndPrimitive();*/

}