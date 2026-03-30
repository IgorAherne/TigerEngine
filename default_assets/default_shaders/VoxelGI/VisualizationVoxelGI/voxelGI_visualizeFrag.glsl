#version 330


uniform sampler3D voxelVolumeTex; //base color of the object's material.
uniform vec3 cameraPos;

uniform float textureSize; //the length of the 3d texture's dimension.
uniform float texelSize; //the length of a 3d texture's texel (along x,y or z)


in vec3 eyeDir;


out vec4 fragColor;   //output of the light's GI contribution


//declare the signatures of further functions:
void volume_insertion_exit(out vec3 enter_point, out vec3 exit_point);

float isInsideTextureVol(vec3 test_point);
	
	
	
	
void main(){
	
	fragColor = vec4(0,0,0,1);
	
	//determine the point of  view vector's "insertion" and "exit" into/out of the cube:
	vec3 enter_point;
	vec3 exit_point;
	
	volume_insertion_exit(enter_point, exit_point);
	

	//if the enter point == 0, we will just return, since the ray didn't cross the cube border.
	//there are no voxels to be traversed within the volume:	
	if(enter_point == 0){
		return;
	} 
	
	
	
	vec3 viewDir = normalize(eyeDir); //world-space direction towards the fragment from camera

	vec3 currLoc = enter_point + 0.001*viewDir;   
	
	vec3 old_voxelCenter = vec3(floor(currLoc / texelSize)*texelSize + 0.5*texelSize); //center coordinate of the enter voxel.
	vec3 voxelCenter;

	
	//while we are in between enter and exit point, keep shifting along the ray, gradually approaching the camera:
	while(dot(enter_point-currLoc, exit_point-currLoc) < 0){
		
		currLoc += 0.1*viewDir;
		voxelCenter = vec3(floor(currLoc / texelSize)*texelSize + 0.5*texelSize);

		
			old_voxelCenter = voxelCenter;

			vec3 uvw_vector = currLoc / (textureSize);  //get how many percent of texture sizes our cameraPos vector extends along each dimention.
			uvw_vector += 0.5;  //re-specify this "-1 to +1" vector  to be relative to the nearest, bottom left corner of the 3d texture.
			uvw_vector = clamp(uvw_vector, 0, 1);

			vec3 newCol = textureLod(voxelVolumeTex, uvw_vector, 0).rgb;
			if (dot(newCol, newCol) > 0) { //if the encountered voxel is not empty, stop there.
				fragColor.rgb = newCol.rgb; //sample out 3d texture, getting a next value of voxels.
				break;
			}

	}//end while()
	
	fragColor.a = 1.0;
} //end main()





void volume_insertion_exit(out vec3 enter_point, out vec3 exit_point){
	//all the normals of the 3d-texture volume's bounding planes:
	//(the distance to each of the plane is always half of the textureSize)
	const vec3[6] main_plane_normals = vec3[](	vec3(-1,0,0), 
												vec3(1,0,0), 
												vec3(0,-1,0), 
												vec3(0,1,0), 
												vec3(0,0,-1), 
												vec3(0,0,1)   );
												
	
		
		//here we will output intersections with our 6 planes which define faces above:
		vec3[6] intrsct_points;
		vec3 viewDir = normalize(eyeDir); //world-space direction towards the fragment from camera
		
		//find the 6 intersections:
		for(int iteration =0; iteration  < 6; iteration++){
			vec3 curr_plane_norm = main_plane_normals[iteration];
			
			float t =    (  (textureSize*0.5) -dot(curr_plane_norm, cameraPos)  )   /   dot(curr_plane_norm, viewDir);
			
			//we add epsilon, to make sure intrsct point is on the other side of the plane.
			intrsct_points[iteration] = cameraPos +  t*viewDir; 
		}
		
		
		
			
			//if camera happens to be inside of the volume, then the enter point is camera Position,
			//exit point is the nearest intersection point (which is NOT behind a camera):
			if(isInsideTextureVol(cameraPos) > 0.0001){
					enter_point = cameraPos;
					float nearest_dist = 99999999;

					for(int i = 0; i < 6; i++){
						float dist =length(intrsct_points[i] - enter_point);
						
						if(dot(intrsct_points[i]-enter_point, viewDir) < 0){ //intersection points behind camera are to be neglected:
							dist = 999999999; 
						}
						
						if(dist < nearest_dist){
							nearest_dist = dist;
							exit_point = intrsct_points[i];
						}
					}//end for-loop
					
					return;
			} //end if camera is inside the texture volume
	
	
	
	
	//here we know the camera is OUTSIDE of the texture volume, otherwise the previous
	//block would already return.
	//Now that we posess 6 intersections with the planes, we need to throw out 4.
	//There will be EXACTLY TWO insertion in the cube, and their components will fit into the texture cube.
	//Otherwise there will be NO INTERSECTIONS AT ALL.
	
	
	int[2] true_enter_exit_plane_ixs = int[](-1,-1);
	int counter = 0;
	
	for(int iteration = 0; iteration < 6; iteration++){
		if( isInsideTextureVol(intrsct_points[iteration] ) > 0.0001){
			true_enter_exit_plane_ixs[counter] = iteration; //insert into one of the two cells.
			counter++;
		}
	}
	
	
			//if one of the indices is still "-1", then we know that there are no
			//points on the surface of the CUBE and NO PLANES WERE SELECTED.
			if(step(0,float(true_enter_exit_plane_ixs[0])) * step(0, float(true_enter_exit_plane_ixs[1])) < 0){
				enter_point = exit_point = vec3(0.0);
				return;
			}

			
			
	//determine if the points are behind the camera 
	//(if at least one is behind - both are behind (since camera is outside of the cube by now), 
	//and should be discarded)
	
			if(   step(0, dot(intrsct_points[true_enter_exit_plane_ixs[0]] - cameraPos, viewDir)) 
				 *step(0, dot(intrsct_points[true_enter_exit_plane_ixs[1]] - cameraPos, viewDir))  == 0){
				
				enter_point = exit_point = vec3(0.0);
				return;
			}
	
	//Finally, here we know that:
	//a) camera is OUTSIDE of the cube
	//b) there are TWO intersection points
	//c) BOTH of these intersections sit on the SURFACE of the cube
	//d) both are IN FRONT of the camera.
	
	//enter point will be the nearest one, exit will be the other one:
	
			float dist0 =length(intrsct_points[true_enter_exit_plane_ixs[0]] - cameraPos);
			float dist1 =length(intrsct_points[true_enter_exit_plane_ixs[1]] - cameraPos);
			

			//now, if dist1 > dist0, we keep enter point based on the "true_enter_exit_plane_index 0",
			//and exit on index 1.   Otherwise we exchange those indices.
			float d1_is_greater_than_d0 = step(dist0, dist1);

			enter_point = d1_is_greater_than_d0 * intrsct_points[true_enter_exit_plane_ixs[0]] 
						+ (1 - d1_is_greater_than_d0)* intrsct_points[true_enter_exit_plane_ixs[1]];

			exit_point = (1 - d1_is_greater_than_d0) * intrsct_points[true_enter_exit_plane_ixs[0]] 
						+ d1_is_greater_than_d0 * intrsct_points[true_enter_exit_plane_ixs[1]];
			
	return; 
}





// return 1 if v inside the box, returns 0 otherwise
float isInsideTextureVol(vec3 test_point) {
	//notice how we add epsilon to both min and max points to account for float errors:
    vec3 s =   step( vec3(-textureSize-0.0003)*0.5, test_point )   -   step( vec3(textureSize+0.0003)*0.5, test_point );
    return s.x * s.y * s.z;   
}