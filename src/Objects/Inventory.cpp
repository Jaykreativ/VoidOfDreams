#include "Inventory.h"

#include "Item.h"
#include "Layers/InputHandler.h"

void PlayerInventory::update(Player& player, const InputState& input, WorldDataServer& world) {
	for (iterator it = begin(); it != end(); it++) {
		if (!(*it))
			continue;
		(*it)->update(player, it, input, world);
	}
}