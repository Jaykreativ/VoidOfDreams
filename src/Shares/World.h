#pragma once

#include "Objects/Weapons/Ray.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"
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

	std::vector<BeamEffectRPC> rayBeamRPCs = {}; // TODO create a dedicated effect system
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

inline void setupWorldServer(WorldDataServer& world) {
	Zap::ActorLoader loader;
	loader.flags |= Zap::ActorLoader::eReuseActor;
	loader.load("Actors/Cube.zac", world.spScene.get());
}
inline void setupWorldClient(WorldDataClient& world) {
	Zap::ActorLoader loader;
	loader.flags |= Zap::ActorLoader::eReuseActor;
	loader.load("Actors/Cube.zac", world.game.spScene.get());
	loader.load("Actors/Light.zac", world.game.spScene.get());  // Loading actor from file, they can be changed using the editor
	loader.load("Actors/Light2.zac", world.game.spScene.get()); // All actors can be changed at runtime
}