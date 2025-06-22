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

	float timerHeight = 0;
	glm::vec2 timerMid = { 0, 0 };
	glm::vec2 timerOffset = { 0, 0 };

	// animations
	float lastHealth = 0;
	float lastEnergy = 0;
};

struct GuiData {
	//font
	vk::Sampler fontSampler;
	ImFontAtlas fontAtlas;
	Zap::Image fontImage;
	ImFont* textFont;
	ImFont* headerFont;

	HudData hud;

	glm::vec2 statsOffsetRelative = { 1, 0.4 };
	glm::vec2 statsOffsetUpperRight = { -10, 0 };
	glm::vec2 statsSize = { 100, 70 };
	float statsAlpha = 0.2;
};