#pragma once
#include "vec4.h"
#include "vec3.h"
#include "vec2.h"
#include "mat3.h" //make sure to not use any functionality of it. Only as return type

//matrices are stored via columns, following a traditional mathematical
//convension.


struct mat4 {
public:
	

	//get 0th, 1st, 2nd or 3rd column. 
	inline vec4 copyColumn(size_t col_num) const {
		return columns[col_num];
	}

	//get 0th, 1st, 2nd or 3rd row.
	const vec4 copyRow(size_t row_num) const {
		//access each column, get corresponding component.
		return vec4(columns[0][row_num],
					columns[1][row_num],
					columns[2][row_num],
					columns[3][row_num]);
	}

	inline void setColumn(size_t col, vec4 values) {
		columns[col] = values;
	}

	inline void setRow(size_t row, vec4 values) {
		switch (row) {
				case 0: {
					//access 0th column, assign appropriate component
					columns[0].x = values.x;
					columns[1].x = values.y;
					columns[2].x = values.z;
					columns[3].x = values.w;
				}break;
				case 1: {
					columns[0].y = values.x;
					columns[1].y = values.y;
					columns[2].y = values.z;
					columns[3].y = values.w;
				}break;
				case 2: {
					columns[0].z = values.x;
					columns[1].z = values.y;
					columns[2].z = values.z;
					columns[3].z = values.w;
				}break;
				case 3: {
					columns[0].w = values.x;
					columns[1].w = values.y;
					columns[2].w = values.z;
					columns[3].w = values.w;
				}break;
		}
	}


	//creates an identity matrix
	mat4() {
		columns[0] = vec4(1,0,0,0);
		columns[1] = vec4(0,1,0,0);
		columns[2] = vec4(0,0,1,0);
		columns[3] = vec4(0,0,0,1);
	}

	//tiny bit faster than  mat4::mat_via_rows()
	static inline mat4 mat_via_columns( vec4 column0,  vec4 column1,  
										vec4 column2,  vec4 column3){
		mat4 out_mat;
		out_mat.columns[0] = column0;
		out_mat.columns[1] = column1;
		out_mat.columns[2] = column2;
		out_mat.columns[3] = column3;
		return out_mat;
	}


	~mat4() {}

	//create a matrix using rows
	static inline mat4 mat_via_rows(vec4 row_0, vec4 row_1, vec4 row_2, vec4 row_3){
		mat4 out_mat;
		out_mat.setRow(0, row_0);
		out_mat.setRow(1, row_1);
		out_mat.setRow(2, row_2);
		out_mat.setRow(3, row_3);
		return out_mat;
	}


	//row-by-vector multiplication.
	vec4 operator*( vec4 rhs) const {
		return vec4(vec4::Dot(copyRow(0), rhs),
					vec4::Dot(copyRow(1), rhs),
					vec4::Dot(copyRow(2), rhs),
					vec4::Dot(copyRow(3), rhs));
	}

	//matrix * matrix.  Row-by-Column multiplication, left matrix  by  right matrix
	mat4 operator*( mat4 rhs) const {
		

		vec4 row_0(	vec4::Dot(copyRow(0), rhs.columns[0]),
					vec4::Dot(copyRow(0), rhs.columns[1]),
					vec4::Dot(copyRow(0), rhs.columns[2]),
					vec4::Dot(copyRow(0), rhs.columns[3])  );

		vec4 row_1(	vec4::Dot(copyRow(1), rhs.columns[0]),
					vec4::Dot(copyRow(1), rhs.columns[1]),
					vec4::Dot(copyRow(1), rhs.columns[2]),
					vec4::Dot(copyRow(1), rhs.columns[3])  );

		vec4 row_2(	vec4::Dot(copyRow(2), rhs.columns[0]),
					vec4::Dot(copyRow(2), rhs.columns[1]),
					vec4::Dot(copyRow(2), rhs.columns[2]),
					vec4::Dot(copyRow(2), rhs.columns[3])  );

		vec4 row_3(	vec4::Dot(copyRow(3), rhs.columns[0]),
					vec4::Dot(copyRow(3), rhs.columns[1]),
					vec4::Dot(copyRow(3), rhs.columns[2]),
					vec4::Dot(copyRow(3), rhs.columns[3])  );
		//avoided forloops for readability
		return mat4::mat_via_rows(row_0, row_1, row_2, row_3);
	}


	//copy the value of one of sixteen entries (COLUMN major order)
	float operator[](size_t ix) const {
		//u take responsibility that ix is  0-15 range
		switch (ix) {
				case 0: {
					return columns[0][0];
				}break;
				case 1: {
					return columns[0][1];
				}break;
				case 2: {
					return columns[0][2];
				}break;
				case 3: { 
					return columns[0][3];
				}break;
				case 4: { 
					return columns[1][0];
				}break;
				case 5: {
					return columns[1][1];
				}break;
				case 6: { 
					return columns[1][2];
				}break;
				case 7: { 
					return columns[1][3];
				}break;
				case 8: { 
					return columns[2][0];
				}break;
				case 9: { 
					return columns[2][1];
				}break;
				case 10: { 
					return columns[2][2];
				}break;
				case 11: {
					return columns[2][3];
				}
				case 12: { 
					return columns[3][0];
				}break;
				case 13: { 
					return columns[3][1];
				}break;
				case 14: { 
					return columns[3][2];
				}break;
				case 15: {
					return columns[3][3];
				}break;
		} //no point in modulo operators and shorter switch. It gives up performance
		return 0;
	}//end operator[]



	//copy a VALUE of one of sixteen entries, in the column and row (col MAJOR order)
	float copyValue(size_t col_ix, size_t row_ix) const {
		return (columns[col_ix] )[row_ix];
	}



	//multiply each of the sixteen entries by a coefficient
	mat4 &operator*=(float rhs) {
		for (int r = 0; r < 4; ++r) {
			columns[r] *= rhs;
		}
		return *this;
	}

	//divide each of the sixteen entries by a float
	mat4 &operator/=(float rhs) {
		for (int r = 0; r < 4; ++r) {
			columns[r] /= rhs;
		}
		return *this;
	}



	//rotation in terms of degrees arround x y or z
	static mat4 euler_rot(float x_deg, float y_deg, float z_deg);

	//make a matrix to rotate anythign arround the axis.  
	static mat4 angleAxis_rot( vec3 axis, float deg);

	//given the current position and rotation of the viewer, construct a view
	//matrix. It can then be used to orent the objects in relation to this viewer.
	//Vectors must be in the world space
	static mat4 viewMatrix( vec3 curr_pos_world,  vec3 pitch_yaw_roll_world);

	static mat4 viewMatrix(const mat4 transf);

	//creates a perspective Projection Matrix.
	//inverse_aspect = ratio of HEIGHT/width
	//near clipping plane and far clipping plane distances,
	//vertical field of vision in degrees (50.f is typical, but experiment if needed)
	static mat4 persp_mat(float inverse_aspect, vec2 near_far_dist, float fov_deg);

	//creates an orthogonal Projection Matrix.
	//supply the distance to left and right planes (relative to zero), 
	//same for bottom, top   and   near, far.
	static mat4 ortho_mat(vec2 left_right, vec2 bot_top, vec2 near_far);



	inline static mat4 identity() {
		return mat4::mat_via_columns(vec4(1.f, 0.f, 0.f, 0.f), //column 1
									 vec4(0.f, 1.f, 0.f, 0.f), //column 2
									 vec4(0.f, 0.f, 1.f, 0.f),
									 vec4(0.f, 0.f, 0.f, 1.f) );
	}


	//transpose this matrix (alters this matrix)
	void transpose();

	//inverse of this matrix (expensive, about 400 multiplications-worth)
	mat4 inverse() const;

	//get the scale diagonal of this matrix (Might be pollutted with rotation)
	inline vec3 scale_diagonal() const {
		return vec3(columns[0].x, columns[1].y, columns[2].z);
	}

	//extract a "forward", unit-length vector
	inline vec3 get_fwd_dir() const {
		return vec3(copyRow(2)._vec3());
	}

	//extract an "upwards" unit-length vector
	inline vec3 get_up_dir() const {
		return vec3(copyRow(1)._vec3());
	}

	//extract a "rightwards" unit-length vector
	inline vec3 get_right_dir() const {
		return vec3(copyRow(0)._vec3());
	}


	//extract a "translation" vector
	vec3 get_translation() const;


	//check if this matrix is perspective. 
	//Returns false otherwise (meaning its an orthographic one)
	inline bool is_perspective_proj() {
		//perspective projection matrix's bottom-right value is 0
		return columns[3].w == 0;

		//orthographic one would be 1
	}

	//check if this matrix is orthographic. 
	//Returns false otherwise (meaning its a perspective one)
	inline bool is_orthographic_proj() {
		//orthographic projection matrix's bottom-right value is 1
		return columns[3].w == 1;
	}

	static mat4 translation(vec3 displacement) {
		mat4 out; //identity
		out.columns[3] = vec4(displacement, 1.0);
		return out;
	}

	static mat4 scale(vec3 scale) {
		mat4 out;
		out.columns[0].x *= scale.x;
		out.columns[1].y *= scale.y;
		out.columns[2].z *= scale.z;
		return out;
	}

	//reset all values to make this matrix an identity one
	inline void toIdentity() {
		columns[0] = vec4(1.f, 0.f, 0.f, 0.f);
		columns[1] = vec4(0.f, 1.f, 0.f, 0.f);
		columns[2] = vec4(0.f, 0.f, 1.f, 0.f);
		columns[3] = vec4(0.f, 0.f, 0.f, 1.f);
	}

	//convert this matrix into simpler one, the mat3.
	mat3 _mat3() const;


private:
	vec4 columns[4]; //mat4 has 4 columns 
};