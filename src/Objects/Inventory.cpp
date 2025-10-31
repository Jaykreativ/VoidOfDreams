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

InventorySlot::InventorySlot(uint32_t index)
	: m_index(index)
{}

std::shared_ptr<InventorySlot> InventorySlot::getNeighbour(uint32_t index) {
	return m_neighbours[index];
}

void InventorySlot::addNeighbour(std::shared_ptr<InventorySlot> neighbour, uint32_t neighbourIndex) {
	m_neighbours[neighbourIndex] = neighbour;
}

PlayerInventory::PlayerInventory() {
	uint32_t currentIndex = 0;
	m_rootSlot = std::make_shared<InventorySlot>(currentIndex++);
	for (uint32_t i = 0; i < 5; i++) { // connect to root
		auto neighbour = std::make_shared<InventorySlot>(currentIndex++);
		neighbour->addNeighbour(m_rootSlot, 0);
		m_rootSlot->addNeighbour(neighbour, i);
	}
	for (uint32_t i = 0; i < 5; i++) { 
		auto slot = m_rootSlot->getNeighbour(i);
		slot->addNeighbour(m_rootSlot->getNeighbour((i-1)%5), 1);// interconnect first row
		slot->addNeighbour(m_rootSlot->getNeighbour((i+1)%5), 4);

		auto neighbour = std::make_shared<InventorySlot>(currentIndex++);
		neighbour->addNeighbour(slot, 0);
		slot->addNeighbour(neighbour, 2); // generate second row
	}
	for (uint32_t i = 0; i < 5; i++) {
		auto firstSlot = m_rootSlot->getNeighbour(i);
		auto secondSlot = firstSlot->getNeighbour(2);
		firstSlot->addNeighbour(m_rootSlot->getNeighbour((i+1)%5)->getNeighbour(2), 3); // connect first and second row
		secondSlot->addNeighbour(m_rootSlot->getNeighbour((i-1)%5), 1); // connect second and first row

		secondSlot->addNeighbour(m_rootSlot->getNeighbour((i - 1) % 5)->getNeighbour(2), 2); // interconnect second row
		secondSlot->addNeighbour(m_rootSlot->getNeighbour((i - 1) % 5)->getNeighbour(2), 4);
	}
	auto top = std::make_shared<InventorySlot>(currentIndex++); // generate top
	for (uint32_t i = 0; i < 5; i++) {
		auto firstSlot = m_rootSlot->getNeighbour(i);
		auto secondSlot = firstSlot->getNeighbour(2);

		secondSlot->addNeighbour(top, 3); // interconnect top
		top->addNeighbour(secondSlot, i);
	}
}