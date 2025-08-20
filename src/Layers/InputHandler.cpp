#include "Layers/Game.h"

#include "InputHandler.h"

#include <chrono>

bool InputState::hasSwitchedMode() const {
	return m_switchMode;
}

glm::vec3 InputState::getMoveDir() const {
	return m_moveDir;
}

glm::mat4 InputState::getRotationMat() const {
	return m_rotMat;
}

void InputState::pack(char*& buf) {
	memcpy(buf, &m_switchMode, sizeof(m_switchMode)); buf += sizeof(m_switchMode);
	memcpy(buf, &m_moveDir, sizeof(m_moveDir)); buf += sizeof(m_moveDir);
	memcpy(buf, &m_rotMat, sizeof(m_rotMat)); buf += sizeof(m_rotMat);
}

void InputState::unpack(const char*& buf) {
	memcpy(&m_switchMode, buf, sizeof(m_switchMode)); buf += sizeof(m_switchMode);
	memcpy(&m_moveDir, buf, sizeof(m_moveDir)); buf += sizeof(m_moveDir);
	memcpy(&m_rotMat, buf, sizeof(m_rotMat)); buf += sizeof(m_rotMat);
}

void Action::pack(char*& buf) {
	m_inputState.pack(buf);
	memcpy(buf, &m_timestamp, sizeof(m_timestamp)); buf += sizeof(m_timestamp);
	memcpy(buf, &m_deltaTime, sizeof(m_deltaTime)); buf += sizeof(m_deltaTime);
}

void Action::unpack(const char*& buf) {
	m_inputState.unpack(buf);
	memcpy(&m_timestamp, buf, sizeof(m_timestamp)); buf += sizeof(m_timestamp);
	memcpy(&m_deltaTime, buf, sizeof(m_deltaTime)); buf += sizeof(m_deltaTime);
}

const Action& ActionList::addAction(const InputState& inputState, double timestamp) {
	float dTime = m_lastTimestamp >= 0.f ?
		timestamp - m_lastTimestamp : 0.f;

	m_list.push_back(Action(inputState, timestamp, dTime));
	m_lastTimestamp = timestamp;
	return m_list.back();
}

size_t ActionList::size() {
	return m_list.size();
}

void ActionList::clear() {
	m_list.clear();
}

std::vector<Action>::iterator ActionList::begin() {
	return m_list.begin();
}

std::vector<Action>::iterator ActionList::end() {
	return m_list.end();
}

size_t ActionList::dataSize() {
	return sizeof(uint32_t) + sizeof(Action) * m_list.size();
}

void ActionList::pack(char*& buf) {
	uint32_t size = m_list.size();
	memcpy(buf, &size, sizeof(size)); buf += sizeof(size);
	for (auto& action : m_list) {
		action.pack(buf);
	}
}

void ActionList::unpack(const char*& buf) {
	uint32_t size = 0;
	memcpy(&size, buf, sizeof(size)); buf += sizeof(size);
	m_list.resize(size);
	for (auto& action : m_list) {
		action.unpack(buf);
	}
}

ActionList& InputHandler::getActions() {
	return m_list;
}

void InputHandler::reset() {
	m_list.clear();
}

void InputHandlerServer::takeActions(const ActionList& actionList) {
	m_list = actionList;
}

void InputHandlerClient::takeInput(Controls& controls, bool isDisabled) {
	InputState newState;
	if (!isDisabled) {
		// move
		if (ImGui::IsKeyDown(controls.moveForward)) {
			newState.m_moveDir += glm::vec3(0, 0, 1);
		}
		if (ImGui::IsKeyDown(controls.moveBackward)) {
			newState.m_moveDir += -glm::vec3(0, 0, 1);
		}
		if (ImGui::IsKeyDown(controls.moveLeft)) {
			newState.m_moveDir += -glm::vec3(1, 0, 0);
		}
		if (ImGui::IsKeyDown(controls.moveRight)) {
			newState.m_moveDir += glm::vec3(1, 0, 0);
		}
		if (ImGui::IsKeyDown(controls.moveUp)) {
			newState.m_moveDir += glm::vec3(0, 1, 0);
		}
		if (ImGui::IsKeyDown(controls.moveDown)) {
			newState.m_moveDir += -glm::vec3(0, 1, 0);
		}
		if (newState.m_moveDir != glm::vec3(0, 0, 0))
			newState.m_moveDir = glm::normalize(newState.m_moveDir);

		//rotate
		glm::vec2 mouseDelta = ImGui::GetIO().MouseDelta;

		m_currentRotMat = glm::rotate(m_currentRotMat, mouseDelta.x / 100.f, { 0, 1, 0 });
		m_currentRotMat = glm::rotate(m_currentRotMat, mouseDelta.y / 100.f, { 1, 0, 0 });

		// switch mode
		newState.m_switchMode = ImGui::IsMouseClicked(controls.switchMode);
	}
	newState.m_rotMat = m_currentRotMat; // set rotation
	m_state = newState;
}

void InputHandlerClient::pushAction() {
	double timestamp = std::chrono::time_point_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
	m_list.addAction(m_state, timestamp);
}

const InputState& InputHandlerClient::getInput() {
	return m_state;
}