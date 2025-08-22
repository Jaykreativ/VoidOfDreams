#pragma once

#include "Objects/Item.h"

class Dash : public PermaAbility{
	void update(Player& player, PlayerInventory::iterator iterator, const InputState& input) override;

	bool isTriggerable() override;
};