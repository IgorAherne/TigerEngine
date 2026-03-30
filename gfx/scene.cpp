#include "scene.h"
#include "gameObject.h"

//initialize the static variables:
scene *scene::currentScene = nullptr;
std::map<size_t, scene*> scene::scenes;


scene::scene(size_t scene_id, bool make_current/*= false*/){
	this->scene_id = scene_id;

	if (make_current) { //if the user wanted
		scene::currentScene = this; //to make this scene current, - do so.
	}
	//it's important to set it as current before we create a  root gameObject,
	//since it will query the  scene::currentScene

	//must be set for prior to following gameObject instantiation.
	//that way this game object will be able to claim that it's the new root!
	this->root_gameObj = nullptr; 
	this->root_gameObj = new gameObject("root");
}


scene::~scene(){
	delete root_gameObj; //will delete every gameObject attached to it.
}


//function used internally only in this cpp file.
//Allows to "attach" pointers to all the children of this gameObject.
//The function is recursive, so at the end, total_children will 
//hold all the encountered children.
void internal_getGameObj_Children( gameObject *parent,
								   std::vector<gameObject*> &total_children){
	const std::set<gameObject*> children = parent->getAllChildren();

	for (gameObject *child : children) {
		total_children.push_back(child); //first store such child in total_children
		//then look at its children:
		internal_getGameObj_Children(child, total_children);
	}
}


std::vector<mesh*> scene::get_scene_Meshes() {
	std::vector<gameObject*> sceneGOs = get_scene_GameObjects();
	std::vector<mesh*> sceneMeshes;

	for (gameObject *go : sceneGOs) {
		if (go->m_mesh) {
			sceneMeshes.push_back(go->m_mesh);
		}
	}//end foreach

	return sceneMeshes;
}


std::vector<gameObject*> scene::get_scene_GameObjects() {
	std::vector<gameObject*> allGameObjects;

	internal_getGameObj_Children(root_gameObj, allGameObjects);
	return allGameObjects;
}


void scene::update(float dt) {
	root_gameObj->updateComponents(dt); //update components on all gameObjects

	invoke_Updates_On_GameObjs(dt, root_gameObj); //update all gameObjects.

	//after all the objects were moved, and their model matrices were recomputed,
	//we can update all the scene light shadowmaps:
	for (light* _light : scene_lights) {
		_light->updateShadowmaps(dt);
	}
}


void scene::invoke_Updates_On_GameObjs(float dt, gameObject *gameObj) {
	gameObj->update(dt);

	for (gameObject *child : gameObj->children) {
		invoke_Updates_On_GameObjs(dt, child);
	}
}


void scene::init(bool (*custom_init_function)() ) {
	custom_init_function();
}