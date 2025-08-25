#pragma once

#include "Inventory.h"


class Player;
struct WorldDataServer;
class Item {
public:
	virtual void update(Player& player, PlayerInventory::iterator iterator, const InputState& input, WorldDataServer& world) = 0;

	void trigger();

	virtual bool isTriggerable();

protected:
	bool m_isTriggered = false;
};

class Weapon : public Item {
	bool isTriggerable() override; // is always triggerable
};

class Ability : public Item {
	bool isTriggerable() override; // is always triggerable
};

class PermaAbility : public Item {

};

class Trigger : public Item {

};