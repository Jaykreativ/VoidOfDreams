#include "Game.h"

#include "Log.h"
#include "Layers/Network.h"
#include "Shares/NetworkData.h"
#include "Shares/Render.h"
#include "Shares/World.h"
#include "Objects/Inventory.h"
#include "Objects/Weapons/Ray.h"
#include "Objects/PermaAbilities/Dash.h"
#include "Objects/Triggers/SimpleTrigger.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"

#include "ImGui.h"

#include <chrono>

void drawNetworkInterface(NetworkData& network, WorldData& world) {
	bool serverRunning = server::isRunning();
	bool clientRunning = client::isRunning();

	if (serverRunning || clientRunning)
		ImGui::BeginDisabled();
	static char usernameBuf[50] = "";
	memcpy(usernameBuf, network.username.data(), std::min<int>(50, network.username.size()));
	ImGui::InputText("username", usernameBuf, 50);
	network.username = usernameBuf;
	
	static char ipBuf[50] = "";
	memcpy(ipBuf, network.ip.data(), std::min<int>(16, network.ip.size()));
	ImGui::InputText("ip", ipBuf, 50);
	network.ip = ipBuf;

	static char portBuf[6] = "";
	memcpy(portBuf, network.port.data(), std::min<int>(6, network.port.size()));
	ImGui::InputText("port", portBuf, 6);
	network.port = portBuf;
	if (serverRunning || clientRunning)
		ImGui::EndDisabled();

	if (serverRunning) {
		if (ImGui::Button("Stop Server")) {
			terminateServer();
		}
	}
	else {
		if (ImGui::Button("Start Server")) {
			runServer(network);
		}
	}
	
	if (clientRunning) {
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

void updateMainMenu(WorldData& world, Controls& controls, float dt, Zap::Window& window) {
	static bool captured = false;

	logger::beginRegion("players");
	{
		if (captured) {
			world.mainMenu.spPlayer->updateInputs(controls, dt);
		}
		world.mainMenu.spPlayer->updateAnimations(dt);
		world.mainMenu.spPlayer->update(controls, dt);
	}
	logger::endRegion();

	logger::beginRegion("gui");
	bool wasCaptured = captured;
	if (wasCaptured)
		ImGui::BeginDisabled();

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

	ImGui::Begin("Frame Profile");
	logger::drawFrameProfileImGui();
	ImGui::End();

	logger::endRegion();

	if (wasCaptured)
		ImGui::EndDisabled();

}

void update(WorldData& world, NetworkData& network, Controls& controls, float dt, Zap::Window& window) {
	updateMainMenu(world, controls, dt, window);
	return;
	static bool captured = false;

	logger::beginRegion("players");
	{
		std::lock_guard<std::mutex> lk(world.mPlayers);
		if (captured) if (std::shared_ptr<Player> spPlayer = world.wpPlayer.lock()) {
			spPlayer->updateInputs(controls, dt);
		}
		for (auto spPlayerPair : world.game.players) {
			spPlayerPair.second->updateAnimations(dt);
			spPlayerPair.second->update(controls, dt);
		} 
		client::sendPlayerMove(network, world);
	}
	logger::endRegion();

	logger::beginRegion("gui");
	bool wasCaptured = captured;
	if (wasCaptured)
		ImGui::BeginDisabled();

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

	drawNetworkInterface(network, world);

	ImGui::Begin("Frame Profile");
	logger::drawFrameProfileImGui();
	ImGui::End();

	//ImGui::Begin("Timeline");
	//logger::drawTimelineImGui();
	//ImGui::End();
	logger::endRegion();

	if (wasCaptured)
		ImGui::EndDisabled();
}

void gameLoop(RenderData& render, WorldData& world, NetworkData& network, Controls& controls) {
	float deltaTime = 0;
	while (!render.window->shouldClose()) {
		logger::beginFrame();
		logger::beginRegion("loop"); // define regions for profiling
		auto startFrame = std::chrono::high_resolution_clock::now();

		logger::beginRegion("update");
		update(world, network, controls, deltaTime, *render.window);

		{
			if (std::shared_ptr<Zap::Scene> spScene = world.wpScene.lock()) { // update scene only if present
				std::lock_guard<std::mutex> lk(world.mPlayers);
				if (std::shared_ptr<Player> spPlayer = world.wpPlayer.lock()) { // enable rendering only if player is selected
					logger::beginRegion("engine");
					render.pbRender->updateCamera(spPlayer->getCamera());
					render.pbRender->enable();
					spScene->update();
					logger::endRegion();
					logger::beginRegion("simulation");
					spScene->simulate(deltaTime);
					logger::endRegion();
				}
				else {
					render.pbRender->disable();
					ImGui::GetBackgroundDrawList()->AddRectFilled({ 0, 0 }, ImGui::GetMainViewport()->Size, ImGui::GetColorU32(render.pbRender->clearColor)); // improvised clear
				}
			}
		}
		logger::endRegion();

		logger::beginRegion("render");
		render.renderer->render();
		logger::endRegion();

		logger::beginRegion("present");
		render.window->present();
		logger::endRegion();
		render.window->pollEvents();

		auto endFrame = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(endFrame - startFrame).count();
		logger::endRegion();
		//logger::cleanTimeline();
		logger::endFrame();
	}
}

void setupLocalPlayer(WorldData& world, std::string username) {
	world.game.players[username] = std::make_shared<Player>(*world.game.spScene, username);
	world.wpPlayer = world.game.players.at(username);
	if (std::shared_ptr<Player> spPlayer = world.wpPlayer.lock()) {
		spPlayer->getInventory().setItem(std::make_shared<Ray>(world), 0);
		spPlayer->getInventory().setItem(std::make_shared<SimpleTrigger>(ImGuiMouseButton_Left), 1);
		spPlayer->getInventory().setItem(std::make_shared<Dash>(), 2);
		spPlayer->getInventory().setItem(std::make_shared<SimpleTrigger>(ImGuiKey_LeftShift), 3);
	}
}

void setupExternalPlayer(WorldData& world, std::string username) {
	world.game.players[username] = std::make_shared<Player>(*world.game.spScene, username);
}

void setupMainMenuWorld(WorldData& world) {
	Zap::ActorLoader loader;
	loader.flags |= Zap::ActorLoader::eReuseActor;
	loader.load("Actors/Light.zac", world.mainMenu.spScene.get());
	loader.load("Actors/Cube.zac", world.mainMenu.spScene.get());
	world.mainMenu.spPlayer = std::make_shared<Player>(*world.mainMenu.spScene.get(), "user", loader);
	world.mainMenu.spPlayer->setTransform(glm::mat4(1));
	world.wpPlayer = world.mainMenu.spPlayer;
}

void setupWorld(WorldData& world) {
	Zap::ActorLoader loader;
	loader.flags |= Zap::ActorLoader::eReuseActor;
	loader.load((std::string)"Actors/Light.zac", world.game.spScene.get());  // Loading actor from file, they can be changed using the editor
	loader.load((std::string)"Actors/Light2.zac", world.game.spScene.get()); // All actors can be changed at runtime
	loader.load((std::string)"Actors/Cube.zac", world.game.spScene.get());
}

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

	Zap::SceneDesc desc{};
	desc.gravity = { 0, 0, 0 };
	world.game.spScene = std::make_shared<Zap::Scene>();
	world.game.spScene->init(desc);
	world.mainMenu.spScene = std::make_shared<Zap::Scene>();
	world.mainMenu.spScene->init(desc);
	world.wpScene = world.mainMenu.spScene;

	render.renderer = new Zap::Renderer();
	render.pbRender = new Zap::PBRenderer(world.mainMenu.spScene.get());
	render.pGui = new Zap::Gui();

	setupWorld(world);
	setupMainMenuWorld(world);

	render.renderer->setTarget(render.window);
	render.renderer->addRenderTask(render.pbRender);
	render.renderer->addRenderTask(render.pGui);
	render.pbRender->setViewport(render.window->getWidth(), render.window->getHeight(), 0, 0); // initial viewport
	render.pbRender->clearColor = { .1, .1, .1, 1 }; // grey background
	render.window->getResizeEventHandler()->addCallback(resize, render.pbRender); // give the window access to the pbRender task to resize its viewport
	render.pGui->initImGui(render.window);

#ifdef _DEBUG
	static bool areShadersCompiled = false;
	if (!areShadersCompiled) {
		vk::Shader::compile("Zap/Shader/src/", { "PBRShader.vert", "PBRShader.frag" }, { "./" });
		areShadersCompiled = true;
	}
#endif

	render.renderer->init();
	render.renderer->beginRecord(); // define the actual rendering structure
	render.renderer->recRenderTemplate(render.pbRender);
	render.renderer->recRenderTemplate(render.pGui);
	render.renderer->endRecord();

	render.window->show();

	gameLoop(render, world, network, controls);

	terminateClient(network, world); // terminate networking if still running
	terminateServer();

	render.renderer->destroy();
	delete render.renderer;
	render.pGui->destroyImGui();
	delete render.pGui;
	delete render.pbRender;
	render.window->getResizeEventHandler()->removeCallback(resize, render.pbRender);
	delete render.window;
	world.game.spScene->destroy();
	world.mainMenu.spScene->destroy();
}