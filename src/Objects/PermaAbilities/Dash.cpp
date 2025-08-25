#include "Dash.h"

#include "Objects/Player.h"

const float _energyCost = 30;

void Dash::update(Player& player, PlayerInventory::iterator iterator, const InputState& input, WorldDataServer& world) {
	if (m_isTriggered && (player.getEnergy() >= _energyCost)) {
		//auto transform = player.getCameraTransform();
		//player.getPhysicsActor().cmpRigidDynamic_addForce(player.getMovementDirection()*50.f);
		//player.spendEnergy(_energyCost);
		printf("Dash\n");
	}
	m_isTriggered = false; // one time trigger
}

bool Dash::isTriggerable() {
	return true;
}
