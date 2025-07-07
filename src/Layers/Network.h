#pragma once

#include "Shares/NetworkData.h"
#include "Shares/Render.h"
#include "Shares/World.h"
#include "Shares/GuiData.h"

namespace client {
	bool isRunning();

	// the world.mPlayers mutex must be locked
	void sendPlayerMove(Player& player);

	// the world.mPlayers mutex must be locked
	void sendPlayerSpawn(std::string username);

	// the world.mPlayers mutex must be locked
	void sendPlayerDeath(std::string username, std::string usernameKiller);

	// the world.mPlayers mutex must be locked
	void sendPlayerDamage(float damage, float health, std::string username, std::string usernameDamager);

	// the world.mPlayers mutex must be locked
	void sendRay(glm::vec3 origin, glm::vec3 direction, std::string username);
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
void runServer(NetworkData& network);

void waitServerStartup();

void terminateServer();