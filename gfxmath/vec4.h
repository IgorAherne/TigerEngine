#pragma once
#include <cmath>

struct vec3; //carefull, don't use any functionality of vec3 in this header.

struct vec4 {
	float x, y, z, w;

	vec4() :x(0), y(0), z(0), w(0) {}

	vec4(float x, float y, float z, float w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	vec4(vec3 v3, float value = 0.f);
	vec4(float value,  vec3 v3);

	~vec4(){};


	//all components summed
	inline float Sum() const {
		return x + y + z + w;
	}

	//dot product
	static float Dot( vec4 lhs,  vec4 rhs) {
		return (lhs*rhs).Sum();
	}

	//get the true length of vector in terms of units.
	//more expensive than sqr_length()
	inline float length() const {
		return std::sqrt(x*x + y*y + z*z + w*w);
	}

	//return the squared length. Fast and allows comparisons of vectors by length. 
	//To get real length, take the square root of it.
	inline float sqr_length() const {
		return x*x + y*y + z*z + w*w;
	}

	//make this vector into a unit length vector.
	inline void normalize() {
		float len = this->length();

		if (len > 0)
			(*this) /= len;
	}

	//create a unit-length vector, which is in the same direction as this one.
	inline vec4 get_normalized() const {
		float len = this->length();

		if (len > 0)
			return (*this) / len;
		else
			return vec4();
	}



	//make a new vec3 based on this vec4
	vec3 _vec3() const;

	inline vec4 operator+( vec4 rhs) const {
		return vec4(x+rhs.x, y+rhs.y, z+rhs.z, w+rhs.w);
	}
	inline vec4 operator-( vec4 rhs) const {
		return vec4(x-rhs.x, y-rhs.y, z-rhs.z, w-rhs.w);
	}
	inline vec4 operator-() const {
		return vec4(-x, -y, -z, -w);
	}
	inline vec4 operator*( vec4 rhs) const {
		return vec4(x*rhs.x, y*rhs.y, z*rhs.z, w*rhs.w);
	}
	inline vec4 operator/( vec4 rhs) const {
		return vec4(x/rhs.x, y/rhs.y, z/rhs.z, w/rhs.w);
	}
	inline vec4 &operator/=( vec4 rhs) {
		x /= rhs.x;
		y /= rhs.y;
		z /= rhs.z;
		w /= rhs.w;
		return *this;
	}
	inline vec4 &operator*=( vec4 rhs) {
		x *= rhs.x;
		y *= rhs.y;
		z *= rhs.z;
		w *= rhs.w;
		return *this;
	}
	inline vec4 &operator/=(float rhs) {
		x /= rhs;
		y /= rhs;
		z /= rhs;
		w /= rhs;
		return *this;
	}
	inline vec4 &operator*=(float rhs) {
		x *= rhs;
		y *= rhs;
		z *= rhs;
		w *= rhs;
		return *this;
	}
	inline vec4 operator/(float rhs) const {
		return vec4(x/rhs, y/rhs, z/rhs, w/rhs);
	}

	inline vec4 operator*(float rhs) const {
		return vec4(x*rhs, y*rhs, z*rhs, w*rhs);
	}
	inline vec4 &operator++() {
		x += 1.0f;
		y += 1.0f;
		z += 1.0f;
		w += 1.0f;
		return *this;
	}
	inline vec4 &operator++(int) {
		x += 1.0f;
		y += 1.0f;
		z += 1.0f;
		w += 1.0f;
		return *this;
	}
	inline vec4 &operator--() {
		x -= 1.0f;
		y -= 1.0f;
		z -= 1.0f;
		w -= 1.0f;
		return *this;
	}
	inline vec4 &operator--(int) {
		x -= 1.0f;
		y -= 1.0f;
		z -= 1.0f;
		w -= 1.0f;
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
			case 3: {
				return w;
			}
		}//end switch
	}

	//linearly interpolate from start to end by the given percentage
	static vec4 Lerp( vec4 start,  vec4 end, float percent) {
		return start*percent + end*percent;
	}
};