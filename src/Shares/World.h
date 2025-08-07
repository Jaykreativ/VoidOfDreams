#pragma once

#include "Objects/Weapons/Ray.h"

#include "Zap/Zap.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>

class PlayerServer;
class PlayerClient;
class Animation;

enum WorldStatus {
	eGAME = 0x1,
	eMAIN_MENU = 0x2
};

struct WorldDataServer {
	std::shared_ptr<Zap::Scene> spScene;
	std::unordered_map<Zap::UUID, std::shared_ptr<PlayerServer>> players = {};
};

struct MainMenuWorldData {
	std::shared_ptr<Zap::Scene> spScene;
	std::shared_ptr<PlayerClient> spPlayer;
};

struct GameWorldData {
	std::shared_ptr<Zap::Scene> spScene;
	std::vector<std::shared_ptr<PlayerClient>> players = {};

	std::vector<std::unique_ptr<Ray::Beam>> rayBeams = {};
};

struct WorldDataClient {
	std::mutex mScene;
	WorldStatus status = eMAIN_MENU; // start in main menu

	std::weak_ptr<Zap::Scene> wpScene;

	std::vector<std::weak_ptr<Animation>> animations;

	std::mutex mPlayer;
	std::weak_ptr<PlayerClient> wpPlayer;

	GameWorldData game = {};
	MainMenuWorldData mainMenu = {};
};