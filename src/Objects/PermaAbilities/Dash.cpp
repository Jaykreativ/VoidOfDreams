#include "Dash.h"

#include "Objects/Player.h"

void Dash::update(Player& player, PlayerInventory::iterator iterator) {
	if (m_isTriggered) {
		m_isTriggered = false; // one time trigger
		auto transform = player.getCameraTransform();
		player.getPhysicsActor().cmpRigidDynamic_addForce(transform[2]*75.f);
	}
}

bool Dash::isTriggerable() {
	return true;
}
