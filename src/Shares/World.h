#include "Objects/Player.h"

#include "Zap/Zap.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

#include <memory>

struct WorldData {
	Zap::Scene* scene;
	std::unique_ptr<Player> pPlayer;
};