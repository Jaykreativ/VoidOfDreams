#include "Game.h"

#include "Log.h"
#include "Layers/Network.h"
#include "Shares/NetworkData.h"
#include "Shares/Render.h"
#include "Shares/World.h"
#include "Shares/Hud.h"
#include "Objects/Inventory.h"
#include "Objects/Weapons/Ray.h"
#include "Objects/PermaAbilities/Dash.h"
#include "Objects/Triggers/SimpleTrigger.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"

#include "ImGui.h"

#include <chrono>

static HudData _hud = {};

class HudDamageAnimation : public Animation {
public:
	HudDamageAnimation(float minHealth, float maxHealth, float fullHealth)
		: Animation(.5f), m_minHealth(minHealth), m_maxHealth(maxHealth), m_fullHealth(fullHealth)
	{}

	void update(float dt) override {
		auto* draw = ImGui::GetWindowDrawList();
		glm::vec2 min = _hud.healthMid - _hud.statBarHalfExtents + glm::vec2(0, (1 - m_maxHealth / m_fullHealth) * _hud.statBarSize);
		glm::vec2 max = _hud.healthMid + _hud.statBarHalfExtents - glm::vec2(0, m_minHealth / m_fullHealth * _hud.statBarSize);
		glm::vec2 mid = (min + max) / 2.f;
		glm::vec2 extents = max - mid;
		draw->AddRectFilled(
			mid - extents * (1 - timeFactor()),
			mid + extents * (1 - timeFactor()),
			_hud.healthColor);
		addDeltaTime(dt);
	}

private:
	float m_minHealth;
	float m_maxHealth;
	float m_fullHealth;
};

class HudEnergyAnimation : public Animation {
public:
	HudEnergyAnimation(float minEnergy, float maxEnergy, float fullEnergy)
		: Animation(.25f), m_minEnergy(minEnergy), m_maxEnergy(maxEnergy), m_fullEnergy(fullEnergy)
	{}

	void update(float dt) override {
		auto* draw = ImGui::GetWindowDrawList();
		glm::vec2 min = _hud.energyMid - _hud.statBarHalfExtents + glm::vec2(0, (1 - m_maxEnergy / m_fullEnergy) * _hud.statBarSize);
		glm::vec2 max = _hud.energyMid + _hud.statBarHalfExtents - glm::vec2(0, m_minEnergy / m_fullEnergy * _hud.statBarSize);
		glm::vec2 mid = (min + max) / 2.f;
		glm::vec2 extents = max - mid;
		draw->AddRectFilled(
			mid - extents * (1 - timeFactor()),
			mid + extents * (1 - timeFactor()),
			_hud.energyColor);
		addDeltaTime(dt);
	}

private:
	float m_minEnergy;
	float m_maxEnergy;
	float m_fullEnergy;
};

void drawHud(Player& player, float dt) {
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::SetNextWindowPos({0, 0});
	ImGui::Begin("Hud", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing);
	auto* draw = ImGui::GetWindowDrawList();

	// crosshair
	{
		_hud.crosshairMiddle = glm::vec2(ImGui::GetWindowSize()) / 2.f;
		_hud.crosshairSize = 25* _hud.scale;
		_hud.crosshairThickness = 2* _hud.scale;
		_hud.crosshairColor = ImGui::GetColorU32({1, 1, 1, 1});
		glm::vec2 xVec = glm::vec2(_hud.crosshairSize, _hud.crosshairThickness) / 2.f;
		draw->AddRectFilled(_hud.crosshairMiddle -xVec, _hud.crosshairMiddle +xVec, _hud.crosshairColor);
		glm::vec2 yVec = glm::vec2(_hud.crosshairThickness, _hud.crosshairSize) / 2.f;
		draw->AddRectFilled(_hud.crosshairMiddle -yVec, _hud.crosshairMiddle +yVec, _hud.crosshairColor);
	}

	// health + energy bar
	{
		_hud.statBarSize = 100 * _hud.scale;
		_hud.statBarOffset = glm::vec2(ImGui::GetWindowSize().x - _hud.statBarSize * 1.1, _hud.statBarSize * 0.1);
		_hud.healthMid = _hud.statBarOffset + glm::vec2(_hud.statBarSize / 4.f, _hud.statBarSize / 2.f);
		_hud.energyMid = _hud.statBarOffset + glm::vec2(_hud.statBarSize *3.f/4.f, _hud.statBarSize /2.f);
		_hud.statBarHalfExtents = glm::vec2(_hud.statBarSize /2.5f, _hud.statBarSize) / 2.f;
		auto* font = ImGui::GetFont();

		// health bar
		_hud.healthColor = ImGui::GetColorU32({ 0.9, 0.1, 0.1, 1 });
		draw->AddRectFilled(_hud.healthMid - _hud.statBarHalfExtents +glm::vec2(0, (1-player.getHealth() / player.getMaxHealth())* _hud.statBarSize), _hud.healthMid + _hud.statBarHalfExtents, _hud.healthColor);
		std::string strHealth = std::to_string((uint32_t)std::clamp<float>(std::ceil(player.getHealth()), 0, player.getMaxHealth()));
		float healthTextX = font->CalcTextSizeA(font->FontSize, _hud.statBarSize / 2.f, 0, strHealth.c_str(), strHealth.c_str() + strHealth.size()).x;
		draw->AddText(glm::vec2(_hud.healthMid.x - healthTextX / 2.f, _hud.statBarOffset.y + _hud.statBarSize), 0xFFFFFFFF, strHealth.c_str(), strHealth.c_str() + strHealth.size());

		// energy bar
		_hud.energyColor = ImGui::GetColorU32({ 0.9, 0.9, 0.9, 1 });
		draw->AddRectFilled(_hud.energyMid - _hud.statBarHalfExtents +glm::vec2(0, (1-player.getEnergy() / player.getMaxEnergy())* _hud.statBarSize), _hud.energyMid + _hud.statBarHalfExtents, _hud.energyColor);
		std::string strEnergy = std::to_string((uint32_t)std::clamp<float>(std::ceil(player.getEnergy()), 0, player.getMaxEnergy()));
		float energyTextX = font->CalcTextSizeA(font->FontSize, _hud.statBarSize / 2.f, 0, strEnergy.c_str(), strEnergy.c_str() + strEnergy.size()).x;
		draw->AddText(glm::vec2(_hud.energyMid.x - energyTextX / 2.f, _hud.statBarOffset.y + _hud.statBarSize), 0xFFFFFFFF, strEnergy.c_str(), strEnergy.c_str() + strEnergy.size());
	}

	if (_hud.lastHealth > player.getHealth())
		_hud.animations.push_back(std::make_unique<HudDamageAnimation>(player.getHealth(), _hud.lastHealth, player.getMaxHealth()));
	_hud.lastHealth = player.getHealth();

	if (_hud.lastEnergy > player.getEnergy())
		_hud.animations.push_back(std::make_unique<HudEnergyAnimation>(player.getEnergy(), _hud.lastEnergy, player.getMaxEnergy()));
	_hud.lastEnergy = player.getEnergy();

	for (size_t i = 0; i < _hud.animations.size(); i++) {
		if (_hud.animations[i]->isDone()) {
			_hud.animations.erase(_hud.animations.begin() + i);
			i--;
		}
		else
			_hud.animations[i]->update(dt);
	}

	//{
	//	float size = 100*hudScale;
	//	glm::vec2 offset = {ImGui::GetWindowSize().x - size*1.1, size*0.1};
	//	glm::vec2 points[] = {
	//		(glm::vec2(sin(glm::pi<float>() * 0 / 5), cos(glm::pi<float>() * 0 / 5)) - 1.f) / -2.f,
	//		(glm::vec2(sin(glm::pi<float>() * 2 / 5), cos(glm::pi<float>() * 2 / 5)) - 1.f) / -2.f,
	//		(glm::vec2(sin(glm::pi<float>() * 4 / 5), cos(glm::pi<float>() * 4 / 5)) - 1.f) / -2.f,
	//		(glm::vec2(sin(glm::pi<float>() * 6 / 5), cos(glm::pi<float>() * 6 / 5)) - 1.f) / -2.f,
	//		(glm::vec2(sin(glm::pi<float>() * 8 / 5), cos(glm::pi<float>() * 8 / 5)) - 1.f) / -2.f
	//	};
	//	for (size_t i = 0; i < 5; i++)
	//		points[i] = points[i] * size + offset;
	//	draw->AddConvexPolyFilled(reinterpret_cast<ImVec2*>(points), 5, 0xFFFFFFFF);
	//}

	ImGui::End();
}

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
	memcpy(ipBuf, network.ip.data(), std::min<int>(50, network.ip.size()));
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

void update(WorldData& world, NetworkData& network, Controls& controls, float dt, Zap::Window& window) {
	static bool captured = false;

	logger::beginRegion("players");
	{
		std::lock_guard<std::mutex> lk(world.mPlayers);
		if (std::shared_ptr<Player> spPlayer = world.pPlayer.lock()) {
			ImGui::Text("Kills: %lu", spPlayer->getKills());
			ImGui::Text("Deaths: %lu", spPlayer->getDeaths());
			ImGui::Text("Damage: %f", spPlayer->getDamage());
			spPlayer->updateMechanics(controls, dt);
			if (captured)
				spPlayer->updateInputs(controls, dt);
			if (ImGui::IsKeyPressed(ImGuiKey_R)) {
				spPlayer->kill();
				spPlayer->spawn();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_K))
				spPlayer->kill();
			drawHud(*spPlayer, dt);
		}
		for (auto spPlayerPair : world.players) {
			spPlayerPair.second->updateAnimations(dt);
			spPlayerPair.second->update(dt);
		}
	}
	logger::endRegion();

	logger::beginRegion("animations");
	for (size_t i = 0; i < world.animations.size(); i++) {
		if (auto spAnimation = world.animations[i].lock()) {
			spAnimation->update(dt);
		}
		else {
			world.animations.erase(world.animations.begin() + i);
			i--;
		}
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
			std::lock_guard<std::mutex> lk(world.mPlayers);
			if (std::shared_ptr<Player> spPlayer = world.pPlayer.lock()) { // enable rendering only if player is selected
				logger::beginRegion("engine");
				render.pbRender->updateCamera(spPlayer->getCamera());
				render.pbRender->enable();
				world.scene->update();
				logger::endRegion();
				logger::beginRegion("simulation");
				world.scene->simulate(deltaTime);
				logger::endRegion();
			}
			else {
				render.pbRender->disable();
				ImGui::GetBackgroundDrawList()->AddRectFilled({ 0, 0 }, ImGui::GetMainViewport()->Size, ImGui::GetColorU32(render.pbRender->clearColor)); // improvised clear
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
	world.players[username] = std::make_shared<Player>(*world.scene, username);
	world.pPlayer = world.players.at(username);
	if (std::shared_ptr<Player> spPlayer = world.pPlayer.lock()) {
		spPlayer->getInventory().setItem(std::make_shared<Ray>(world), 0);
		spPlayer->getInventory().setItem(std::make_shared<SimpleTrigger>(ImGuiMouseButton_Left), 1);
		spPlayer->getInventory().setItem(std::make_shared<Dash>(), 2);
		spPlayer->getInventory().setItem(std::make_shared<SimpleTrigger>(ImGuiKey_LeftShift), 3);
	}
}

void setupExternalPlayer(WorldData& world, std::string username) {
	world.players[username] = std::make_shared<Player>(*world.scene, username);
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

	Zap::SceneDesc desc{};
	desc.gravity = { 0, 0, 0 };
	world.scene = new Zap::Scene();
	world.scene->init(desc);

	render.renderer = new Zap::Renderer();
	render.pbRender = new Zap::PBRenderer(world.scene);
	render.pGui = new Zap::Gui();

	setupWorld(world);

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
	world.rayBeams.clear();

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