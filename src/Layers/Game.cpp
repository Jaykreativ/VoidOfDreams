#include "Game.h"

#include "Log.h"
#include "Shares/Render.h"
#include "Shares/World.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"

#include "ImGui.h"

#include <chrono>

void gameLoop(RenderData& render, WorldData& world) {
	float deltaTime = 0;
	while (!render.window->shouldClose()) {
		//logger::beginRegion("loop"); // define regions for profiling
		auto startFrame = std::chrono::high_resolution_clock::now();

		world.pPlayer->updateAnimations(deltaTime);
		world.pPlayer->updateInputs(Keybinds{}, deltaTime);

		render.pbRender->updateCamera(world.pPlayer->getCamera());
		world.scene->update();
		render.renderer->render();

		render.window->present();
		render.window->pollEvents();

		auto endFrame = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(endFrame - startFrame).count();
		//logger::endRegion();
		//logger::cleanTimeline();
	}
}

void setupWorld(WorldData& world) {
	Zap::ActorLoader loader;
	loader.load((std::string)"Actors/Light.zac", world.scene);  // Loading actor from file, they can be changed using the editor
	loader.load((std::string)"Actors/Light2.zac", world.scene); // All actors can be changed at runtime
	loader.load((std::string)"Actors/Cube.zac", world.scene);

	world.pPlayer = std::make_unique<Player>(*world.scene, loader); // create player, will be deleted when world exits scope
}

void resize(Zap::ResizeEvent& eventParams, void* customParams) {
	Zap::PBRenderer* pbRender = reinterpret_cast<Zap::PBRenderer*>(customParams);
	pbRender->setViewport(eventParams.width, eventParams.height, 0, 0);
}

void runGame() {
	RenderData render = {};
	WorldData world = {};

	render.window = new Zap::Window(1000, 600, "Void of Dreams");
	render.window->init();


	world.scene = new Zap::Scene();

	render.renderer = new Zap::Renderer();
	render.pbRender = new Zap::PBRenderer(world.scene);
	render.pGui = new Zap::Gui();

	setupWorld(world);

	world.scene->init();

	render.renderer->setTarget(render.window);
	render.renderer->addRenderTask(render.pbRender);
	render.renderer->addRenderTask(render.pGui);
	render.pbRender->setViewport(render.window->getWidth(), render.window->getHeight(), 0, 0); // initial viewport
	render.pbRender->clearColor = { .1, .1, .1, 1 }; // grey background
	render.window->getResizeEventHandler()->addCallback(resize, render.pbRender); // give the window access to the pbRender task to resize its viewport
	render.pGui->initImGui(render.window);

	static bool areShadersCompiled = false;
	if (!areShadersCompiled) {
		vk::Shader::compile("Zap/Shader/src/", { "PBRShader.vert", "PBRShader.frag" }, { "./" });
		areShadersCompiled = true;
	}

	render.renderer->init();
	render.renderer->beginRecord(); // define the actual rendering structure
	render.renderer->recRenderTemplate(render.pbRender);
	render.renderer->recRenderTemplate(render.pGui);
	render.renderer->endRecord();

	render.window->show();

	gameLoop(render, world);

	render.renderer->destroy();
	delete render.renderer;
	render.pGui->destroyImGui();
	delete render.pGui;
	delete render.pbRender;
	world.scene->destroy();
	delete world.scene;
	render.window->getResizeEventHandler()->removeCallback(resize, render.pbRender);
	delete render.window;
}