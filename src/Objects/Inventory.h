#pragma once

#include <memory>
#include <array>
#include <iterator>
#include <cstddef>

#define PLAYER_INVENTORY_SIZE 12

class Item;
class Player;
class InputState;
struct WorldDataServer;

template<size_t _size>
class Inventory {
public:
	using iterator = typename std::array<std::shared_ptr<Item>, _size>::iterator ;

	void resetItem(size_t index) {
		m_items[index].reset();
	}

	void setItem(std::shared_ptr<Item> spItem, size_t index) {
		m_items[index] = spItem;
	}

	iterator begin() {
		return m_items.begin();
	}

	iterator end() {
		return m_items.end();
	}

	size_t size() {
		return _size;
	}

private:
	std::array<std::shared_ptr<Item>, _size> m_items = {};
};

class InventorySlot {
	friend class PlayerInventory;
public:
	InventorySlot(uint32_t index);

	std::shared_ptr<InventorySlot> getNeighbour(uint32_t index);

private:

	void addNeighbour(std::shared_ptr<InventorySlot> neighbour, uint32_t neighbourIndex);

	uint32_t m_index;
	std::array<std::shared_ptr<InventorySlot>, 5> m_neighbours;
};

class PlayerInventory : public Inventory<PLAYER_INVENTORY_SIZE> {
public:
	PlayerInventory();

	void update(Player& player, const InputState& input, WorldDataServer& world);

private:
	std::shared_ptr<InventorySlot> m_rootSlot;
};