#pragma once

#include "Shares/NetworkData.h"
#include "Shares/World.h"

namespace client {
	bool isRunning();

	// the world.mPlayers mutex must be locked
	void sendPlayerMove(Player& player);

	// the world.mPlayers mutex must be locked
	void sendRay(glm::vec3 origin, glm::vec3 direction, std::string username);

	// the world.mPlayers mutex must be locked
	void sendDamage(float damage, Player& player, std::string username);
}

// runs the client networking
// returns false on failure
// terminates the client on failure
bool runClient(NetworkData& network, WorldData& world);

void terminateClient(NetworkData& network, WorldData& world);

namespace server {
	bool isRunning();
}

// starts the server thread
// takes a copy of the network data, this cannot be changed while the server is running, needs a restart
void runServer(NetworkData network);

void terminateServer();