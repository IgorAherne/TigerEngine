#include "mat3.h"
#include "mat4.h"

//OBSOLETE, use mat4 instead

// multiply mat3 by mat4.
mat4 mat3::operator*( mat4 rhs) const {
	vec3 row_0(	 vec3::Dot(copyRow(0), rhs.copyColumn(0)._vec3() ),
				 vec3::Dot(copyRow(0), rhs.copyColumn(1)._vec3() ),
				 vec3::Dot(copyRow(0), rhs.copyColumn(2)._vec3() )  );
				 					
	vec3 row_1(	 vec3::Dot(copyRow(1), rhs.copyColumn(0)._vec3() ),
				 vec3::Dot(copyRow(1), rhs.copyColumn(1)._vec3() ),
				 vec3::Dot(copyRow(1), rhs.copyColumn(2)._vec3() )  );
				 					
	vec3 row_2(	 vec3::Dot(copyRow(2), rhs.copyColumn(0)._vec3() ),
				 vec3::Dot(copyRow(2), rhs.copyColumn(1)._vec3() ),
				 vec3::Dot(copyRow(2), rhs.copyColumn(2)._vec3() )  );
	
	//avoided forloops for readability

	//spit out matrix 4 (last column and last row are untouched
	//by multiplication)
	return mat4::mat_via_rows( row_0._vec4(rhs.copyRow(0).w), //as you see we cast
							   row_1._vec4(rhs.copyRow(1).w), //into vec4. We snatch
							   row_2._vec4(rhs.copyRow(2).w), //w values from the 
							   rhs.copyRow(3) );			  //mat4 we worked with.
}




void mat3::transpose() {
	vec3 row0 = copyRow(0); //we need those 4 intermediate vectors.
	vec3 row1 = copyRow(1);//Had we just started pluging rows instead of columns,
	vec3 row2 = copyRow(2);//we would mess up the later columns.

	//transpose this matrix:
	columns[0] = row0;
	columns[1] = row1;
	columns[2] = row2;
}



mat4 mat3::_mat4(vec4 col_3/* = vec4(0, 0, 0, 1)*/) {
	return mat4::mat_via_columns (vec4(copyColumn(0), 0), //0st col
								  vec4(copyColumn(1), 0), //1st column
								  vec4(copyColumn(2), 0), //2nd column
								  col_3 ); //3rd column
}