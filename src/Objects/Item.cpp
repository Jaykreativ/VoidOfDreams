#include "Item.h"

void Item::trigger() {
	m_isTriggered = true;
}

bool Item::isTriggerable() {
	return false;
}

bool Weapon::isTriggerable() {
	return true;
}

bool Ability::isTriggerable() {
	return true;
}