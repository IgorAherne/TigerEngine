#pragma once
#include <GLEW/glew.h> //those 2 must
#include <GLFW\glfw3.h> //come before any other includes, AND in this order.
#include <iostream>
#include "../gfxmath/timer.h"
#include "../gfx/renderer.h"
#include "../gfx/framework.h"
#include "../gfx/input.h"
#include "../gfx/scene.h"
#include "../gfxmath/timer.h"

#include "../gfx/gameObject.h"
#include "../gfx/camera.h"
#include "../gfx/repositories.h"
#include "../gfx/mesh.h"
#include "../gfx/pointLight.h"
#include "../gfx/basic_navigator.h"
#include "../gfx/basic_rotator.h"
#include "../gfx/basic_spinner.h"

#include "../gfx/pointLightGI.h"
#include "../gfx/material.h"



bool myFirst_scene_init();

int main() {

	//initialize glfw:
	framework::init();

	renderer *myRenderer = renderer::createRenderer("Tiger Engine 0.1", 1900, 1300);
	//renderer *my2Renderer = renderer::createRenderer("my2Window", 700, 600,
	//																	myRenderer);

	timer t1;

	scene *firstScene = scene::getCurrentScene();
	firstScene->init(myFirst_scene_init);

	input::updateInput(0);

	//main loop:
	while (true) {
		float dt = t1.dt();

		input::updateInput(dt);

		firstScene->update(dt);

		if (!renderer::renderAllrenderers(dt)) {
			break;
		}

		input::flush_keys();
		std::cout << "\n\nfps:  " << 1.0f / dt;

		if (1.0f / dt < 0.001 && t1.elapsed() > 10000) {
			break;
		}
	}//end while

	//terminate everything:
	framework::terminate();

	return 0;
}



bool myFirst_scene_init() {

	//TODO redo, since every scene will have same set of instructions.
	//perhaps take as an argument a function pointer.


	std::vector<renderer*> renderers;
	renderers.push_back(renderer::getRenderer(0));


	gameObject *firstCamera = new gameObject("camera");


	key x_mov[2]{ key::KEY_A, key::KEY_D };
	key y_mov[2]{ key::KEY_Q, key::KEY_E };
	key z_mov[2]{ key::KEY_S, key::KEY_W };

	firstCamera->attachComponent<basic_navigator>(
		new basic_navigator(x_mov, y_mov, z_mov,
			vec2(5, 3))
		);
	firstCamera->getTransform()->setPosition(vec3(0, 0, 0.f));
	//firstCamera->getTransform()->setPitchYawRoll(vec3(90, 0, 0));
	camera *cam = new camera(vec2(0.2, 1000), renderers);
	cam->setFov(90.f);
	//TODO set fov was not working. (throws nullref exeption at frustum::reShape() )
	firstCamera->attachComponent<camera>(cam);


	key x_mov1[2]{ key::KEY_LEFT, key::KEY_RIGHT };
	key y_mov1[2]{ key::KEY_HOME, key::KEY_END };
	key z_mov1[2]{ key::KEY_DOWN, key::KEY_UP };

	gameObject *test_geom = new gameObject("cube_test_geom_tool");
	test_geom->attachComponent<mesh>(repositories::getMesh("demo_geom"));
	//test_geom->attachComponent<mesh>(repositories::getMesh("cube_room_no_letters"));
	test_geom->getTransform()->setPitchYawRoll(vec3(0, 0, 0));
	test_geom->getMaterial()->set_Color(color(0.8, 0.8f, 0.8f, 1.0f));
	//test_geom->attachComponent<basic_navigator>(
	//						new basic_navigator(x_mov1, y_mov1, z_mov1, vec2(3,0)));
	//test_geom->attachComponent<basic_rotator>(
	//					new basic_rotator(x_mov1, y_mov1, z_mov1, 42));

	gameObject *igor_aherne_text = new gameObject("igor_aherne_text");
	igor_aherne_text->attachComponent<mesh>(repositories::getMesh("igor_aherne_text"));
	igor_aherne_text->getTransform()->setPitchYawRoll(vec3(0, 0, 0));
	igor_aherne_text->getMaterial()->set_emissive(10);
	igor_aherne_text->getMaterial()->set_Color(color(0.01, 0.6f, 0.8f, 1.0f));


	gameObject *orange_sphere = new gameObject("orange_sphere");
	//orange_sphere->attachComponent<mesh>(repositories::getMesh("geosphere"));
	//orange_sphere->attachComponent<mesh>(mesh::createQuad());
	orange_sphere->attachComponent<mesh>(repositories::getMesh("geosphere"));
	orange_sphere->getMaterial()->set_Color(color(0.9, 0.6, 0.1, 1.0f));
	orange_sphere->getMaterial()->set_emissive(10);
	orange_sphere->getTransform()->setPosition(vec3(0, 0, -3.f));
	orange_sphere->getTransform()->setScale(vec3(3, 3, 3));
	orange_sphere->attachComponent<basic_navigator>(
					new basic_navigator(x_mov1, y_mov1, z_mov1, vec2(3, 0)));
	//orange_sphere->attachComponent<basic_rotator>(
	//				new basic_rotator(x_mov1, y_mov1, z_mov1, 42) );

	gameObject *green_sphere = new gameObject("green_sphere");
	green_sphere->attachComponent<mesh>(repositories::getMesh("offset_geosphere"));
	green_sphere->getMaterial()->set_Color(color(0.2, 1, 0.2, 1.0f));
	green_sphere->getMaterial()->set_emissive(20);
	green_sphere->getTransform()->setPosition(vec3(6, 10, 11.f));
	green_sphere->getTransform()->setScale(vec3(2, 2, 2));
	green_sphere->attachComponent<basic_spinner>(new basic_spinner(vec3(0, 11, 0), 0));
	
	gameObject *orange_sphere2 = new gameObject("pink sphere");
	orange_sphere2->attachComponent<mesh>(repositories::getMesh("geosphere"));
	orange_sphere2->getMaterial()->set_Color(color(1, 0.1, 1, 1.0f));
	orange_sphere2->getMaterial()->set_emissive(20);
	orange_sphere2->getTransform()->setPosition(vec3(15.5, 8, 7.5f));
	orange_sphere2->getTransform()->setScale(vec3(2, 2, 2));
	
	gameObject *blue_emissive_sphere = new gameObject("blue emsssive sphere");
	blue_emissive_sphere->attachComponent<mesh>(repositories::getMesh("geosphere"));
	blue_emissive_sphere->getMaterial()->set_Color(color(0.1, 0.5, 0.9, 1.0f));
	blue_emissive_sphere->getMaterial()->set_emissive(20);
	blue_emissive_sphere->getTransform()->setPosition(vec3(-5, 13.5, 12.f));
	blue_emissive_sphere->getTransform()->setScale(vec3(4,4, 4));
	
	//gameObject *blue_sphere = new gameObject("blue_sphere");
	//blue_sphere->attachComponent<mesh>(repositories::getMesh("geosphere"));
	//blue_sphere->getMaterial()->set_Color(color(0.1, 0.1f, 0.95, 1.0f));
	//blue_sphere->getTransform()->setPosition(vec3(4, -13, +3.f));
	//blue_sphere->getTransform()->setScale(vec3(2.f, 2, 2.f));

	//gameObject *lurker = new gameObject("lurker_monster");
	//lurker->attachComponent<mesh>(repositories::getMesh("lurker_monster"));
	//lurker->getMaterial()->set_Color(color(0.8, 0.8f, 0.8f, 1.0f));
	//lurker->getTransform()->setPosition(vec3(-4, -13.5, -6.f));
	//lurker->getTransform()->setPitchYawRoll(vec3(0,160, 0));

	//gameObject *torus_knot = new gameObject("torus_knot");
	//torus_knot->attachComponent<mesh>(repositories::getMesh("torus_knot"));
	//torus_knot->getMaterial()->set_Color(color(0.8, 0.8f, 0.8f, 1.0f));
	//torus_knot->getTransform()->setPosition(vec3(-6.5f, -9.5, 6.5f));

	//gameObject *red_wall = new gameObject("red_wall");
	//red_wall->attachComponent<mesh>(mesh::createQuad());
	//red_wall->getMaterial()->set_Color(color(1, 0.1,0.1, 1.0f));
	//red_wall->getTransform()->setPosition(vec3(0.f, 0, -13.f));
	////red_wall->getTransform()->setPitchYawRoll(vec3(45, 45, 45));
	//red_wall->getTransform()->setScale(vec3(25, 25, 25));
	
	
	//gameObject *yellow_wall = new gameObject("yellow_wall");
	//yellow_wall->attachComponent<mesh>(mesh::createQuad());
	//yellow_wall->getMaterial()->set_Color(color(1, 1, 0.1, 1.0f));
	//yellow_wall->getTransform()->setPosition(vec3(-13.f, 0, 0.f));
	//yellow_wall->getTransform()->setPitchYawRoll(vec3(0, 90, 0));
	//yellow_wall->getTransform()->setScale(vec3(23.f, 23, 23.f));
	//
	//gameObject *purple_wall = new gameObject("purple_wall");
	//purple_wall->attachComponent<mesh>(mesh::createQuad());
	//purple_wall->getMaterial()->set_Color(color(0.9, 0.2, 0.9f, 1.0f));
	//purple_wall->getTransform()->setPosition(vec3(0.2f, 13, 0.2f));
	//purple_wall->getTransform()->setPitchYawRoll(vec3(90, 0, 0));
	//purple_wall->getTransform()->setScale(vec3(23.f, 23, 23.f));


	key x_mov2[2]{ key::KEY_J, key::KEY_L };
	key y_mov2[2]{ key::KEY_U, key::KEY_O };
	key z_mov2[2]{ key::KEY_K, key::KEY_I };

	gameObject *my_light = new gameObject("light");
	my_light->attachComponent<pointLight>(new pointLight(1024, vec2(1, 40)));
	my_light->attachComponent<basic_navigator>(
						new basic_navigator(x_mov2, y_mov2, z_mov2, vec2(3, 0)));
	my_light->getTransform()->setPosition(vec3(3,0, 7));
	my_light->getComponent<pointLight>()->set_intensity(0.001);
	return true;
}



//READ:
//https://people.mpi-inf.mpg.de/~ritschel/Papers/ScreenSpaceBentCones.pdf