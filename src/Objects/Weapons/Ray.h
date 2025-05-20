#pragma once

#include "Objects/Item.h"
#include "Shares/World.h"

class Ray : public Weapon{
public:
	Ray(WorldData& world);

	void update(Player& player, PlayerInventory::iterator iterator) override;

private:
	WorldData& m_world;
};