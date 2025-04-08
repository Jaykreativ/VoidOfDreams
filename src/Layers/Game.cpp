#include "Game.h"

#include "Log.h"
#include "Layers/Network.h"
#include "Shares/NetworkData.h"
#include "Shares/Render.h"
#include "Shares/World.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"

#include "ImGui.h"

#include <chrono>

void update(WorldData& world, Controls& controls, float dt, Zap::Window& window) {
	static bool captured = false;
	if (ImGui::Button("Capture")) { // use imgui to capture mouse because there is no menu implemented yet
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		captured = true;
	}
	if (captured && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		captured = false;
	}

	if (ImGui::Button("First Person"))
		controls.cameraMode = Controls::eFIRST_PERSON;
	ImGui::SameLine();
	if (ImGui::Button("Third Person"))
		controls.cameraMode = Controls::eTHIRD_PERSON;

	std::lock_guard<std::mutex> lk(world.mPlayers);
	if(captured) if (std::shared_ptr<Player> spPlayer = world.pPlayer.lock()) {
		spPlayer->updateInputs(controls, dt);
	}
	for (auto spPlayerPair : world.players) {
		spPlayerPair.second->updateAnimations(dt);
		spPlayerPair.second->update(controls);
	}
}

void drawNetworkInterface(NetworkData& network, WorldData& world) {
	if (server::isRunning() || client::isRunning())
		ImGui::BeginDisabled();
	static char usernameBuf[50] = "";
	memcpy(usernameBuf, network.username.data(), std::min<int>(50, network.username.size()));
	ImGui::InputText("username", usernameBuf, 50);
	network.username = usernameBuf;
	
	static char ipBuf[16] = "";
	memcpy(ipBuf, network.ip.data(), std::min<int>(16, network.ip.size()));
	ImGui::InputText("ip", ipBuf, 16);
	network.ip = ipBuf;

	static char portBuf[6] = "";
	memcpy(portBuf, network.port.data(), std::min<int>(6, network.port.size()));
	ImGui::InputText("port", portBuf, 6);
	network.port = portBuf;
	if (server::isRunning() || client::isRunning())
		ImGui::EndDisabled();

	if (server::isRunning()) {
		if (ImGui::Button("Stop Server")) {
			terminateServer();
		}
	}
	else {
		if (ImGui::Button("Start Server")) {
			runServer(network);
		}
	}
	
	if (client::isRunning()) {
		if (ImGui::Button("Stop Client")) {
			terminateClient(network, world);
		}
	}
	else {
		if (ImGui::Button("Start Client")) {
			runClient(network, world);
		}
	}  
}

void gameLoop(RenderData& render, WorldData& world, NetworkData& network, Controls& controls) {
	float deltaTime = 0;
	while (!render.window->shouldClose()) {
		//logger::beginRegion("loop"); // define regions for profiling
		auto startFrame = std::chrono::high_resolution_clock::now();


		update(world, controls, deltaTime, *render.window);

		{
			std::lock_guard<std::mutex> lk(world.mPlayers);
			if (std::shared_ptr<Player> spPlayer = world.pPlayer.lock()) { // enable rendering only if player is selected
				render.pbRender->updateCamera(spPlayer->getCamera());
				render.pbRender->enable();
				world.scene->update();
			}
			else {
				render.pbRender->disable();
				ImGui::GetBackgroundDrawList()->AddRectFilled({ 0, 0 }, ImGui::GetMainViewport()->Size, ImGui::GetColorU32(render.pbRender->clearColor));
			}

			client::sendPlayerMove(network, world);
		}

		drawNetworkInterface(network, world);

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
	loader.load((std::string)"Actors/Cube.zac", world.scene);}

void resize(Zap::ResizeEvent& eventParams, void* customParams) {
	Zap::PBRenderer* pbRender = reinterpret_cast<Zap::PBRenderer*>(customParams);
	pbRender->setViewport(eventParams.width, eventParams.height, 0, 0);
}

void runGame() {
	RenderData render = {};
	WorldData world = {};
	NetworkData network = {};
	Controls controls = {};

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

	gameLoop(render, world, network, controls);

	terminateClient(network, world); // terminate networking if still running
	terminateServer();

	world.players.clear();

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