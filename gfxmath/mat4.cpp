#include "mat4.h"
#include <cmath>
#define PI 3.1415926



mat4 mat4::euler_rot(float x_deg, float y_deg, float z_deg) {
	x_deg = x_deg *0.0055555  * PI;  //*0.0055555  is 1/180.f
	y_deg = y_deg *0.0055555  * PI;
	z_deg = z_deg *0.0055555  * PI; //we have just made our degrees into radians.

	float c = std::cos(x_deg); //cos
	float s = std::sin(x_deg);  //sin
	mat4 xRot = mat4::mat_via_rows(  vec4(1, 0, 0,  0), 
									 vec4(0, c,-s,  0),
									 vec4(0, s, c,  0), 
									 vec4(0, 0, 0,  1)   );

	c = std::cos(y_deg); //cos
	s = std::sin(y_deg);  //sin
	mat4 yRot = mat4::mat_via_rows(	 vec4( c,  0, s, 0),
									 vec4( 0,  1, 0, 0),
									 vec4(-s,  0, c, 0),
									 vec4( 0,  0, 0, 1)  );

	c = std::cos(z_deg); //cos
	s = std::sin(z_deg);  //sin
	mat4 zRot = mat4::mat_via_rows(	 vec4(c, -s, 0, 0),
									 vec4(s,  c, 0, 0),
									 vec4(0,  0, 1, 0),
									 vec4(0,  0, 0, 1)	);

	return zRot * yRot * xRot; //XYZ order.
}



mat4 mat4::angleAxis_rot(vec3 axis, float deg) {
	deg = deg *0.0055555  * PI;  //*0.0055555  is 1/180.f
								 //we have just made our degrees into radians.

	float c = std::cos(deg); //cos
	float s = std::sin(deg);  //sin
	float t = 1.f - std::cos(deg); //tan

	float x = axis.x;
	float y = axis.y;
	float z = axis.z;
	mat4 rot = mat4::mat_via_rows( vec4(t*x*x +c,    t*x*y -s*z,  t*x*z +s*y,  0),
								   vec4(t*x*y +s*z,  t*y*y +c,    t*y*z -s*x,  0),
								   vec4(t*x*z -s*y,  t*y*z +s*x,  t*z*z +c,    0),
								   vec4(0,           0,           0,           1)  );
	return rot;
}



mat4 mat4::viewMatrix(vec3 curr_pos_world, vec3 pitch_yaw_roll_world) {
	mat4 rotationX = mat4::euler_rot(-pitch_yaw_roll_world.x, 0, 0);
	mat4 rotationY = mat4::euler_rot(0, -pitch_yaw_roll_world.y, 0);
	mat4 rotationZ = mat4::euler_rot(0, 0, -pitch_yaw_roll_world.z);

	return  rotationX *  rotationY * rotationZ * mat4::translation(-curr_pos_world);
}


//a more expensive, 2nd version of view matrix. More flexible however.
mat4 mat4::viewMatrix(const mat4 transf) {
	return transf.inverse();
}



//working one:
mat4 mat4::persp_mat(float inverse_aspect, vec2 n_f_distances, float fov_deg) {
	fov_deg = std::abs(fov_deg); //no negative fov plz!.
	float fov_radians = fov_deg *0.0055555 * PI; //*0.0055555  is 1/180.f
									//we have just made our fov degrees into radians.
	fov_radians *= 0.5f; //need only half of the angle

	float one_over_tan = 1.f / (std::tan(fov_radians));
	float f_minus_n = n_f_distances.y - n_f_distances.x;   //far - near
	
	mat4 proj;


	proj.columns[0].x = one_over_tan*inverse_aspect;

	//TODO:
	// minus in front of value since we work in rhs coordinate system:
	proj.columns[1].y = one_over_tan;

	//this block will adjust col 2
	proj.columns[2].z = -(n_f_distances.x + n_f_distances.y )   / f_minus_n;
	proj.columns[2].w = -1.f; //TODO -1 ?

	proj.columns[3].z = -(2.f * n_f_distances.y * n_f_distances.x)   / f_minus_n;
	proj.columns[3].w = 0.f; //bottom right must be zero in perspective proj!
	return proj;
}

////experimental one:
//mat4 mat4::persp_mat(float inverse_aspect, vec2 n_f_distances, float fov_deg) {
//	fov_deg = std::abs(fov_deg); //no negative fov plz!.
//	float fov_radians = fov_deg *0.0055555 * PI; //*0.0055555  is 1/180.f
//												 //we have just made our fov degrees into radians.
//	fov_radians *= 0.5f; //need only half of the angle
//
//	const float h = 1.0f / tan(fov_radians);
//	float neg_depth = n_f_distances.x - n_f_distances.y;
//
//	mat4 m;
//	m.columns[0].x = h *inverse_aspect;
//	m.columns[1].y = h;
//	m.columns[2].z = (n_f_distances.y + n_f_distances.x) / neg_depth;
//	m.columns[2].w = -1.0f;
//	m.columns[3].z = 2.0f*(n_f_distances.y * n_f_distances.x) / neg_depth;
//	m.columns[3].w = 0.0f;
//
//	return m;
//}




mat4 mat4::ortho_mat(vec2 left_right, vec2 bot_top, vec2 near_far){
	mat4 ortho;

	ortho.columns[0].x = 2 / (left_right.y-left_right.x) ;//right - left
	
	ortho.columns[1].y = 2 / (bot_top.y - bot_top.x); //top - bot

	ortho.columns[2].z = 2 / (near_far.y - near_far.x); //far - near


	ortho.columns[3].x = -(left_right.y+left_right.x) / (left_right.y-left_right.x);
	ortho.columns[3].y = -(bot_top.y+bot_top.x) / (bot_top.y-bot_top.x);
	ortho.columns[3].z = -(near_far.y + near_far.x) / (near_far.y - near_far.x);
	ortho.columns[3].w = 1.f; //just to make it clear.  (it would still be 1 from 
						   //from constructor, which sets matrix to identity anyway)
	return ortho;
}





void mat4::transpose() {
	vec4 row0 = copyRow(0); //we need those 4 intermediate vectors.
	vec4 row1 = copyRow(1); //Had we just started pluging rows instead of columns,
	vec4 row2 = copyRow(2); //we would mess up the later columns.
	vec4 row3 = copyRow(3);
	 
	//transpose this matrix:
	columns[0] = row0;
	columns[1] = row1;
	columns[2] = row2;
	columns[3] = row3;
}



vec3 mat4::get_translation() const {
	mat4 transpose =mat4::mat_via_columns(	copyColumn(0), copyColumn(1), 
											copyColumn(2), copyColumn(3)  );
	//get rid of the rotation coefficient from everywhere, including the
	//translation row. We revert all of the "rotation" garbage and get a
	//matrix which only has transform data remaining.
	//TODO check if this works. Cause it seems it doesnt (returns empty vec3)
	mat4 translation_only = transpose * (*this);
	return vec3(translation_only[12], translation_only[13], translation_only[14]);
}



mat4 mat4::inverse() const {

	double inv[16], det;

	//recall, operator[] works column major order.
	inv[0] = (*this)[5] * (*this)[10] * (*this)[15] -
		(*this)[5] * (*this)[11] * (*this)[14] -
		(*this)[9] * (*this)[6] * (*this)[15] +
		(*this)[9] * (*this)[7] * (*this)[14] +
		(*this)[13] * (*this)[6] * (*this)[11] -
		(*this)[13] * (*this)[7] * (*this)[10];

	inv[4] = -(*this)[4] * (*this)[10] * (*this)[15] +
		(*this)[4] * (*this)[11] * (*this)[14] +
		(*this)[8] * (*this)[6] * (*this)[15] -
		(*this)[8] * (*this)[7] * (*this)[14] -
		(*this)[12] * (*this)[6] * (*this)[11] +
		(*this)[12] * (*this)[7] * (*this)[10];

	inv[8] = (*this)[4] * (*this)[9] * (*this)[15] -
		(*this)[4] * (*this)[11] * (*this)[13] -
		(*this)[8] * (*this)[5] * (*this)[15] +
		(*this)[8] * (*this)[7] * (*this)[13] +
		(*this)[12] * (*this)[5] * (*this)[11] -
		(*this)[12] * (*this)[7] * (*this)[9];

	inv[12] = -(*this)[4] * (*this)[9] * (*this)[14] +
		(*this)[4] * (*this)[10] * (*this)[13] +
		(*this)[8] * (*this)[5] * (*this)[14] -
		(*this)[8] * (*this)[6] * (*this)[13] -
		(*this)[12] * (*this)[5] * (*this)[10] +
		(*this)[12] * (*this)[6] * (*this)[9];

	inv[1] = -(*this)[1] * (*this)[10] * (*this)[15] +
		(*this)[1] * (*this)[11] * (*this)[14] +
		(*this)[9] * (*this)[2] * (*this)[15] -
		(*this)[9] * (*this)[3] * (*this)[14] -
		(*this)[13] * (*this)[2] * (*this)[11] +
		(*this)[13] * (*this)[3] * (*this)[10];

	inv[5] = (*this)[0] * (*this)[10] * (*this)[15] -
		(*this)[0] * (*this)[11] * (*this)[14] -
		(*this)[8] * (*this)[2] * (*this)[15] +
		(*this)[8] * (*this)[3] * (*this)[14] +
		(*this)[12] * (*this)[2] * (*this)[11] -
		(*this)[12] * (*this)[3] * (*this)[10];

	inv[9] = -(*this)[0] * (*this)[9] * (*this)[15] +
		(*this)[0] * (*this)[11] * (*this)[13] +
		(*this)[8] * (*this)[1] * (*this)[15] -
		(*this)[8] * (*this)[3] * (*this)[13] -
		(*this)[12] * (*this)[1] * (*this)[11] +
		(*this)[12] * (*this)[3] * (*this)[9];

	inv[13] = (*this)[0] * (*this)[9] * (*this)[14] -
		(*this)[0] * (*this)[10] * (*this)[13] -
		(*this)[8] * (*this)[1] * (*this)[14] +
		(*this)[8] * (*this)[2] * (*this)[13] +
		(*this)[12] * (*this)[1] * (*this)[10] -
		(*this)[12] * (*this)[2] * (*this)[9];

	inv[2] = (*this)[1] * (*this)[6] * (*this)[15] -
		(*this)[1] * (*this)[7] * (*this)[14] -
		(*this)[5] * (*this)[2] * (*this)[15] +
		(*this)[5] * (*this)[3] * (*this)[14] +
		(*this)[13] * (*this)[2] * (*this)[7] -
		(*this)[13] * (*this)[3] * (*this)[6];

	inv[6] = -(*this)[0] * (*this)[6] * (*this)[15] +
		(*this)[0] * (*this)[7] * (*this)[14] +
		(*this)[4] * (*this)[2] * (*this)[15] -
		(*this)[4] * (*this)[3] * (*this)[14] -
		(*this)[12] * (*this)[2] * (*this)[7] +
		(*this)[12] * (*this)[3] * (*this)[6];

	inv[10] = (*this)[0] * (*this)[5] * (*this)[15] -
		(*this)[0] * (*this)[7] * (*this)[13] -
		(*this)[4] * (*this)[1] * (*this)[15] +
		(*this)[4] * (*this)[3] * (*this)[13] +
		(*this)[12] * (*this)[1] * (*this)[7] -
		(*this)[12] * (*this)[3] * (*this)[5];

	inv[14] = -(*this)[0] * (*this)[5] * (*this)[14] +
		(*this)[0] * (*this)[6] * (*this)[13] +
		(*this)[4] * (*this)[1] * (*this)[14] -
		(*this)[4] * (*this)[2] * (*this)[13] -
		(*this)[12] * (*this)[1] * (*this)[6] +
		(*this)[12] * (*this)[2] * (*this)[5];

	inv[3] = -(*this)[1] * (*this)[6] * (*this)[11] +
		(*this)[1] * (*this)[7] * (*this)[10] +
		(*this)[5] * (*this)[2] * (*this)[11] -
		(*this)[5] * (*this)[3] * (*this)[10] -
		(*this)[9] * (*this)[2] * (*this)[7] +
		(*this)[9] * (*this)[3] * (*this)[6];

	inv[7] = (*this)[0] * (*this)[6] * (*this)[11] -
		(*this)[0] * (*this)[7] * (*this)[10] -
		(*this)[4] * (*this)[2] * (*this)[11] +
		(*this)[4] * (*this)[3] * (*this)[10] +
		(*this)[8] * (*this)[2] * (*this)[7] -
		(*this)[8] * (*this)[3] * (*this)[6];

	inv[11] = -(*this)[0] * (*this)[5] * (*this)[11] +
		(*this)[0] * (*this)[7] * (*this)[9] +
		(*this)[4] * (*this)[1] * (*this)[11] -
		(*this)[4] * (*this)[3] * (*this)[9] -
		(*this)[8] * (*this)[1] * (*this)[7] +
		(*this)[8] * (*this)[3] * (*this)[5];

	inv[15] = (*this)[0] * (*this)[5] * (*this)[10] -
		(*this)[0] * (*this)[6] * (*this)[9] -
		(*this)[4] * (*this)[1] * (*this)[10] +
		(*this)[4] * (*this)[2] * (*this)[9] +
		(*this)[8] * (*this)[1] * (*this)[6] -
		(*this)[8] * (*this)[2] * (*this)[5];

	det = (*this)[0] * inv[0] + (*this)[1] * inv[4] + (*this)[2] * inv[8] + (*this)[3] * inv[12];

	if (det == 0)
		return mat4(); //identity, sine there is no inverse

	det = 1.0 / det;

	for (int i = 0; i < 16; ++i) {
		inv[i] *= det;
	}

	mat4 invOut = mat4::mat_via_columns(vec4(inv[0],  inv[1],  inv[2],  inv[3]),
										vec4(inv[4],  inv[5],  inv[6],  inv[7]),
										vec4(inv[8],  inv[9],  inv[10], inv[11]),
										vec4(inv[12], inv[13], inv[14], inv[15]));
	return invOut; 
}



mat3 mat4::_mat3() const {
	return mat3::mat_via_columns( columns[0]._vec3(),
								  columns[1]._vec3(),
								  columns[2]._vec3()  );
}


