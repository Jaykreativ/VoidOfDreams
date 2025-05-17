#include "Inventory.h"

#include "Item.h"

template<size_t _size>
void Inventory<_size>::resetItem(size_t index) {
	m_items[index].reset();
}

template<size_t _size>
void Inventory<_size>::setItem(std::shared_ptr<Item> spItem, size_t index) {
	m_items[index] = spItem;
}

template<size_t _size>
typename Inventory<_size>::iterator Inventory<_size>::begin() {
	return m_items.begin();
}

template<size_t _size>
typename Inventory<_size>::iterator Inventory<_size>::end() {
	return m_items.end();
}

template<size_t _size>
size_t Inventory<_size>::size() {
	return _size;
}

void PlayerInventory::update(Player& player) {
	for (iterator it = begin(); it != end(); it++) {
		if (!(*it))
			continue;
		(*it)->update(player, it);
	}
}