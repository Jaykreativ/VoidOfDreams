#pragma once

#include "Objects/Item.h"

class Dash : public PermaAbility{
	void update(Player& player, PlayerInventory::iterator iterator) override;

	bool isTriggerable() override;

	const float _energyCost = 30;
};