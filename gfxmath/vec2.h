#pragma once
#include <cmath>

//carefull, don't use any functionality of vec3.h or vec4.h in this header.
struct vec3;//only use as return types or arguments. 
struct vec4;

struct vec2 {
	float x, y;




	vec2() :x(0), y(0) {}

	vec2(float x, float y) {
		this->x = x;
		this->y = y;
	}

	~vec2() {};

	//all components summed
	inline float Sum() const {
		return x + y;
	}

	//dot product
	static float Dot( vec2 lhs,  vec2 rhs) {
		return (lhs*rhs).Sum();
	}

	//get the true length of vector in terms of units.
	//more expensive than sqr_length()
	inline float length() const {
		return std::sqrt(x*x + y*y);
	}

	//return the squared length. Fast and allows comparisons of vectors by length. 
	//To get real length, take the square root of it.
	inline float sqr_length() const {
		return x*x + y*y;
	}

	//make this vector into a unit length vector.
	inline void normalize() {
		float len = this->length();

		if (len > 0)
			(*this) /= len;
	}

	//create a unit-length vector, which is in the same direction as this one.
	inline vec2 get_normalized() const {
		float len = this->length();

		if (len > 0)
			return (*this) / len;
		else
			return vec2();
	}


	//make a new vec3 based on this vec2
	vec3 _vec3(float z = 0.0f) const;

	//make a new vec4 based on this vec2
	vec4 _vec4(float z = 0.0f, float w = 0.0f) const;

	inline vec2 operator+( vec2 rhs) const {
		return vec2(x+rhs.x, y+rhs.y);
	}
	inline vec2 operator-( vec2 rhs) const {
		return vec2(x-rhs.x, y-rhs.y);
	}
	inline vec2 operator*( vec2 rhs) const {
		return vec2(x*rhs.x, y*rhs.y);
	}
	inline vec2 operator/( vec2 rhs) const {
		return vec2(x/rhs.x, y/rhs.y);
	}
	inline vec2 &operator/=( vec2 rhs) {
		x /= rhs.x;
		y /= rhs.y;
		return *this;
	}
	inline vec2 &operator*=( vec2 rhs) {
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}
	inline vec2 &operator/=(float rhs) {
		x /= rhs;
		y /= rhs;
		return *this;
	}
	inline vec2 &operator*=(float rhs) {
		x *= rhs;
		y *= rhs;
		return *this;
	}
	inline vec2 operator/(float rhs) const {
		return vec2(x/rhs, y/rhs);
	}

	inline vec2 operator*(float rhs) const {
		return vec2(x*rhs, y*rhs);
	}
	inline vec2 &operator++() {
		x += 1.0f;
		y += 1.0f;
		return *this;
	}
	inline vec2 &operator++(int) {
		x += 1.0f;
		y += 1.0f;
		return *this;
	}
	inline void operator+=(vec2 rhs) {
		x += rhs.x;
		y += rhs.y;
	}
	inline void operator-=(vec2 rhs) {
		x -= rhs.x;
		y -= rhs.y;
	}
	inline vec2 &operator--() {
		x -= 1.0f;
		y -= 1.0f;
		return *this;
	}
	inline vec2 &operator--(int) {
		x -= 1.0f;
		y -= 1.0f;
		return *this;
	}
	inline bool operator!=(vec2 rhs) { //return true if at least  X or Y components
		return x != rhs.x   ||   y != rhs.y; //don't match.
	}
	inline bool operator==(vec2 rhs) { //return true if ALL components equal to the
		return x == rhs.x   &&   y == rhs.y; //corresponding components of the rhs.
	}

	inline float operator[](size_t ix) const {
		ix %= 2; //loop arround 2

		switch (ix) {
			case 0: {
				return x;
			}break;
			case 1: {
				return y;
			}break;
		}//end switch
	}

	//linearly interpolate from start to end by the given percentage
	static vec2 Lerp( vec2 start,  vec2 end, float percent) {
		return start*percent + end*percent;
	}
};