#include "texture.h"
#include "shader.h"
#include "repositories.h"

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image\stb_image.h"


void texture::setupTexture( GLuint internal_format, bool createMipMaps,
							unsigned char *data,
							size_t x_res, size_t y_res/*=0*/, size_t z_res/*=0*/){

	m_internal_format = internal_format;
	this->mipmapped = createMipMaps;


	if (x_res > 0 && y_res == 0 && z_res == 0) {
		m_domain = domain::texture1D;
	} else if (x_res > 0 && y_res > 0 && z_res == 0) {
		m_domain = domain::texture2D;
	} else if (x_res > 0 && y_res > 0 && z_res > 0) {
		m_domain = domain::texture3D; //if all 3 are above 0 we are a texture3D
	} else {
		m_domain = domain::error;
		return;
	}

	m_Xres = x_res; //setup resolutions
	m_Yres = y_res;
	m_Zres = z_res;

	glGenTextures(1, &m_texture_OGL_id);


	GLint external_format = 0;
	//TODO maybe we missed some other internal formats, so add them here to 'if' 
	//check:

	GLuint dataType = GL_UNSIGNED_BYTE;

	switch (m_internal_format) {
		case GL_R16:{
			external_format = GL_RED;
		}break;
		case GL_R16F:{
			external_format = GL_RED;
			dataType = GL_HALF_FLOAT;
		}break;
		case GL_R32F:{
			external_format = GL_RED;
			dataType = GL_FLOAT;
		}break;
		case GL_R32UI:{
			external_format = GL_RED_INTEGER;
			dataType = GL_UNSIGNED_INT;
		}break;
		case GL_R32I:{
			external_format = GL_RED_INTEGER;
			dataType = GL_INT;
		}break;

		case GL_RGB8:{
			external_format = GL_RGB;
		}break;
		case GL_RGB12:{
			external_format = GL_RGB;
		}break;
		case GL_RGB16:{
			external_format = GL_RGB;
		}break;
		case GL_RGB16F:{
			external_format = GL_RGB;
			dataType = GL_HALF_FLOAT;
		}break;
		case GL_RGB32F:{
			external_format = GL_RGB;
			dataType = GL_FLOAT;
		}break;
		case GL_RGB32UI:{
			external_format = GL_RGB;  //TODO perhaps GL_RGB_INTEGER?
			dataType = GL_UNSIGNED_INT;//or unsigned integer?
		}break;
		case GL_RGB32I:{
			external_format = GL_RGB;	//TODO same here?
			dataType = GL_INT;
		}break;//TODO add GL_RGB16UI and ints
		

		case GL_RGBA8:{
			external_format = GL_RGBA;
		}break;
		case GL_RGBA12:{
			external_format = GL_RGBA;
		}break;
		case GL_RGBA16:{
			external_format = GL_RGBA;
		}break;
		case GL_RGBA16F:{
			external_format = GL_RGBA;
			dataType = GL_HALF_FLOAT;
		}break;
		case GL_RGBA32F:{
			external_format = GL_RGBA;
			dataType = GL_FLOAT;
		}break;


		case GL_RGBA32UI:{
			external_format = GL_RGBA;  //TODO perhaps GL_RGB_INTEGER?
			dataType = GL_UNSIGNED_INT;//or unsigned integer?
		}break;
		case GL_RGBA32I:{
			external_format = GL_RGBA;	//TODO same here?
			dataType = GL_INT;
		}break;//TODO add GL_RGB16UI and ints
			
		default:{
			external_format = GL_RGBA;
			dataType = GL_UNSIGNED_BYTE;
		}
	}//end switch(dataType)
 
	allocateTexture( mipmapped, 
				     this->is_depth()? GL_DEPTH_COMPONENT:external_format,
					 data, dataType );

	this->setWrap(GL_REPEAT);
	this->setFiltering(createMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

	glBindTexture(m_domain, 0);
}




void texture::setupCubeTexture(GLuint internal_format, bool createMipMaps, 
								size_t resolution, unsigned char *data/*=nullptr*/){

	m_internal_format = internal_format;
	m_Xres = m_Yres = m_Zres = resolution;

	this->mipmapped = createMipMaps;

	if (resolution == 0) {
		m_domain = domain::error;
		return;
	} else {
		m_domain = domain::textureCube;
	}

	glCreateTextures(m_domain, 1, &m_texture_OGL_id);
	glBindTexture(m_domain, m_texture_OGL_id);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


	if (this->is_depth()) {
		//TODO manual shadow comparison, screw COMPARE_R_TO_TEXTURE
		glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE,
			GL_COMPARE_R_TO_TEXTURE);
		glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	this->setWrap(GL_CLAMP_TO_EDGE);
	this->setFiltering(createMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);


	GLint external_format= 0;
	//TODO maybe we missed some other internal formats, so add them here to 'if' 
	//check:
	if (m_internal_format == GL_RGB8 || m_internal_format == GL_RGB12 ||
		m_internal_format == GL_RGB16 || m_internal_format == GL_RGB32F) {
		external_format = GL_RGB;
	}
	else {
		external_format = GL_RGBA;
	}

	allocateTexture( mipmapped, 
					 this->is_depth()? GL_DEPTH_COMPONENT:external_format,
					 data );

	glBindTexture(m_domain, 0);
}




//setup an empty texture
texture::texture( GLuint internal_format, bool createMipMaps,
				  size_t x_res, size_t y_res/*=0*/, size_t z_res/*=0*/ ) {
	
	setupTexture(internal_format, createMipMaps, nullptr, x_res, y_res, z_res);
}




//setup an empty cubemap
texture::texture(GLuint internal_format, bool createMipMaps, size_t resolution) {
	
	setupCubeTexture(internal_format, createMipMaps, resolution, nullptr);
}





texture::texture(std::string textureURL, bool createMipMaps, 
				 bool isFourChannel/*=false*/) {
	int comp;
	unsigned char *texdata = stbi_load( textureURL.c_str(), (int*)&m_Xres, (int*)&m_Yres,
										&comp, 0);
	GLuint format = 0;
	std::string extention = textureURL.substr(textureURL.find(".") + 1); 

	setupTexture(isFourChannel ? GL_RGBA8 : GL_RGB8, createMipMaps, texdata, 
				 m_Xres, m_Yres,0);
}



texture::~texture() {
	glDeleteTextures(0, &m_texture_OGL_id);
}



//helper function allowing to get maximum value
int get_max(size_t a, size_t b) {
	if (a >= b) {
		return a;
	}
	return b;
}



void texture::generateMips(float shader_control_value, size_t mip_offset/*=0*/) {
	if (m_domain == domain::texture3D && GLEW_VERSION_4_3) {
		//if it's a 3d texture and we have 4.3 ogl - we won't use CPU automatic
		//mipmapping. Instead we will use our compute shader! 
		shader *computeShader;

		if (m_internal_format == GL_RGBA16F) {
			computeShader = repositories::getShader("mipmapping3DtexRGBA16F");
		}
		else {
			computeShader = repositories::getShader("mipmapping3DtexRGBA8");
		}

		glUseProgram(computeShader->getShaderProgramID());

		GLuint loc = glGetUniformLocation(computeShader->getShaderProgramID(),
											"upper_mipTex");
		glUniform1i(loc, 1);

		//see how many mip levels our texture should have
		int total_mip_levels = std::log2f(m_Xres) + 1;

		//determine what is the max number of work groups avalable for the 
		//compute shader
		//TODO make actual use of this stat. groups are massive 
		//2147483647, 65536, 65536 on GTX980
		int work_grp_cnt[3];
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);


		//Initially, we planned on using GL_LINEAR and sampling in the middle of
		//eight texels. However, OpenGL seems to do something wierd with alphas,
		//And we are forced to sample 8 manual samples.
		//TODO fix this?
		this->setFiltering(GL_NEAREST); 
		//DONT SUPPLY GL_LINEAR_MIPMAP_LINEAR, since we will fetch only from a 
		//specific mipmap!

		glDisable(GL_BLEND);

		mip_offset == std::fmaxf(mip_offset, 0);

		for (int mip_level = 1+mip_offset; 
				mip_level < total_mip_levels; 
				++mip_level) {

			//framework::fetch_OGL_errors();

			loc = glGetUniformLocation(computeShader->getShaderProgramID(),
										"upper_mip_lvl");
			glUniform1i(loc, mip_level-1); //upload the "previous mip" level 

			loc = glGetUniformLocation(computeShader->getShaderProgramID(),
										"control");
			glUniform1f(loc, shader_control_value);
					 

			loc = glGetUniformLocation(computeShader->getShaderProgramID(),
										"upper_mip_numTexels");


			int upper_mip_res = m_Xres / (std::powf(2, mip_level - 1));
			glUniform1f(loc, upper_mip_res );

			//bind appropriate mip-level of the texture to texture unit 0:
			//TODO GL_TRUE is not really required, but without it things
			//didn't work on university's GTX780 TI.
			glBindImageTexture(  0, m_texture_OGL_id, mip_level, GL_TRUE, 0,
									GL_WRITE_ONLY, m_internal_format  );

			//simmilar thing happens to level of "mip that is larger" than 
			//this one:
			//However, we will be using textureLod() on it, so we can bind it
			//as a sampler, to increase performance.
			this->associate_to_TextureUnit(1);

			//TODO allow for unequal-sized dimention textures as well:
			GLuint work_unit_dim = upper_mip_res/2;

			glDispatchCompute(work_unit_dim, work_unit_dim, work_unit_dim);
									   
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glBindTexture(GL_TEXTURE_3D, 0);
		}//end for each mip level

		glUseProgram(0);
	}

	else if (m_domain == domain::texture2D && GLEW_VERSION_4_3) {
		//if it's a 2d texture and we have 4.3 ogl - we won't use CPU automatic
		//mipmapping. Instead we will use our compute shader! 

		shader *computeShader = repositories::getShader("mipmapping2Dtex");
		glUseProgram(computeShader->getShaderProgramID());


		GLuint loc = glGetUniformLocation(computeShader->getShaderProgramID(),
											"upper_mipTex");
		glUniform1i(loc, 1);


		//see how many mip levels our texture should have
		int total_mip_levels = std::log2f(m_Xres) + 1;

		//determine what is the max number of work groups avalable for the 
		//compute shader
		//TODO make actual use of this stat. groups are massive 
		//2147483647, 65536 on GTX980
		int work_grp_cnt[2];
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);


		this->setFiltering(GL_LINEAR); //allow for BILINEAR 2d filtering
		//that way we won't have to sample 4 values - hardware will do it for us.
		//DONT SUPPLY GL_LINEAR_MIPMAP_LINEAR, since we will fetch only from a 
		//specific mipmap!


		for (int mip_level = 1; mip_level < total_mip_levels; ++mip_level) {

			loc = glGetUniformLocation(computeShader->getShaderProgramID(),
										"upper_mip_lvl");
			glUniform1i(loc, mip_level - 1); //upload the "previous mip" level 


			loc = glGetUniformLocation(computeShader->getShaderProgramID(),
										"upper_mip_numTexels");

			int upper_mip_res = m_Xres / (std::powf(2, mip_level - 1));
			glUniform1f(loc, upper_mip_res);

			//bind appropriate mip-level of the texture to texture unit 0:
			//TODO GL_TRUE is not really required, but without it things
			//didn't work on university's GTX780 TI.
			glBindImageTexture(0, m_texture_OGL_id, mip_level, GL_TRUE, 0,
								GL_WRITE_ONLY, m_internal_format);

			glActiveTexture(GL_TEXTURE1); //texture unit 1, as setup above.
			glBindTexture(GL_TEXTURE_2D, m_texture_OGL_id);

			//TODO allow for unequal-sized dimention textures as well:
			GLuint work_unit_dim = upper_mip_res / 2;

			glDispatchCompute(work_unit_dim, work_unit_dim, 1);

			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

			glBindTexture(GL_TEXTURE_2D, 0);
		}//end for each mip level



		glUseProgram(0);
	}
	else { //otherwise we got to do mip mapping on cpu side
		//TODO thread out or do via custom shader + screen sized quad?
		glBindTexture(m_domain, m_texture_OGL_id);
		glGenerateMipmap(m_domain);
		glBindTexture(m_domain, 0);
	}
}




void texture::allocateTexture(	bool allocate_mipmap_space,
								GLuint format,
								unsigned char *data /*=nullptr*/,
								GLenum data_type/*=GL_UNSIGNED_BYTE */
								) {

	glBindTexture(this->m_domain, m_texture_OGL_id);

	int total_mip_levels = std::log2f(m_Xres) + 1;
	
	if (!allocate_mipmap_space) {
		total_mip_levels = 1;
	}

	glTexParameteri(m_domain, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(m_domain, GL_TEXTURE_MAX_LEVEL, total_mip_levels);

	switch (m_domain) {
		case domain::texture1D:{
			//tell how to interpret the texture on the OGL side, where to get it
			//from in C++, client side and the format it will be in that 
			//"C++ location"
			glTexStorage1D(GL_TEXTURE_1D, total_mip_levels, m_internal_format, m_Xres);
			if (data != nullptr) {
				for (int mip = 0; mip < total_mip_levels; mip++) {
					glTexSubImage1D(GL_TEXTURE_1D, mip, 0, m_Xres, format, data_type, data);
				}
			}//end if
		}break;
		
		case domain::texture2D:{
			int width = m_Xres;
			int height = m_Yres;

			for (int mip = 0; mip < total_mip_levels; mip++) {
				framework::fetch_OGL_errors();

				//will allow resizing. 
				//WIth glTexStorage2D we would have to re-create texture every time its resized.
				//However, renderer screen textures are all 2D and might get resized many times.
				glTexImage2D(m_domain, mip, m_internal_format, width, height,
								0, format, data_type, data);
				framework::fetch_OGL_errors();

				width = max(1, (width / 2));
				height = max(1, (height / 2));
			}
		}break;
		
		case domain::texture3D:{
			glTexStorage3D(GL_TEXTURE_3D, total_mip_levels, m_internal_format, m_Xres, m_Yres, m_Zres);
			if (data != nullptr) {
				for (int mip = 0; mip < total_mip_levels; mip++) {
					glTexSubImage3D(GL_TEXTURE_3D, mip, 0, 0, 0, m_Xres, m_Yres, m_Zres, format, data_type, data);
				}
			}//end if
		}break;

		case domain::textureCube:{
			for (size_t i = 0; i < 6; ++i) {
				int width = m_Xres;
				int height = m_Yres;
				//TODO just as above, employ glTexSubImage3D and storage
				for (int mip = 0; mip < total_mip_levels; mip++) {
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
									mip, m_internal_format,
									width, height, 0,

									format, data_type, data);
					width = max(1, (width / 2));
					height = max(1, (height / 2));
				}
			}//end for 6
		}break;
		//note, there is no case for "error" value. We simply don't do anything.
	}
	glBindTexture(this->m_domain, 0);

}//end allocateTexture