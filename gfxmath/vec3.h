#pragma once
#include <cmath>

struct vec4;//carefull, don't use any functionality of vec2 or vec4 in this header.
struct vec2;//only use as return type or arguments here.

struct vec3 {
	float x, y, z;
	
	vec3() :x(0), y(0), z(0) {}
	vec3(float val) :x(val), y(val), z(val){}

	vec3(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	~vec3(){};

	//all components summed
	inline float Sum() const {
		return x + y + z;
	}

	//dot product
	static float Dot( vec3 lhs,  vec3 rhs) {
		return (lhs*rhs).Sum();
	}

	//produces vector perpendicular to the given two. Argument order matters.
	//If the second vector is rotated counter clockwize from 1st one, result will
	//point upwards, towards the viewer. Otherwise, points in the opposite direction.
	static vec3 Cross( vec3 vector1,  vec3 ccw_vector) {
		return vec3( (vector1.y*ccw_vector.z - vector1.z*ccw_vector.y),
					-(vector1.x*ccw_vector.z - vector1.z*ccw_vector.x),
					 (vector1.x*ccw_vector.y - vector1.y*ccw_vector.x)  );
	}

	//get the true length of vector in terms of units.
	//more expensive than sqr_length()
	inline float length() const {
		return std::sqrt(x*x + y*y + z*z);
	}

	//return the squared length. Fast and allows comparisons of vectors by length. 
	//To get real length, take the square root of it.
	inline float sqr_length() {
		return x*x + y*y + z*z;
	}

	//make this vector into a unit length vector.
	inline void normalize()  {
		float len = this->length();

		if(len > 0)
			(*this) /= len;
	}

	//create a unit-length vector, which is in the same direction as this one.
	inline vec3 get_normalized() const {
		float len = this->length();

		if (len > 0)
			return (*this) / len;
		else
			return vec3();
	}


	//make a new vec2 based on this vec3
	vec2 _vec2() const;
	//make a new vec4 based on this vec3
	vec4 _vec4(float w = 0.0f) const;


	inline vec3 operator+( vec3 rhs) const {
		return vec3(x + rhs.x, y + rhs.y, z + rhs.z);
	}
	inline vec3 operator-( vec3 rhs) const {
		return vec3(x - rhs.x, y - rhs.y, z - rhs.z);
	}
	inline vec3 operator-() const {
		return vec3(-x, -y, -z);
	}
	inline vec3 operator*( vec3 rhs) const {
		return vec3(x*rhs.x, y*rhs.y, z*rhs.z);
	}
	inline vec3 operator/( vec3 rhs) const {
		return vec3(x/rhs.x, y/rhs.y, z/rhs.z);
	}
	inline vec3 &operator/=( vec3 rhs) {
		x /= rhs.x;
		y /= rhs.y;
		z /= rhs.z;
		return *this;
	}
	inline vec3 &operator*=( vec3 rhs) {
		x *= rhs.x;
		y *= rhs.y;
		z *= rhs.z;
		return *this;
	}
	inline vec3 &operator/=(float rhs) {
		x /= rhs;
		y /= rhs;
		z /= rhs;
		return *this;
	}
	inline vec3 &operator*=(float rhs) {
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}
	inline vec3 operator/(float rhs) const {
		return vec3(x/rhs, y/rhs, z/rhs);
	}

	inline vec3 operator*(float rhs) const {
		return vec3(x*rhs, y*rhs, z*rhs);
	}

	inline vec3 &operator++() {
		x += 1.0f;
		y += 1.0f;
		z += 1.0f;
		return *this;
	}
	inline void operator+=(vec3 rhs) {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
	}
	inline void operator-=(vec3 rhs) {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
	}
	inline vec3 &operator++(int) {
		x += 1.0f;
		y += 1.0f;
		z += 1.0f;
		return *this;
	}
	inline vec3 &operator--() {
		x -= 1.0f;
		y -= 1.0f;
		z -= 1.0f;
		return *this;
	}
	inline vec3 &operator--(int) {
		x -= 1.0f;
		y -= 1.0f;
		z -= 1.0f;
		return *this;
	}

	inline float operator[](size_t ix) const {
		switch (ix) {
			case 0: {
				return x;
			}break;
			case 1: {
				return y;
			}break;
			case 2: {
				return z;
			}break;
		}//end switch
	}


	//linearly interpolate from start to end by the given percentage
	static vec3 Lerp( vec3 start,  vec3 end, float percent) {
		return start*percent + end*percent;
	}
};