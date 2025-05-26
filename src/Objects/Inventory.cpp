#include "Inventory.h"

#include "Item.h"

void PlayerInventory::update(Player& player) {
	for (iterator it = begin(); it != end(); it++) {
		if (!(*it))
			continue;
		(*it)->update(player, it);
	}
}