#include "Dash.h"

#include "Objects/Player.h"

const float _energyCost = 30;
const float _strength = 50;

void Dash::update(Player& player, PlayerInventory::iterator iterator, const InputState& input, WorldDataServer& world) {
	if (m_isTriggered && (player.getEnergy() >= _energyCost)) {
		auto rotMat = input.getRotationMat(); // get rotation
		player.getPhysicsActor().cmpRigidDynamic_addForce((rotMat * glm::vec4(input.getMoveDir(), 0))*_strength);
		player.spendEnergy(_energyCost);
	}
	m_isTriggered = false; // one time trigger
}

bool Dash::isTriggerable() {
	return true;
}
