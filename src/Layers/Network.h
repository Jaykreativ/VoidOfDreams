#pragma once

#include "Shares/NetworkData.h"

void runClient(NetworkData network);

void terminateClient();

// starts the server thread
// takes a copy of the network data, this cannot be changed while the server is running, needs a restart
void runServer(NetworkData network);

void terminateServer();