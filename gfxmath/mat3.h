#pragma once
#include "vec3.h"
#include "vec4.h"


struct mat4; //carefull, don't use any functionality of mat4 in this header.


struct mat3 {


public:

	//get 0th, 1st or 2nd column. 
	vec3 copyColumn(size_t col_num) const {
		return vec3(columns[col_num].x, columns[col_num].y, columns[col_num].z);
	}


	//get 0th, 1st or 2nd row. 
	const vec3 copyRow(size_t row_num) const {
		//access each column, get corresponding component.
		return vec3(columns[0][row_num],
					columns[1][row_num],
					columns[2][row_num]);
	}


	inline void setColumn(size_t col, vec3 values) {
		columns[col] = values;
	}


	inline void setRow(size_t row, vec3 values) {
		switch (row) {
				case 0: {
					//access 0th column, assign appropriate component
					columns[0].x = values.x;
					columns[1].x = values.y;
					columns[2].x = values.z;
				}break;
				case 1: {
					columns[0].y = values.x;
					columns[1].y = values.y;
					columns[2].y = values.z;
				}break;
				case 2: {
					columns[0].z = values.x;
					columns[1].z = values.y;
					columns[2].z = values.z;
				}break;
		}
	}


	//creates an identity matrix
	mat3() {
		columns[0] = vec3(1, 0, 0);
		columns[1] = vec3(0, 1, 0);
		columns[2] = vec3(0, 0, 1);
	}


	//tiny bit faster than  mat4::mat_via_rows()
	static inline mat3 mat_via_columns( vec3 column0, vec3 column1, vec3 column2){
		mat3 out_mat;
		out_mat.columns[0] = column0;
		out_mat.columns[1] = column1;
		out_mat.columns[2] = column2;
		return out_mat;
	}


	//create a matrix using rows
	static inline mat3 mat_via_rows(vec3 row_0, vec3 row_1, vec3 row_2) {
		mat3 out_mat;
		out_mat.setRow(0, row_0);
		out_mat.setRow(1, row_1);
		out_mat.setRow(2, row_2);
		return out_mat;
	}


	~mat3(){}

	
	//row-by-vector multiplication.
	vec3 operator*( vec3 rhs) const {
		return vec3( vec3::Dot(copyRow(0), rhs),
					 vec3::Dot(copyRow(1), rhs),
					 vec3::Dot(copyRow(2), rhs) );
	}


	//matrix * matrix.  Row-by-Column multiplication, left-to-right
	mat3 operator*( mat3 rhs) const { 
		vec3 row_0( vec3::Dot(copyRow(0), rhs.columns[0]),
					vec3::Dot(copyRow(0), rhs.columns[1]),
					vec3::Dot(copyRow(0), rhs.columns[2])  );

		vec3 row_1( vec3::Dot(copyRow(1), rhs.columns[0]),
					vec3::Dot(copyRow(1), rhs.columns[1]),
					vec3::Dot(copyRow(1), rhs.columns[2])  );

		vec3 row_2( vec3::Dot(copyRow(2), rhs.columns[0]),
					vec3::Dot(copyRow(2), rhs.columns[1]),
					vec3::Dot(copyRow(2), rhs.columns[2])  );
		//avoided forloops for readability
		return mat3::mat_via_rows(row_0, row_1, row_2);
	}


	//allows to multiply matrix3 by matrix4.
	mat4 operator*( mat4 rhs) const;

	


	//get one of nine entries (COLUMN MAJOR order)
	float operator[](size_t ix) const {

		switch (ix) {
					case 0: { 
						columns[0].x; //access 0th row, query appropriate component.
					}break;
					case 1: {
						columns[0].y;
					}break;
					case 2: {
						columns[0].z;
					}break;
					case 3: {
						columns[1].x; //access 0th row, query appropriate component.
					}break;
					case 4: {
						columns[1].y;
					}break;
					case 5: {
						columns[1].z;
					}
					case 6: {
						columns[2].x; //access 0th row, query appropriate component.
					}break;
					case 7: {
						columns[2].y;
					}break;
					case 8: {
						columns[2].z;
					}break;
		}
	}//end operator[]



	 //multiply each of the nine entries by a coefficient
	mat3 &operator*=(float rhs) {
		for (int r = 0; r < 3; ++r) {
			columns[r] *= rhs;
		}
		return *this;
	}

	//divide each of the nine entries by a float
	mat3 &operator/=(float rhs) {
		for (int r = 0; r < 3; ++r) {
			columns[r] /= rhs;
		}
		return *this;
	}
	

	//rotation in terms of degrees arround x y or z
	static mat3 euler_rot(float x_deg, float y_deg, float z_deg);
	
	//convert to matrix4
	mat4 _mat4(vec4 col_3 = vec4(0,0,0,1));


	//transpose this matrix 
	void transpose();

	//inverse of this matrix (expensive)
	mat3 inverse();

	//extract a "forward", unit-length vector
	inline vec3 get_fwd_dir() { return copyRow(2); }

	//extract an "upwards" unit-length vector
	inline vec3 get_up_dir() { return copyRow(1); }

	//extract a "rightwards" unit-length vector
	inline vec3 get_right_dir() { return copyRow(0); }

	//get the scale contained by this matrix (Might be pollutted with rotation)
	inline vec3 scale_diagonal() { return vec3((*this)[0], (*this)[4], (*this)[9]); }


	//reset all values to make this matrix an identity one
	inline void toIdentity() {
		columns[0] = vec3(1.f, 0.f, 0.f); //column 1
		columns[1] = vec3(0.f, 1.f, 0.f); //column 2
		columns[2] = vec3(0.f, 0.f, 1.f);
	}



	private:
		vec3 columns[3]; //mat3 is COLUMN major
};