#pragma once

#include "Objects/Item.h"

#include "imgui.h"

class SimpleTrigger : public Trigger {
public:
	SimpleTrigger(ImGuiKey key);

	void update(Player& player, PlayerInventory::iterator iterator, const InputState& input, WorldDataServer& world) override;

	bool isTriggerable() override;

private:
	bool m_lastTriggered = false;
	ImGuiKey m_key;
};