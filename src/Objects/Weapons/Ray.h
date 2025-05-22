#pragma once

#include "Objects/Item.h"
#include "Shares/World.h"

class Ray : public Weapon{
public:
	Ray(WorldData& world);

	void update(Player& player, PlayerInventory::iterator iterator) override;

	static void processRay(glm::vec3 origin, glm::vec3 direction, WorldData& world, Player& checkPlayer, Player& senderPlayer);

private:
	WorldData& m_world;
};