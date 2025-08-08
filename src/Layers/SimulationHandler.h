#pragma once

#include "Layers/InputHandler.h"

#include "Shares/World.h"

class SimulationHandler {
public:

};

class SimulationHandlerServer : public SimulationHandler {
public:
	void simulate(float dt, WorldDataServer& world);
};

class SimulationHandlerClient : public SimulationHandler {
public:
	void simulate(float dt, WorldDataClient& world, Controls& controls, InputHandlerClient& input);
};