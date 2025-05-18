#include "Ray.h"

void Ray::update(Player& player, PlayerInventory::iterator iterator) {
	if (m_isTriggered) {
		m_isTriggered = false; // one time trigger
		printf("RAY!!\n");
	}
}