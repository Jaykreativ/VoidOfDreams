#include "Layers/Game.h"

#include "InputHandler.h"

glm::vec3 InputHandler::getMoveDir() {
	return m_moveDir;
}

void InputHandlerClient::takeInput(Controls& controls, bool isDisabled) {
	glm::vec3 oldDir = m_moveDir;
	m_moveDir = { 0, 0, 0 };
	m_rotMat = glm::mat4(1);

	if (!isDisabled) {
		// move
		if (ImGui::IsKeyDown(controls.moveForward)) {
			m_moveDir += glm::vec3(0, 0, 1);
		}
		if (ImGui::IsKeyDown(controls.moveBackward)) {
			m_moveDir += -glm::vec3(0, 0, 1);
		}
		if (ImGui::IsKeyDown(controls.moveLeft)) {
			m_moveDir += -glm::vec3(1, 0, 0);
		}
		if (ImGui::IsKeyDown(controls.moveRight)) {
			m_moveDir += glm::vec3(1, 0, 0);
		}
		if (ImGui::IsKeyDown(controls.moveUp)) {
			m_moveDir += glm::vec3(0, 1, 0);
		}
		if (ImGui::IsKeyDown(controls.moveDown)) {
			m_moveDir += -glm::vec3(0, 1, 0);
		}
		if (m_moveDir != glm::vec3(0, 0, 0))
			m_moveDir = glm::normalize(m_moveDir);

		//rotate
		glm::vec2 mouseDelta = ImGui::GetIO().MouseDelta;

		m_rotMat = glm::rotate(m_rotMat, mouseDelta.x / 100.f, { 0, 1, 0 });
		m_rotMat = glm::rotate(m_rotMat, mouseDelta.y / 100.f, { 1, 0, 0 });

		// switch mode
		m_switchMode = ImGui::IsMouseClicked(controls.switchMode);

	}
	// detect changes
	m_changedMoveDir = oldDir != m_moveDir;
}

bool InputHandlerClient::hasSwitchedMode() {
	return m_switchMode;
}

bool InputHandlerClient::hasMoveDirChanged() {
	return m_changedMoveDir;
}

glm::mat4 InputHandlerClient::getRotationDeltaMat() {
	return m_rotMat;
}