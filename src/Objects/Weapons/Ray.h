#pragma once

#include "Objects/Item.h"

class Ray : public Weapon{
	void update(Player& player, PlayerInventory::iterator iterator) override;
};