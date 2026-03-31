#pragma once
#include "framework.h"
#include "renderer.h"
#include <vector>
#include "../gfxmath/vec4.h"

//Class allowing for double-bounce Global Illumination, using voxel-cone tracing.
//Voxelizes scene geometry and stores it as voxels, in the datastructure on the GPU.
//Allows to:
//
// 1) Create voxels out of triangles. Stores them in the 3D textures.
//    Injects light into the appropriate voxels of those 3D textures.
//
// 2) Computes Global Illlumination using the 3D textures, from the point of view 
//	  of the camera. Returns a 2D texture that can be added on top of diffuse colors
//    seen by the viewer


class gameObject;
class shader;
class texture;
class mesh;
class camera;
class light;

class voxelGICore{
public:
	//GI_textures_res supplies the resolution of the Global Illumination's 
	//underlying datastructures.
	voxelGICore(size_t GI_textures_res, float world_size);

	~voxelGICore();

	//Render the meshes into cubes. Efficient when multiple game objects are passed
	//for processing at once.
	//A vector of lights has to be passed so that the voxels can be lit. Make sure
	//to pass those which ALREADY have their shadowmap updated and ready to use
	//TODO remove nesessity of rasterizing into the shadowmap
	void voxelizeMeshes(const std::vector<gameObject*>& gobjects,
						const std::vector<light*>& lights_with_shadowmaps);
	
	//clear all the voxel values of internal 3D textures
	void clearVoxels();


	//outputs the voxels visible to the camera. Works with one texture at a time.
	//Outputs onto the ALREADY bound frameBuffer (with at least 1 color attachment)
	//TODO accept bool depthTest (to have occluding geometry covering parts of
	//the 3d texture)
	void displayVoxels(const camera *cam, vec4 viewport);


	//create illumination textures, based on provided textures 
	//of scene viewed by camera.
	//If any required texture is abscent from the _renderTextures, (is nullptr), 
	//structure will still attempt to use it.
	//
	//For example, a structure will expect a normal (world-space),
	//specularity and depth textures to calculate the GI bounces.
	//provide it an FBO where to output the results, with 2 textures that were
	//already attached PRIOR to this function call.  
	//Those 2 textures must be:   where to ouput resulting illumination color
	//and illumination specularity. TODO we don't really need spec texture, it can 
	//be included into illumination anyway...
	//Just as other textures, those 2 will be queried from the array via 
	//renderer::renderTextures  and  renderer::lightRenderTextures enums.  
	//
	//Additional 2 light textures must also be supplied as the ARGUMENT, via 
	//_light-renderTextures array. It will be used in sampling of textures.
	//Since each shader thread will ONLY sample a texel that it will output into 
	//later and no other texels, those textures might as well be the ones 
	//attached to the FBO.
	//
	//The FrameBuffer must be cleared color and depth-wise BEFORE this function 
	//gets called. Structure won't clear anything, will just output into framebuffer.
	//This framebuffer must NOT have a stencil or depth attachments that are attached
	//Depth texture from _renderTextures will however be used.
	//
	//TODO if enum names changed, update them in this comment too.
	//
	//In addition to that, it requires the current renderer which issued this draw.
	void renderSecondBounce( const renderer *curr_renderer, 
						 GLuint outputFBO,
					  texture *_renderTextures[renderTextures::MAX_SIZE],
					  texture *_light_renderTextures[lightRenderTextures::MAX_SIZE]);


	float shader_control_value;
	float shader_control2_value; //TODO for debugging
	float shader_control3_value; //TODO for debugging
	float shader_control4_value; //TODO for debugging
	float shader_control5_value;
	float gi_resolution_divisor; //how much to downsample the screen-space GI pass. Higher = faster but blurrier.

protected:
	void modernVoxelize( const std::vector<gameObject*>& gobjects,
						 const std::vector<light*>& lights_with_shadowmaps );

	//follows modernVoxelize(), allows to compute average of attomically added color
	//values. Does it via compute shader.
	void voxelizeAverage();

	//for each voxel in the 3d texture volume it performs voxel cone tracing, 
	//gathering incoming light & storing it in the bounce_light_tex3D texture.
	void filterInto_1st_bounce();

	//functionality allowing us to compute GLobal illumination, as seen by the 
	//viewer, storing it into our internalFBO. 
	void voxelGICore::renderScreenGI(vec2 viewportSize, GLuint outputFBO,
							   const renderer *curr_renderer,
							   texture *_renderTextures[renderTextures::MAX_SIZE],
					  texture *_light_renderTextures[lightRenderTextures::MAX_SIZE]);


		//pure color of objects RGBA16F (alpha stores transparency)
		//This will allow us to perform atomic read/writes to the texture
		texture *basecolor_tex3D;

		//shading of lights multiplied by the basecolor.
		//Alpha stores transparency duplicated from the basecolor.
		//Format is RGBA16F
		//This will allow us to perform atomic read/writes to the texture
		texture *diffuse_tex3D;

		//used for counting the number of triangles that fall into each voxel.
		//Will be used in averaging out colors.
		//Format is RGBA32UI
		//unsigned integers allow us to perform atomic increments to the texture
		texture *count_texture;

		//normals,  with emissiveness brighness (how bright object should
		//glow on its own) in alpha.
		//Format is RGBA16F
		//This will allow us to perform atomic read/writes to the texture
		texture *normal_and_brighness_tex3D;
		
		//we want to only have to mipmap 2 textures at max. Diffuse and
		//this one. It contains 1st bounce gathered for each geometry voxel.
		//In alpha it contains SELF brightness of emissive illumination (self-glow). 
		//It will be copied from the normal_and_brightness_tex3D during construction
		// of this bounce_and_brightness_tex3D.
		//That way we will be able to mipmap 2 textures with all the data needed
		//for voxel cone tracing.
		//Format is RGBA16F
		//This will allow us to perform atomic read/writes to the texture
		texture *bounce_and_brightness_tex3D;

		//used in getting the scene rendered at the correct resolution from one
		//of the points of view, during voxelization stage.
		//Triangles are then restored and are inserted in one of our 3d texture, 
		//in exact correct position:
		//format is GL_R32
		texture *raster_tex2D;


		//tells how many units the texture set occupies (along each dimension)
		float datastructure_size_world; 



	GLuint internalFBO; //Frame Buffer Object used for rendering within this class.


  //To speed up rendering, GI will be computed for smaller resolution and
  //interpolation will take place. The following 3 textures will be used for that:

	texture* low_res_GITex;//low-res GI texture2D contribution, as seen by the viewer

	//resulting GI, after the interpolation stages were done. It is the GI 
	//contribution, visible to the viewer.
	texture* final_highResGITex;

	//in high pass, if a fragment can't recieve a GI value interpolated from the 
	//low-res texture, it will be marked "0" in this stencil texture. A further pass 
	//will compute GI for all such maked fragments.
	texture* high_res_depthStencilTex;



	shader *voxelization_shader; //used when converting triangles to voxel data.
	shader *modern_voxelization_shader; //used by openGL 4.3+
	shader *modern_voxelizeAverage_shader;//averages colors from voxelization stage.
	shader *displayVoxel_shader; //used to visualize voxels in a 3d texture.

	//used to calculcate 1st GI bounce for all voxels:
	shader *filter_1st_bounce_shader; 

	//outputs 
	shader *renderSecondBounce_shader;


	mesh *screen_quad;
};

