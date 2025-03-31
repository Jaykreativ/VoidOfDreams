#pragma once

#include "Objects/Player.h"

#include "Zap/Zap.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

#include <unordered_map>
#include <string>
#include <memory>

struct WorldData {
	Zap::Scene* scene;
	std::weak_ptr<Player> pPlayer;
	std::unordered_map<std::string, std::shared_ptr<Player>> players = {};
};