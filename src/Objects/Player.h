#pragma once

#include "Shares/Controls.h"
#include "Objects/Inventory.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

class Player {
public:
	Player(Zap::Scene& scene, Zap::ActorLoader loader = Zap::ActorLoader());
	~Player();

	void updateAnimations(float dt);

	void updateInputs(Controls& controls, float dt);

	void update(Controls& controls);

	PlayerInventory& getInventory();

	Zap::Actor getCamera();

	void setTransform(glm::mat4 transform);

	glm::mat4 getTransform();

private:
	Zap::Actor m_base; // this is the actual transform of the player
	Zap::Actor m_core; // the bright core in the centre
	Zap::Actor m_hull; // the rotating hull
	Zap::Actor m_camera;

	PlayerInventory m_inventory;

	void updateCamera(Controls& controls);
};