#pragma once

#include "Objects/Item.h"

class SimpleTrigger : public Trigger {
	void update(Player& player, PlayerInventory::iterator iterator) override;

	bool isTriggerable() override;
};