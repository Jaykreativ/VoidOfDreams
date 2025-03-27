#pragma once

#include "imgui.h"

struct Controls {
	ImGuiKey moveForward = ImGuiKey_W;
	ImGuiKey moveBackward = ImGuiKey_S;
	ImGuiKey moveLeft = ImGuiKey_A;
	ImGuiKey moveRight = ImGuiKey_D;
	ImGuiKey moveUp = ImGuiKey_Space;
	ImGuiKey moveDown = ImGuiKey_C;

	float mouseVerticalSensitivity = 1;
	float mouseHorizontalSensitivity = 1;

	enum CameraMode {
		eFIRST_PERSON,
		eTHIRD_PERSON
	} cameraMode = eFIRST_PERSON;
};