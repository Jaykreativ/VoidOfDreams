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
	enum State {
		eNONE = 0x0,
		eGAME = 0x1,
		ePAUSE = 0x2,
		eSETTINGS = 0x4,
		eHOST = 0x8,
		eMATCHMAKING = 0x10
	} state = ePAUSE;

	//font
	vk::Sampler fontSampler;
	ImFontAtlas fontAtlas;
	Zap::Image fontImage;
	ImFont* textFont;
	ImFont* headerFont;

	HudData hud;

	std::vector<std::string> errorMessages = {};

	glm::vec2 statsOffsetRelative = { 1, 0.4 };
	glm::vec2 statsOffsetUpperRight = { -10, 0 };
	glm::vec2 statsSize = { 100, 70 };
	float statsAlpha = 0.2;

	glm::vec2 pauseMidRelative = { 0.5, 0.5 };
	glm::vec2 pauseSize = { 0, 0 };
	float pauseRoundingRelative = 0.05;
	glm::vec2 pausePaddingRelative = { 0.1, 0.08 };
	glm::vec2 pauseButtonSize = { 320, 64 };
	float pauseOuterAlpha = 0.2;
	float pauseAlpha = 0.5;

	int matchmakingSelectedRoomIndex = -1;
};