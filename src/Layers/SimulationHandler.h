#pragma once

#include "Shares/World.h"

class SimulationHandler {
public:
	virtual void simulate(float dt, WorldDataClient& world);
};

class SimulationHandlerServer : public SimulationHandler {
public:
	void simulate(float dt, WorldDataClient& world);
};

class SimulationHandlerClient : public SimulationHandler {
public:
	void simulate(float dt, WorldDataClient& world, Controls& controls, InputHandlerClient& input);
};