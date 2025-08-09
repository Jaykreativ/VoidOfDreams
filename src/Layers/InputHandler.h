#pragma once

#include "Shares/Controls.h"

#include "glm.hpp"

class InputState {
public:
	bool hasSwitchedMode() const;

	glm::vec3 getMoveDir() const;

	glm::mat4 getRotationDeltaMat() const;

	// network interface
	void pack(char*& buf);

	void unpack(const char*& buf);

private:
	bool m_switchMode = false;

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

	const InputState& GetInputState() const { return m_inputState; }
	float GetTimestamp() const { return m_timestamp; }
	float GetDeltaTime() const { return m_deltaTime; }

	// network interface
	void pack(char*& buf);

	void unpack(const char*& buf);

private:
	InputState m_inputState;
	float m_timestamp;
	float m_deltaTime;
};

class ActionList {
public:
	const Action& addAction(const InputState& inputState, float timestamp);

	// network interface
	size_t dataSize();

	void pack(char*& buf);

	void unpack(const char*& buf);

private:
	std::vector<Action> m_list;
	float m_lastTimestamp = 0;
};

class InputHandler {
public:
	ActionList& getActions();

protected:
	ActionList m_list;
};

class InputHandlerServer : public InputHandler {
public:

};

class InputHandlerClient : public InputHandler {
public:
	void takeInput(Controls& controls, bool isDisabled = false);

	void pushAction();

	const InputState& getInput();

private:
	InputState m_state;
};