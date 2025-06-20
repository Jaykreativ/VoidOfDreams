#pragma once

#include "Objects/Animation.h"

#include "imgui.h"

#include "glm.hpp"

#include <memory>
#include <vector>

struct HudData {
	float scale = 1;
	std::vector<std::unique_ptr<Animation>> animations = {};

	glm::vec2 crosshairMiddle = { 0, 0 };
	float crosshairSize = 0;
	float crosshairThickness = 0;
	ImU32 crosshairColor = 0xFFFFFFFF;

	float statBarSize = 0;
	glm::vec2 statBarOffset = { 0, 0 };
	glm::vec2 healthMid = { 0, 0 };
	glm::vec2 energyMid = { 0, 0 };
	glm::vec2 statBarHalfExtents = { 0, 0 };
	ImU32 healthColor = 0xFFFFFFFF;
	ImU32 energyColor = 0xFFFFFFFF;

	// animations
	float lastHealth = 0;
	float lastEnergy = 0;
};