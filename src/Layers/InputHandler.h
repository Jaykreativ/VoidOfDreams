#pragma once

#include "Shares/Controls.h"

#include "glm.hpp"

class InputHandler {
public:
	glm::vec3 getMoveDir();

protected:
	glm::vec3 m_moveDir;
};

class InputHandlerServer : public InputHandler {

};

class InputHandlerClient : public InputHandler {
public:
	void takeInput(Controls& controls, bool isDisabled = false);

	bool hasSwitchedMode();

	bool hasMoveDirChanged();

	glm::mat4 getRotationDeltaMat();

private:
	bool m_switchMode = false;
	bool m_changedMoveDir = false;
	glm::mat4 m_rotMat = glm::mat4(1);
};