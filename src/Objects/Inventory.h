#pragma once

#include <memory>
#include <array>
#include <iterator>
#include <cstddef>

#define PLAYER_INVENTORY_SIZE 12

class Item;
class Player;

template<size_t _size>
class Inventory {
public:
	using iterator = typename std::array<std::shared_ptr<Item>, _size>::iterator ;

	void resetItem(size_t index);

	void setItem(std::shared_ptr<Item> spItem, size_t index);

	iterator begin();

	iterator end();

	size_t size();

private:
	std::array<std::shared_ptr<Item>, _size> m_items = {};
};

class PlayerInventory : public Inventory<PLAYER_INVENTORY_SIZE> {
public:
	void update(Player& player);
};