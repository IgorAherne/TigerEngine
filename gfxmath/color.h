#pragma once
#include "vec4.h"


struct color {
	color() : r(0), g(0), b(0), a(0) {}

	//create colors using floats, which are in the 0.0 - 1.0 range
	//Internally, color components are stored as unsigned bytes
	color(float r, float g, float b, float a){
		this->r = static_cast<unsigned char>(255.f*r);
		this->g = static_cast<unsigned char>(255.f*g);
		this->b = static_cast<unsigned char>(255.f*b);
		this->a = static_cast<unsigned char>(255.f*a);
	}

	//create colors via unsigned bytes (whole numbers, in the 0 - 255 range)
	//Internally, color components are always stored as unsigned bytes
	color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	inline color operator+(const color &rhs) {
		//use byte-clamper function to make it clear which of the
		//constructors to use, and to prevent byte value from looping back to zero.
		return color( clmp(r + rhs.r),
					  clmp(g + rhs.g),
					  clmp(b + rhs.b),
					  clmp(a + rhs.a)  );
	}
	inline color operator*(float rhs) {
		//use byte-clamper function to make it clear which of the
		//constructors to use, and to prevent byte value from looping back to zero.
		return color(clmp((float)r*rhs), 
					 clmp((float)g*rhs), 
					 clmp((float)b*rhs),
					 clmp((float)a*rhs)  );
	}
	inline color operator/(float rhs) {
		//use byte-clamper function to make it clear which of the
		//constructors to use, and to prevent byte value from looping back to zero.
		return color(clmp((float)r/rhs),
					 clmp((float)g/rhs),
					 clmp((float)b/rhs),
					 clmp((float)a/rhs)  );
	}
	inline color operator/=(float rhs) {
		return (*this)/rhs;
	}
	inline color operator*=(float rhs) {
		return (*this)*rhs;
	}


	
	//returns r component as float in the 0.0 - 1.0 range
	inline float get_float_r() { return (float)r / 255; }
	//returns g component as float in the 0.0 - 1.0 range
	inline float get_float_g() { return (float)g / 255; }
	//returns b component as float in the 0.0 - 1.0 range
	inline float get_float_b() { return (float)b / 255; }
	//returns a component as float in the 0.0 - 1.0 range
	inline float get_float_a() { return (float)a / 255; }

	//return color as a vector4 (usefull when color has to be 
	//passed to opengl uniform).
	inline vec4 get_color_vec4(){ 
		return vec4(get_float_r(), get_float_g(), get_float_b(), get_float_a());
	}

	//return color as a vector3, throwing out w-component 
	//(usefull when color has to be passed to opengl uniform).
	inline vec3 get_color_vec3() {
		return vec3(get_float_r(), get_float_g(), get_float_b());
	}

	unsigned char r;
	unsigned char g; //the color struct is 4 byte long. No allignment issues
	unsigned char b;
	unsigned char a;

private:
	//returns a clamped unsigned byte, preventing it from going out of its bounds.
	inline unsigned char clmp(unsigned char ubyte) {
		ubyte =   ubyte >= 0 ?   ubyte : 0;
		return    ubyte <= 255 ?  ubyte : 255;
	}
};
