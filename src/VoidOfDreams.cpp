#include "Zap/Zap.h"
#include "Zap/Rendering/Window.h"
#include "Zap/Rendering/Renderer.h"
#include "Zap/Rendering/PBRenderer.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"
#include "Zap/Scene/Transform.h"
#include "Zap/Scene/Material.h"
#include "Zap/FileLoader.h"

namespace app {
	Zap::Window* window;
	Zap::Scene* scene;
	Zap::Renderer* renderer;
	Zap::PBRenderer* pbRender;
	Zap::Actor actor;
	Zap::Actor camera;
}

void resize(Zap::ResizeEvent& eventParams, void* customParams) {
	app::pbRender->setViewport(eventParams.width, eventParams.height, 0, 0);
}

void setupActors() {
	Zap::ActorLoader loader;
	app::actor = loader.load((std::string)"Actors/Player.zac", app::scene); // Loading actor from file, they can be changed using the editor
	loader.load((std::string)"Actors/Light.zac", app::scene);                   // All actors can be changed at runtime
	loader.load((std::string)"Actors/Light2.zac", app::scene);
	loader.load((std::string)"Actors/Cube.zac", app::scene);

	app::camera = Zap::Actor(); // Creating new actor at runtime, this cannot be used by the editor
	app::scene->attachActor(app::camera);
	app::camera.addTransform(glm::mat4(1));
	app::camera.cmpTransform_setPos(0, 0, 7.5);
	app::camera.cmpTransform_rotateY(180);
	app::camera.addCamera();
}

void main() {
	auto base = Zap::Base::createBase("Void of Dreams", "VOD.zal"); // you need to give the engine access to your asset library
	base->init();
	
	app::window = new Zap::Window(1000, 600, "Void of Dreams");
	app::window->init();
	app::window->getResizeEventHandler()->addCallback(resize);

	app::scene = new Zap::Scene();

	app::renderer = new Zap::Renderer();
	app::pbRender = new Zap::PBRenderer(app::scene);
	
	setupActors();

	app::scene->init();

	app::renderer->setTarget(app::window);
	app::renderer->addRenderTask(app::pbRender);
	app::pbRender->setViewport(app::window->getWidth(), app::window->getHeight(), 0, 0);
	app::pbRender->clearColor = {.1, .1, .1, 1};

	app::renderer->init();
	app::renderer->beginRecord();
	app::renderer->recRenderTemplate(app::pbRender);
	app::renderer->endRecord();

	app::window->show();
	float deltaTime = 0;
	while (!app::window->shouldClose()) {
		auto startFrame = std::chrono::high_resolution_clock::now();

		app::actor.cmpTransform_rotate(25 * deltaTime, {1, 1, 1});

		app::pbRender->updateCamera(app::camera);
		app::scene->update();
		app::renderer->render();

		app::window->present();
		app::window->pollEvents();

		auto endFrame = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(endFrame - startFrame).count();
	}

	app::renderer->destroy();
	delete app::renderer;
	app::scene->destroy();
	delete app::scene;
	app::window->getResizeEventHandler()->removeCallback(resize);
	delete app::window;

	base->terminate();
	Zap::Base::releaseBase();

	system("pause");
}