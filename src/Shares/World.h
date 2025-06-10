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

struct WorldData {
	Zap::Scene* scene;

	std::vector<std::weak_ptr<Animation>> animations;

	std::mutex mPlayers;
	std::weak_ptr<Player> pPlayer;
	std::unordered_map<std::string, std::shared_ptr<Player>> players = {};

	std::vector<std::unique_ptr<Ray::Beam>> rayBeams = {};
};