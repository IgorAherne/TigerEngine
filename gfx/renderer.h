#pragma once
#include <map>
#include <vector>
#include <set>
#include "shader.h"
#include "framework.h" //contains GLEW and GLFW
#include "input.h"
#include "../gfxmath/mat4.h"

class window;
class camera;
struct GLFWwindow;
struct distanceSortedObj;
class gameObject;
class texture;
class voxelGICore; //used for voxel global illumination


class mesh;



//used to query array of render textures of deferred rendering,
//in our renderer class
namespace renderTextures {
	enum renderTextures {
		POSITION_INDX = 0,
		
		//surface color with transparency bu WITHOUT light 
		//shadows, attenuation, lambert etc:
		BASECOLOR_INDX, 
		NORMAL_AND_EMISSION_INDX, //normal and self-glow (emission) texture
		SPECULAR_INDX,  //TODO check that all enums have properly increasing values: 0,1,2etc
		VELOCITY_INDX,
		DEPTH_INDX, //easier if depth is last.
		MAX_SIZE
	};
}

namespace lightRenderTextures {
	enum lightRenderTextures {
	   ILLUM_INDX=0,//used as illumination color result after shining a light.
					//to get diffuse color do  BASECOLOR*ILLUM
	   SPEC_ILLUM_INDX, //used as illumination specularity result.
	   MAX_SIZE
	};
}


//renderer cannot be instantiated outside of this class. 
//all the renderer instances are stored in a static map,  "rendererInstances".

class renderer {
	//any renderer can access other renderer.
	//And this static function of the input class:
	friend void input::updateInput(float);
	//window's resize callback  can force us resize our textures:
	friend void windowResize(GLFWwindow *, int, int);

	friend class pointLightGI;

public:
	//get one of the renderers (renderer #0 by default)
	//Avoid calling it often. Instead, store Renderer pointer when it is
	//created by renderer::createRenderer(), then use this pointer when needed.
	//To delete this pointer call  renderer::destroyRenderer().
	//If no renderer was found - returns nullptr
	inline static renderer *getRenderer(size_t renderer_id = 0) {
		auto renderer_address = rendererInstances.find(renderer_id);

		if (renderer_address != rendererInstances.end()) {
			//return the renderer's shader
			return renderer_address->second;
		}
		return nullptr;
	}


	//tells how many render instances there are.
	static inline size_t getNumRenderers() {
		return rendererInstances.size();
	}

	//get the id of this renderer. This id is used in rendererInstances static map
	inline size_t rendererID() {
		return renderer_id;
	}


	//returns pointer to Renderer if it was created.
	//Once done, store the returned pointer and use it, instead of  
	//renderer::getRenderer() function, because it is slower.
	//
	//Supply (optionaly) a renderer, whose context should be shared with this 
	//created renderer's context. This means using same shader programs, VBOs and 
	//textures, etc. If no renderer is shared, then all of those things will have
	//to be re-created / re-uploaded for this new renderer's context.
	static renderer *createRenderer(std::string window_name, size_t w_width, 
									size_t w_height, 
									renderer *shared_renderer = nullptr) {

		int	renderer_id = 0;//will change to a greater value if there are other ids.

		auto iterator = rendererInstances.rbegin(); //get last renderer in map.
		if (iterator != rendererInstances.rend()){ //if there is a renderer
			renderer_id = iterator->first + 1; //our id will be +1 greater.
		}


		renderer *r = new renderer(	window_name, w_width, w_height,
									renderer_id, shared_renderer );

		rendererInstances[renderer_id] = r;
		return r;
	}



	//destroy the given renderer. Returns true if the renderer was deleted.
	//False if there was no renderer with this ID.
	static bool destroyRenderer(size_t renderer_id) {
		auto renderer_address = rendererInstances.find(renderer_id);

		if (renderer_address != rendererInstances.end()) {
			delete renderer_address->second; //delete renderer
			rendererInstances.erase(renderer_address); //remove entry from map
			return true; //signal that deletion was successful
		}
		return false;
	}


	//introduce this renderer a new camera
	inline void registerCamera(camera *newCamera) {
		cameras.insert(newCamera);
	}

	//get all the cameras tracked by this renderer
	inline std::set<camera*> getMyCameras() {
		return cameras;
	}

	//returns a window of this renderer. Null if not possible.
	inline const window *get_window() const {
		return m_window;
	}

	//returns 
	inline const camera *getCurrentCamera() const {
		return curr_camera;
	}


	//envoke render on all renderers
	//dt is time elapsed since the previous frame
	//false is returned if there are no  renderers  in renderInstances.
	static bool renderAllrenderers(float dt);

	//current view matrix of this renderercan be accessed from anywhere, 
	//espessially by materials, when this renderer draws them.
	//It can then bind them to the shader, as a uniform, or use it elsewhere
	mat4 get_curr_view_mat4() const {
		return this->curr_view_mat4; 
	}
	//current projection matrix of this renderercan be accessed from anywhere, 
	//espessially by materials, when this renderer draws them.
	//It can then bind them to the shader, as a uniform, or use it elsewhere
	mat4 get_curr_proj_mat4() const {
		return this->curr_proj_mat4;
	}



private:
	//private constructor. DON'T EVER MAKE A PUBLIC ONE.
	//shared renderer allows to share its context with this renderer's window.
	//That way, we won't have to re-create shader objects, textures, FBOs and VBOs,
	//etc.
	//TODO resolve multi-windows. Right now VAOs and FBOs are not shared.
	renderer( const std::string window_name, size_t w_width,
					size_t w_height, size_t thisRenderer_id, 
					renderer* shared_renderer = nullptr);

	~renderer(); //private destructor. NEVER MAKE A PUBLIC ONE.


	//don't allow anyone except for Renderer class 
	//to delete a pointer of Renderer class:
	void operator delete(void *ptr) {
		delete ptr;
	}
	void operator delete[](void *ptr) {
		delete[] ptr;
	}

	//render everything related to THIS renderer and all of its cameras.
	//the output is delivered to its window or any other resulting target.
	void renderRenderer(float dt);

	//called by window when it's dimensions get changed. Asks this renderer
	//to resize it's screen textures, to match the window's dimensions
	void resize_all_screen_textures();

	void onWindowResize();


	//render a specific game object
	void render_gObject(gameObject *go);

	//returns the viewport dimensions. If cammera insists on using its own viewport
	//dimensions, then they are returned. Otherwise, our window's dimensions 
	//define the new aspect ratio of the camera (camera copies it in), 
	//and the returned viewport dimensions.
	vec4 update_camera_aspect(camera *cam);
	
	//point the camera's view and projection matrices 
	//to   this->curr_view_mat4   and   this->curr_proj_mat4
	void set_curr_matrices(camera *cam);

	//render all opaque geometry into the geometry frameBuffer.
	//That way we get base colors, normals, positions (both in eye space),
	//specularity and depth.
	//
	//requests viewport (x-position, y-position, width, height) 
	//and  a vector of structs that contain sqr_distance from camera and a gameObject
	void opaque_geom_pass(	vec4 viewport,
							std::vector<distanceSortedObj> &frust_contents);

	//lighting the gFramebuffer of this renderer, using lights in the scene.
	//Draws each mesh of the light, with the scale based on the light's range.
	void lighting_pass();

	//makes use of the voxelGI structure, and extracts real-time Global illumination
	//which lights the scene.
	//Then applis the fetched lighting directly into the back buffer.
	//
	//Make sure you've update voxel global illumination structures before you
	//extract information from it. It means using functions like
	//voxelGICore::clearVoxels();
	//voxelGICore::voxelizeMeshes();
	//
	//Also, make sure you call this function only when you have something in
	//basecolor (unlit, raw material color), normal and depth renderTextures, 
	//since it will work in deferred mode.
	void lighting_voxelGI_pass();

	//here we will draw all of the shapes used for debugging - for example, the 
	//positions of lights as small spheres.
	//supply it the bot left corner and width and height of the area to output to.
	void gizmo_pass(vec4 viewport);

private:
	//using map since we need to have keys in a sorted order.
	//this way we can find the min / max key.
	static std::map<size_t, renderer*> rendererInstances;

	//setup texture-samplers in the shader, binding each one
	//to its appropriate texture unit.
	//We set them up for the shader which applies the accomulated illumination
	//to the visible scene, "lighting" it up.
	//
	//typically called once, at the creation of the renderer, telling the 
	//lightCommitShader which texture units to use, for which textures.
	//However, as the window is resized, renderer might call it to update 
	//uniforms like "pixelSize".
	void setup_lightCommit_uniforms();

	//generates gFBO, lightFBO and attaches corresponding textures.
	//The textures must have already been agenerated 
	//and settted-up via glTexImage2D()
	void initFrameBuffers();
	

	//shader which commits the accomulated illumination to the 3visible scene texture.
	//It typically happens in the end of the lighting pass.
	shader *lightCommitShader;

	//shader to draw all the debug shapes
	shader *gizmo_shader;

	std::set<camera*> cameras;  //cameras processed by THIS renderer.

	//as the rendering happens for various cameras, they get set as "current".
	//Any rendering stage, envoked outside of the renderer can get hold of
	//certain information about this camera, provided through
	//appropriate getters of this renderer.
	camera *curr_camera; 


	//texture outputs, used in deferred rendering
	texture *_renderTextures[renderTextures::MAX_SIZE]; 
	//texture outputs of lights, used in deferred rendering
	texture *_light_renderTextures[lightRenderTextures::MAX_SIZE];
	
	//geometry frame buffer object of this renderer. Takes all the renderTextures as
	//its attachments.
	GLuint gFBO;
	//used to store diffuse color resulting from the lights' illumination 
	//as well as specularity.
	GLuint lightFBO;

	mat4 curr_view_mat4;//those two can be accessed from anywhere, (via getters)
	mat4 curr_proj_mat4;//espessially by materials, when this renderer draws them

	mesh *screenQuad; //rectangle of the shape of the screen.
	mesh *sphereMesh; //used as a gizmo, for instance, during gizmo_pass()

	//object holding information about all the geometry processed by this renderer
	//in terms of voxels. They are used by it to compute Global Illumination.
	voxelGICore *_voxelGICore; 
	
	//the window. THIS renderer works with its OGL context
	window *m_window;
	size_t renderer_id;  //id of THIS renderer.  It's used in rendererInstances
};

