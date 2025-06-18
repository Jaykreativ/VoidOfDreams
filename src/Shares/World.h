#pragma once

#include "Objects/Player.h"
#include "Objects/Animation.h"
#include "Objects/Weapons/Ray.h"

#include "Zap/Zap.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>

enum WorldStatus {
	eGAME = 0x1,
	eMAIN_MENU = 0x2
};

struct MainMenuWorldData {
	std::shared_ptr<Zap::Scene> spScene;
	std::shared_ptr<Player> spPlayer;
};

struct GameWorldData {
	std::shared_ptr<Zap::Scene> spScene;
	std::unordered_map<std::string, std::shared_ptr<Player>> players = {};
};

struct WorldData {
	std::mutex mScene;
	Zap::Scene* scene;
	WorldStatus status = eMAIN_MENU; // start in main menu

	std::weak_ptr<Zap::Scene> wpScene;

	std::vector<std::weak_ptr<Animation>> animations;

	std::mutex mPlayers;
	std::weak_ptr<Player> pPlayer;
	std::unordered_map<std::string, std::shared_ptr<Player>> players = {};

	std::vector<std::unique_ptr<Ray::Beam>> rayBeams = {};
	std::weak_ptr<Player> wpPlayer;

	GameWorldData game = {};
	MainMenuWorldData mainMenu = {};
};