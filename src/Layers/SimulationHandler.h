#pragma once

#include "Layers/InputHandler.h"

#include "Shares/World.h"

class SimulationHandler {
public:

};

struct ClientProxy;
class SimulationHandlerServer : public SimulationHandler {
public:
	void simulate(float dt, WorldDataServer& world, std::unordered_map<Zap::UUID, ClientProxy>& clients);
};

class SimulationHandlerClient : public SimulationHandler {
public:
	void simulate(float dt, WorldDataClient& world, Controls& controls, InputHandlerClient& input);
};