#pragma once
#include "framework.h"

namespace tex_domain {
	enum tex_domain {
		texture1D = GL_TEXTURE_1D,
		texture2D = GL_TEXTURE_2D,
		texture3D = GL_TEXTURE_3D,
		textureCube = GL_TEXTURE_CUBE_MAP,
		error     = -1, //can happen if incorrect resolution is used
	};
}




struct texture {
	//define a shortcut to the "texture tex_domain" enum
	typedef tex_domain::tex_domain domain; 

public:


	//Setup an empty texture1D, texture2D or texture3D. To create an empty
	//cubemap use:
	//texture(GLuint, size_t) overloaded method instead.
	//
	//internal_format is what the texture contents are on the GPU: 
	//RGBA8, RGBA16, GL_DEPTH_COMPONENT32, etc.
	//By default, z resolution is 0, resulting in 2D texture.
	//set both z and y to zero if you want 1D texture.
	//Setting x to zero will result in  m_domain  changed to "error"
	texture( GLuint internal_format, bool createMipMaps,
			 size_t x_res, size_t y_res, size_t z_res=0);


	//get the 2D texture generated and loaded up onto openGL, using the 
	//texture name.
	//Supply true if the texture has 4 channels (RGBA8)
	//if its RGB8 - supply false
	//TODO add support for 16 and 32 bit per channel and pure bitmap + depth + cube
	texture(std::string textureURL, bool createMipMaps, bool isFourChannel=false);


	//This overload sets up an empty CUBEMAP texture.
	//
	//internal_format is what the texture contents are: 
	//GL_RGBA, GL_RGBA16, GL_DEPTH_COMPONENT32, etc.
	//Setting resolution to zero will result in  m_domain  changed to "error".
	//Later, the resolution of texture will be 
	//stored in x-dimension resolution variable (in y and and z too, but use x).
	texture(GLuint internal_format, bool createMipMaps, size_t resolution);

	~texture();


	//see if this texture is 1D, 2D, 3D or even is an error (check resolutions)
	inline domain get_texture_domain() const {
		return m_domain;
	}

	//find out how this texture is "called" in openGL.
	inline GLuint get_OGL_id() const {
		return m_texture_OGL_id;
	}

	//see how the texture's texels are treated. 
	//They might be GL_RGBA8, GL_DEPTH_COMPONENT32, etc
	inline GLuint get_internal_format() const {
		return m_internal_format;
	}

	//the first dimension, present in all textures, including cubemap resolution.
	inline size_t get_xRes() const {
		return m_Xres;
	}

	//second dimension, present in texture2D and texture3D, but not cubemap
	inline size_t get_yRes() const {
		return m_Yres;
	}

	//resolution of the third dimension, present in texture3D, but not cubemap
	inline size_t get_zRes() const {
		return m_Zres;
	}


	//so far only supports layout(rgba8), and only with the textures of same sizes
	//on all dimentions TODO: allow various sizes and improve this layout qualifier?
	//
	//if mip_offset is specified, begins generating from that mip level downwards.
	//For example, if texture is 512x512, and mip_offset == 1, the first mip
	//that will be computed will be 128x128. It will base its values on the mip 
	//above, the 256x256 one.
	//Texture should not be stencil or depth   TODO fix this?
	 void generateMips(float shader_control_value, size_t mip_offset=0);

	
	//associates the texture id with the texture unit for
	//a CURRENTLY ACTIVE shader.
	//
	inline void associate_to_TextureUnit(size_t texture_unit) {
		glActiveTexture(GL_TEXTURE0 + texture_unit);
		glBindTexture(m_domain, m_texture_OGL_id);

		//we need to activate some texture unit that won't be used, so
		//that some other textures can be bound without getting associ-
		//ated with the GL_TEXTURE0:
		glActiveTexture(GL_TEXTURE0 + 99); //TODO fix,  might not be enough!
	}



	//quickly tell if the texture's internal format is one of the depth literals.
	inline bool is_depth() const {
		 return    m_internal_format == GL_DEPTH_COMPONENT
				|| m_internal_format == GL_DEPTH_COMPONENT16
				|| m_internal_format == GL_DEPTH_COMPONENT24
				|| m_internal_format == GL_DEPTH_COMPONENT32
				|| m_internal_format == GL_DEPTH24_STENCIL8;
	}


	//quickly tell if the texture's internal format is one of the stencil literals.
	inline bool is_stencil() const {
		return m_internal_format == GL_DEPTH24_STENCIL8;
	}



	//Setting x to zero will result in m_domain changed to "error".
	//If the domain of texture is 2D, and x, y, z are supplied, z is ignored, etc.
	//Function will call the allocateTexture() in the end.
	//If you want to pass new data while resizing, use  resize_with_data() instead.
	inline void resize(size_t x_res, size_t y_res, size_t z_res) {
		switch (m_domain) {
			case domain::texture1D:{
				m_Xres = x_res;
				m_Yres = 0;
				m_Zres = 0;
			}break;
			case domain::texture2D:{
				m_Xres = x_res;
				m_Yres = y_res;
				m_Zres = 0;
			}break;
			case domain::texture3D:{
				m_Xres = x_res;
				m_Yres = y_res;
				m_Zres = z_res;
			}break;
		}//end switch

		
		 //"wipe out" all the texel values in the texture:
		
		//TODO watch out, format might not be RGBA. Extract external format from
		//m_internal_format. Functionality can be found in the texture::texture()
		//and placed in a separate function for simplicity.
		//It can then be used form here as well
		allocateTexture(mipmapped,
					    this->is_depth() ? GL_DEPTH_COMPONENT : GL_RGBA  );
	}



	
	//Uploads data to the texels while resizing. Use this to speed up texel-filling
	//process.  resize() would send nulldata to the GPU. If you have actual data,
	//why not to supply it here, so it's sent straghtaway.
	//
	//Setting x to zero will result in _type changed to "error".
	//If the domain of texture is 2D, and x, y, z are supplied, z is ignored, etc.
	//Function will call the allocateTexture() in the end.
	//remember, the format should be GL_DEPTH_COMPONENT if the texture is depth.
	//Or it can be GL_RGBA, GL_RGB etc.
	void resize_with_data( size_t x_res, size_t y_res, size_t z_res,
						   void *data, GLenum format, 
						   GLenum data_type = GL_UNSIGNED_BYTE );


	//Creates a texture id.
	//Allocates the space for this texture to the GPU. 
	//Allocated texels are supplied with nulldata or the provided data.
	//Format helps to interpret the future data 
	//(in groups of 3 like GL_RGB would, etc).
	//recall that GL_DEPTH_COMPONENT would be used for interpreting depth data.
	//Data_type helps to understand the "building" blocks in the future data;
	//Can be GL_UNSIGNED_BYTE, etc (if future data is an array of unsigned bytes).
	void allocateTexture( bool allocate_mipmap_space,
					      GLuint format, 
						  unsigned char *data = nullptr,
					      GLenum data_type=GL_UNSIGNED_BYTE
						  );

	
	//how interpolation should occur within texture. Default is GL_LINEAR.
	//use things like GL_NEAREST, GL_LINEAR_MIPMAP_LINEAR for trilinear filtering.
	//Note that GL_LINEAR_MIPMAP_NEAREST will be bilinear filtering, with nearest
	//mipmap, resulting in sharp transitions between mipmaps.
	inline void setFiltering(GLint interpolation = GL_LINEAR) {
		glBindTexture(m_domain, m_texture_OGL_id);
		glTexParameteri(m_domain, GL_TEXTURE_MIN_FILTER, interpolation); 

		switch (interpolation) {
			case GL_NEAREST:{
				glTexParameteri(m_domain, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}break;
			case GL_LINEAR:{
				glTexParameteri(m_domain, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}break;
			case GL_NEAREST_MIPMAP_NEAREST:{
				glTexParameteri(m_domain, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}break;
			case GL_LINEAR_MIPMAP_LINEAR:{
				glTexParameteri(m_domain, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}break;
		}
		glBindTexture(m_domain, 0);
	}

	//tell if texture will tile itself (default) when UV get outside of [0, 1] range
	//or if you want it GL_CLAMP_TO_EDGE or GL_MIRRORED_REPEAT, etc
	inline void setWrap(GLuint wrap = GL_REPEAT) {
		glBindTexture(m_domain, m_texture_OGL_id);
		
		glTexParameteri(m_domain, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(m_domain, GL_TEXTURE_WRAP_T, wrap);
		if (m_domain == GL_TEXTURE_CUBE_MAP) {//if we are cube, set up 3rd dimension
			glTexParameteri(m_domain, GL_TEXTURE_WRAP_R, wrap);
		}

		glBindTexture(m_domain, 0);
	}


protected:

	//sorts-out this class instance to contain all the nessesary information
	//about the texture.
	void setupTexture( GLuint internal_format, bool createMipMaps, 
					   unsigned char *data,
					   size_t x_res, size_t y_res/*=0*/, size_t z_res/*=0*/);

	void setupCubeTexture( GLuint internal_format,
						   bool createMipMaps, 
						   size_t resolution,
						   unsigned char *data=nullptr);


	GLuint m_texture_OGL_id; //the id on OpenGL side of this texture.

	domain m_domain; //is it a 1D, 2D or 3D texture.
	GLuint m_internal_format; //GL_RGBA, GL_RGBA16, GL_DEPTH_COMPONENT32, etc

	bool mipmapped; //is the texture using mipmaps


	size_t m_Xres; //used if the texture is at least a 1-dimensional texture.

	size_t m_Yres; //y is used if the texture is at least 2-dimensional.  Default=0
	size_t m_Zres; //resolution, where z is only used if the texture is 3D. Default=0
};

