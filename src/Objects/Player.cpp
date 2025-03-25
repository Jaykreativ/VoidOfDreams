#include "Player.h"

#include "Layers/Game.h"

#include "glm.hpp"
#include "imgui.h"

#include <string>

Player::Player(Zap::Scene& scene, Zap::ActorLoader loader) {
	m_actor = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("Player.zac"), &scene);

	m_camera = Zap::Actor(); // creating a camera to follow player
	scene.attachActor(m_camera);
	m_camera.addTransform(glm::mat4(1));
	m_camera.addCamera();
	//m_camera.cmpCamera_lookAtCenter(); // make the camera orbit the player
	updateCamera();
}

Player::~Player() {

}


void Player::updateCamera() {
	glm::mat4 offset = glm::mat4(1);
	offset = glm::translate(offset, {0, 0, -5});
	m_camera.cmpCamera_setOffset(offset);
	m_camera.cmpTransform_setTransform(m_actor.cmpTransform_getTransform());
}

void Player::updateAnimations(float dt) {
	m_actor.cmpTransform_rotate(15 * dt, {2, 3, 5});
}

void Player::updateInputs(Keybinds& keybinds, float dt) {
	glm::vec3 pos = m_actor.cmpTransform_getPos();
	glm::vec3 xDir = glm::normalize(m_actor.cmpTransform_getTransform()[0]);
	glm::vec3 yDir = glm::normalize(m_actor.cmpTransform_getTransform()[1]);
	glm::vec3 zDir = glm::normalize(m_actor.cmpTransform_getTransform()[2]);

	if (ImGui::IsKeyDown(keybinds.moveForward)) {
		pos += zDir * dt;
	}
	if (ImGui::IsKeyDown(keybinds.moveBackward)) {
		pos += -zDir * dt;
	}
	if (ImGui::IsKeyDown(keybinds.moveLeft)) {
		pos += -xDir * dt;
	}
	if (ImGui::IsKeyDown(keybinds.moveRight)) {
		pos += xDir * dt;
	}
	if (ImGui::IsKeyDown(keybinds.moveUp)) {
		pos += yDir * dt;
	}
	if (ImGui::IsKeyDown(keybinds.moveDown)) {
		pos += -yDir * dt;
	}

	m_actor.cmpTransform_setPos(pos);

	updateCamera();
}

Zap::Actor Player::getCamera() {
	return m_camera;
}