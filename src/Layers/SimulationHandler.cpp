#include "SimulationHandler.h"

#include "Objects/Player.h"

void SimulationHandler::simulate(float dt, WorldDataClient& world) {
	for (auto spPlayer : world.game.players) {
		spPlayer->update(dt);
	}
}

void SimulationHandlerServer::simulate(float dt, WorldDataClient& world) {
	SimulationHandler::simulate(dt, world);

}

void SimulationHandlerClient::simulate(float dt, WorldDataClient& world, Controls& controls, InputHandlerClient& input) {
	SimulationHandler::simulate(dt, world);
	if (std::shared_ptr<PlayerClient> spPlayer = world.wpPlayer.lock()) {
		spPlayer->updateFocused(dt, controls, input);
		spPlayer->updateMechanics(controls, dt);
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
}