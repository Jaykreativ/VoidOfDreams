#pragma once

#include "Objects/Item.h"

class Dash : public PermaAbility{
	void update(Player& player, PlayerInventory::iterator iterator, const InputState& input, WorldDataServer& world) override;

	bool isTriggerable() override;
};