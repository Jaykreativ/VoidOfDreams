#pragma once

#include "Objects/Item.h"

#include "imgui.h"

class SimpleTrigger : public Trigger {
public:
	SimpleTrigger(ImGuiKey key);
	SimpleTrigger(ImGuiMouseButton_ mouseButton);

	void update(Player& player, PlayerInventory::iterator iterator) override;

	bool isTriggerable() override;

private:
	bool m_isKey;
	ImGuiKey m_key;
	ImGuiMouseButton m_mouseButton;
};