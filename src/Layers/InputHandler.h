#pragma once

#include "Shares/Controls.h"

#include "glm.hpp"


class InputState {
public:
	bool hasSwitchedMode() const;

	glm::vec3 getMoveDir() const;

	glm::mat4 getRotationMat() const;

	bool isKeyDown(ImGuiKey key) const;

	// network interface
	size_t dataSize();

	void pack(char*& buf);

	void unpack(const char*& buf);

private:
	bool m_switchMode = false;

	std::vector<int> m_keysPressed = {};

	glm::vec3 m_moveDir = glm::vec3(0, 0, 0);
	glm::mat4 m_rotMat = glm::mat4(1);

	friend class InputHandler;
	friend class InputHandlerServer;
	friend class InputHandlerClient;
};

class Action {
public:
	Action() {}
	Action(const InputState& inputState, float timestamp,
		float deltaTime) :
		m_inputState(inputState),
		m_timestamp(timestamp),
		m_deltaTime(deltaTime)
	{}

	const InputState& getInputState() const { return m_inputState; }
	float getTimestamp() const { return m_timestamp; }
	float getDeltaTime() const { return m_deltaTime; }

	// network interface
	size_t dataSize();

	void pack(char*& buf);

	void unpack(const char*& buf);

private:
	InputState m_inputState;
	float m_timestamp;
	float m_deltaTime;
};

class ActionList {
public:
	const Action& addAction(const InputState& inputState, double timestamp);

	size_t size();

	void clear();

	std::vector<Action>::iterator begin();

	std::vector<Action>::iterator end();

	// network interface
	size_t dataSize();

	void pack(char*& buf);

	void unpack(const char*& buf);

private:
	std::vector<Action> m_list;
	double m_lastTimestamp = 0;
};

class InputHandler {
public:
	ActionList& getActions();

	void reset();

protected:
	ActionList m_list;
};

class InputHandlerServer : public InputHandler {
public:
	void takeActions(const ActionList& actionList);
};

class InputHandlerClient : public InputHandler {
public:
	void takeInput(Controls& controls, bool isDisabled = false);

	void pushAction();

	const InputState& getInput();

private:
	glm::mat4 m_currentRotMat = glm::mat4(1);
	InputState m_state;
};