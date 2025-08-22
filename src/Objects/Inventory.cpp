#include "Inventory.h"

#include "Item.h"

void PlayerInventory::update(Player& player, const InputState& input) {
	for (iterator it = begin(); it != end(); it++) {
		if (!(*it))
			continue;
		(*it)->update(player, it, input);
	}
}