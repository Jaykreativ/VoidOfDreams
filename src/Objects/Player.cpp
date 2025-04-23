#include "Layers/Game.h"

#include "glm.hpp"
#include "imgui.h"

#include "Player.h"

#include <string>

Player::Player(Zap::Scene& scene, Zap::ActorLoader loader) {
	loader.flags = loader.flags | Zap::ActorLoader::eReuseActor;
	m_base = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerBase.zac"), &scene);
	m_core = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerCore.zac"), &scene);
	m_hull = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerHull.zac"), &scene);

	m_camera = Zap::Actor(); // creating a camera to follow player
	scene.attachActor(m_camera);
	m_camera.addTransform(glm::mat4(1));
	m_camera.addCamera();
}

Player::~Player() {
	m_base.destroy();
	m_core.destroy();
	m_hull.destroy();
	m_camera.destroy();
}

void Player::updateCamera(Controls& controls) {
	glm::mat4 transform = m_base.cmpTransform_getTransform();
	glm::vec3 zDir = glm::normalize(transform[2]);

	glm::mat4 offset = glm::mat4(1);
	switch (controls.cameraMode)
	{
	case Controls::eFIRST_PERSON:
		offset = glm::translate(offset, zDir*1.5f); // offset camera to the front in first person
		break;
	case Controls::eTHIRD_PERSON:
		offset = glm::translate(offset, zDir*-5.f); // offset camera to the back in third person
		break;
	default:
		break;
	}
	m_camera.cmpCamera_setOffset(offset);
	m_camera.cmpTransform_setTransform(m_base.cmpTransform_getTransform());
}

void Player::updateAnimations(float dt) {
	m_hull.cmpTransform_rotate(15 * dt, {2, 3, 5});
	m_core.cmpTransform_rotate(-90 * dt, {2, 3, 5});
}

void Player::updateInputs(Controls& controls, float dt) {
	glm::mat4 transform = m_base.cmpTransform_getTransform();
	glm::vec3 pos = m_base.cmpTransform_getPos();
	glm::vec3 xDir = glm::normalize(transform[0]);
	glm::vec3 yDir = glm::normalize(transform[1]);
	glm::vec3 zDir = glm::normalize(transform[2]);

	// move
	float velocity = 10;
	if (ImGui::IsKeyDown(controls.moveForward)) {
		pos += zDir * dt * velocity;
	}
	if (ImGui::IsKeyDown(controls.moveBackward)) {
		pos += -zDir * dt * velocity;
	}
	if (ImGui::IsKeyDown(controls.moveLeft)) {
		pos += -xDir * dt * velocity;
	}
	if (ImGui::IsKeyDown(controls.moveRight)) {
		pos += xDir * dt * velocity;
	}
	if (ImGui::IsKeyDown(controls.moveUp)) {
		pos += yDir * dt * velocity;
	}
	if (ImGui::IsKeyDown(controls.moveDown)) {
		pos += -yDir * dt * velocity;
	}


	//rotate
	glm::vec2 mousePos = (glm::vec2)ImGui::GetMousePos();
	glm::vec2 mouseDelta = mousePos - m_lastMousePos;
	m_lastMousePos = mousePos;

	glm::mat4 rotMat(1);
	rotMat = glm::rotate(rotMat, mouseDelta.x / 100.f, {0, 1, 0});
	rotMat = glm::rotate(rotMat, mouseDelta.y/100.f, {1, 0, 0});
	ImGui::Text((std::to_string(xDir.x) + "," + std::to_string(xDir.y) + "," + std::to_string(xDir.z)).c_str());

	m_base.cmpTransform_setTransform(transform * rotMat);
	m_base.cmpTransform_setPos(pos);
}

void Player::update(Controls& controls) {
	glm::vec3 pos = m_base.cmpTransform_getPos();
	m_core.cmpTransform_setPos(pos);
	m_hull.cmpTransform_setPos(pos);

	updateCamera(controls);
}

Zap::Actor Player::getCamera() {
	return m_camera;
}

void Player::setTransform(glm::mat4 transform) {
	m_base.cmpTransform_setTransform(transform);
}

glm::mat4 Player::getTransform() {
	return m_base.cmpTransform_getTransform();
}