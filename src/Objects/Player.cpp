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
	m_hull.cmpRigidDynamic_setAngularDamping(.5);
	m_hull.cmpRigidDynamic_setLinearDamping(.9);

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
		offset = glm::translate(offset, zDir*1.6f); // offset camera to the front in first person
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
	m_core.cmpTransform_rotate(-90 * dt, {2, 3, 5});

	auto v = m_hull.cmpRigidDynamic_getLinearVelocity();
	m_hull.cmpRigidDynamic_addTorque(v*0.0001f);
}

void Player::updateInputs(Controls& controls, float dt) {
	glm::mat4 transform = m_base.cmpTransform_getTransform();
	glm::vec3 xDir = glm::normalize(transform[0]);
	glm::vec3 yDir = glm::normalize(transform[1]);
	glm::vec3 zDir = glm::normalize(transform[2]);

	// move
	float speed = 25;
	m_movementDir = { 0, 0, 0 };
	if (ImGui::IsKeyDown(controls.moveForward)) {
		m_movementDir += zDir;
	}
	if (ImGui::IsKeyDown(controls.moveBackward)) {
		m_movementDir += -zDir;
	}
	if (ImGui::IsKeyDown(controls.moveLeft)) {
		m_movementDir += -xDir;
	}
	if (ImGui::IsKeyDown(controls.moveRight)) {
		m_movementDir += xDir;
	}
	if (ImGui::IsKeyDown(controls.moveUp)) {
		m_movementDir += yDir;
	}
	if (ImGui::IsKeyDown(controls.moveDown)) {
		m_movementDir += -yDir;
	}
	if (m_movementDir != glm::vec3(0, 0, 0))
		m_movementDir = glm::normalize(m_movementDir);

	//rotate
	glm::vec2 mouseDelta = ImGui::GetIO().MouseDelta;

	glm::mat4 rotMat(1);
	rotMat = glm::rotate(rotMat, mouseDelta.x / 100.f, {0, 1, 0});
	rotMat = glm::rotate(rotMat, mouseDelta.y/100.f, {1, 0, 0});

	m_base.cmpTransform_setTransform(transform * rotMat);
	m_hull.cmpRigidDynamic_addForce(m_movementDir * dt * speed);
}

void Player::update(Controls& controls) {
	glm::vec3 pos = m_hull.cmpTransform_getPos(); // hull determines the position
	m_core.cmpTransform_setPos(pos);
	m_base.cmpTransform_setPos(pos);

	updateCamera(controls);

	m_inventory.update(*this); // update all items in inventory
}

PlayerInventory& Player::getInventory() {
	return m_inventory;
}

Zap::Actor Player::getCamera() {
	return m_camera;
}

Zap::Actor Player::getPhysicsActor() {
	return m_hull;
}

glm::mat4 Player::getCameraTransform() {
	return m_camera.cmpTransform_getTransform();
}

glm::vec3 Player::getMovementDirection() {
	return m_movementDir;
}

void Player::setTransform(glm::mat4 transform) {
	m_hull.cmpTransform_setTransform(transform);
	m_hull.cmpRigidDynamic_updatePose();
}

glm::mat4 Player::getTransform() {
	return m_hull.cmpTransform_getTransform();
}