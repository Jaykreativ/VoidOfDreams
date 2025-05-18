#include "SimpleTrigger.h"

SimpleTrigger::SimpleTrigger(ImGuiKey key)
	: m_isKey(true), m_key(key), m_mouseButton(0)
{}

SimpleTrigger::SimpleTrigger(ImGuiMouseButton_ mouseButton)
	: m_isKey(false), m_mouseButton(mouseButton), m_key(ImGuiKey_None)
{}

void SimpleTrigger::update(Player& player, PlayerInventory::iterator iterator) {
	bool trigger = false;
	if (m_isKey) {
		if (ImGui::IsKeyPressed(m_key, false)) {
			trigger = true;
		}
	}
	else {
		if (ImGui::IsMouseClicked(m_mouseButton, false)) {
			trigger = true;
		}
	}

	if(trigger && (*(iterator - 1))->isTriggerable())
		(*(iterator - 1))->trigger();
}

bool SimpleTrigger::isTriggerable() {
	return false;
}
