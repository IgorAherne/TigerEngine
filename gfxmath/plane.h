#pragma once
#include "vec3.h"
#include "vec2.h"
#include "bshape.h"

class aabb_bshape;
class sphere_bshape;




class plane : public bshape{

public:
	//constructs a plane which passes through the origin and faces upwards
	plane() : bshape(vec3()),normal(0.f, 1.f, 0.f)  {};

	//constructs a plane which passes through the given point and has the provided
	//direction
	plane(vec3 point_on_plane, vec3 normal);

	//constructs the plane. Normal points towards the viewer, if the viewer saw
	//the points in clockwise order.
	plane(vec3 coplanar_point1, vec3 coplanar_point2, vec3 coplanar_point3);
	
	~plane() {};

	//undetermined for a plane, since it's infinite, 
	//there is no min/maximum along any direction.
	vec2 project_on_normal_world(vec3 unit_length_dir_world) const override {
		return vec2(0, 0); //undetermined for a plane, since it's infinite, 
	}				//there is no min/maximum along any direction.


	void recompute(const std::vector<vec3> &vert_coords_local)override{
		//TODO
	}


	//give the general direction in which the plane should be oriented.
	//The function will normalize it.
	void set_NormalDirection(vec3 non_unit_length_normal) {
		normal = non_unit_length_normal.get_normalized();
	}

	//give the UNIT length normal which will be used to orient the plane.
	void set_UnitLengthNormal(vec3 unit_length_normal) {
		normal = unit_length_normal; //assume the user provided unit length normal
	}


	//returns a unit-length vector which defines the orientation of the plane
	inline vec3 getNormal() {
		return normal;
	}
	


private:
	vec3 normal; //must be unit length.
};

