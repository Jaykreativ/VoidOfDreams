#include "SimpleTrigger.h"

#include "Objects/Player.h"

SimpleTrigger::SimpleTrigger(ImGuiKey key)
	: m_key(key)
{}

void SimpleTrigger::update(Player& player, PlayerInventory::iterator iterator, const InputState& input) {
	bool trigger = false; input.isKeyDown(m_key);

	// simple triggers only trigger on press
	if ((m_lastTriggered != trigger) && trigger) {
		if((*(iterator - 1))->isTriggerable())
			(*(iterator - 1))->trigger();
	}
	m_lastTriggered = trigger;
}

bool SimpleTrigger::isTriggerable() {
	return false;
}
