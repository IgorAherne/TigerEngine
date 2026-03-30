#pragma once
#include <map>
#include <vector>
#include "light.h"

class gameObject;

class scene{
	friend class light;

public:
	scene(size_t scene_id, bool make_current_scene = false);
	~scene();

	inline static scene *getCurrentScene() {
		if (currentScene)//by default, static currentScene is initialized to nullptr
			return currentScene;

		setCurrentScene(-1); //this will spot that current scene is nullptr
		return currentScene; //and will create a new scene with id zero.
	}



	inline gameObject *getRootGameObject() {
		return root_gameObj;
	}

	//get all the meshes within the scene. NOT GAME OBJECTS!
	std::vector<mesh*> get_scene_Meshes();

	//get all the gameObjercts in the scene (except the root)
	std::vector<gameObject*> get_scene_GameObjects();


	inline std::vector<light*> get_scene_lights() {
		return scene_lights;
	}


	inline static void setCurrentScene(size_t scene_id) {
		auto iterator = scenes.find(scene_id);

		if (iterator != scenes.end()) {
			currentScene = iterator->second;
			return;
		}
		
		//otherwise map is empty - create a default scene, store in map,
		//and make it current scene.
		scenes[0] = new scene(0, true); //true will make it a current scene.
	}


	//true if erased a scene, false otherwise
	inline static bool destroyScene(size_t scene_id) {
		auto itrtr = scenes.find(scene_id);

		if (itrtr != scenes.end()) {
				if (itrtr->second == currentScene) {
				//	currentScene = nullptr; //reset current scene to nullpointer.
				}

				delete itrtr->second;
		}

		return scenes.erase(scene_id);
	}


	//Use after the scene was created, for creating objects and attaching components.
	//Provide it a function pointer with the code nessesary.
	void init( bool (*custom_init_function)() );

	//update every gameObject's components and then every gameObject itself.
	void update(float dt);


private:
	//called by all lighting components
	void registerLight(light *_light) {
		scene_lights.push_back(_light); //TODO make sure there are no duplicate lights
	}

	//Gets called by scene::update() automatically, 
	//invokes Update() on each gameObject
	void invoke_Updates_On_GameObjs(float dt, gameObject *gameObj);


	//new_scene_id will be set to the biggest current ID of all the scenes+1 if 
	// -1 (default value) is supplied.
	//otherwise the requested ID will be used.
	//Returns a newly created scene*
	//Otherwise, returns nullpointer if the requested ID was already used.
	static scene *createScene(int new_scene_id = -1) {
		if (new_scene_id == -1) {
			new_scene_id = 0;//will be zero in case the following "if" fails:

			auto it = scenes.rbegin();
			if (it != scenes.rend()) {
				new_scene_id = it->first + 1;//new id is 1 greater than the biggest.
			}

			auto pair = std::pair<size_t, scene*>(new_scene_id,
				new scene(new_scene_id));
			scenes.insert(pair);
			return pair.second;
		}

		//TODO test if new_scene_id was already present not to override anything.
		//map::insert does it. Change the code, then test. Or at least add
		//the check to see if the id was already present.

		//else, the exact scene id was supplied. We have to use it.
		//However, first check if there is already a scene with such id, so we don't 
		//overwrite it.
		if (scenes.find(new_scene_id) == scenes.end()) {
			auto pair = std::pair<size_t, scene*>(new_scene_id,
				new scene(new_scene_id));
			scenes.insert(pair);
			return pair.second;
		}
		return nullptr; //nullpointer if the requested ID was already used.
	}


private:
	static scene *currentScene; //currently active scene

	static std::map<size_t, scene*> scenes;

	size_t scene_id; //id of this scene in scenes map.
	gameObject *root_gameObj;

	std::vector<light*> scene_lights;
};

