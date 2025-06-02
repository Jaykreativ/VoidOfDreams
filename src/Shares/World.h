#pragma once

#include "Objects/Player.h"

#include "Zap/Zap.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>

struct MainMenuWorldData {
	std::shared_ptr<Zap::Scene> spScene;
	std::shared_ptr<Player> spPlayer;
};

struct GameWorldData {
	std::shared_ptr<Zap::Scene> spScene;
	std::unordered_map<std::string, std::shared_ptr<Player>> players = {};
};

struct WorldData {
	std::weak_ptr<Zap::Scene> wpScene;

	std::mutex mPlayers;
	std::weak_ptr<Player> wpPlayer;

	GameWorldData game = {};
	MainMenuWorldData mainMenu = {};
};