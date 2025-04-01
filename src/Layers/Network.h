#pragma once

#include "Shares/NetworkData.h"
#include "Shares/World.h"

void runClient(NetworkData network, WorldData& world);

void terminateClient(NetworkData network);

// starts the server thread
// takes a copy of the network data, this cannot be changed while the server is running, needs a restart
void runServer(NetworkData network);

void terminateServer();