#include "Dash.h"

#include "Objects/Player.h"

void Dash::update(Player& player, PlayerInventory::iterator iterator) {
	if (m_isTriggered && (player.getEnergy() >= _energyCost)) {
		auto transform = player.getCameraTransform();
		player.getPhysicsActor().cmpRigidDynamic_addForce(player.getMovementDirection()*50.f);
		player.spendEnergy(_energyCost);
	}
	m_isTriggered = false; // one time trigger
}

bool Dash::isTriggerable() {
	return true;
}
