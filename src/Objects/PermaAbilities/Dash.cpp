#include "Dash.h"

void Dash::update(Player& player, PlayerInventory::iterator iterator) {
	if (m_isTriggered) {
		m_isTriggered = false; // one time trigger
		printf("DASH!!\n");
	}
}

bool Dash::isTriggerable() {
	return true;
}
