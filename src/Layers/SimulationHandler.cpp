#include "SimulationHandler.h"

#include "Log.h"
#include "Layers/NetworkHandler.h"
#include "Objects/Player.h"

void SimulationHandlerServer::simulate(float dt, WorldDataServer& world, std::unordered_map<Zap::UUID, ClientProxy>& clients) {
	for (auto& playerPair : world.players) {
		auto id = playerPair.first;
		auto spPlayer = playerPair.second;
		if (clients.count(id)) {
			auto& input = clients.at(id).inputHandler;
			for (auto& action : input.getActions()) { // loop through all client actions received in the last input packet
				spPlayer->updateFocused(action.getDeltaTime(), action.getInputState());
				spPlayer->updateMechanics(action.getDeltaTime());
				spPlayer->update(dt);
			}
			input.getActions().clear();
		}
	}
	world.spScene->simulate(dt);
}

void SimulationHandlerClient::simulate(float dt, WorldDataClient& world, Controls& controls, InputHandlerClient& input) {
	if (world.status == eGAME)
		for (auto spPlayer : world.game.players) {
			spPlayer->update(dt);
		}
	if (std::shared_ptr<PlayerClient> spPlayer = world.wpPlayer.lock()) {
		spPlayer->updateFocused(dt, controls, input.getInput(), input);
		if (world.status == eGAME)
			spPlayer->updateMechanics(dt);
		else
			spPlayer->update(dt);
	}

	for (size_t i = 0; i < world.animations.size(); i++) {
		if (auto spAnimation = world.animations[i].lock()) {
			spAnimation->update(dt);
		}
		else {
			world.animations.erase(world.animations.begin() + i);
			i--;
		}
	}

	logger::beginRegion("physics");
	if (std::shared_ptr<Zap::Scene> spScene = world.wpScene.lock()) {
		spScene->simulate(dt);
	}
	logger::endRegion();
}