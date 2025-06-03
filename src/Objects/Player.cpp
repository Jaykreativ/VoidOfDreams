#include "Layers/Game.h"
#include "Layers/Network.h"

#include "glm.hpp"
#include "imgui.h"

#include "Player.h"

#include <string>

Player::Player(Zap::Scene& scene, std::string username, Zap::ActorLoader loader)
	: m_scene(scene), m_username(username)
{
	loader.flags = loader.flags | Zap::ActorLoader::eReuseActor;
	m_base = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerBase.zac"), &m_scene);

	m_camera = Zap::Actor(); // creating a camera to follow player
	m_scene.attachActor(m_camera);
	m_camera.addTransform(glm::mat4(1));
	m_camera.addCamera();
}

Player::~Player() {
	m_base.destroy();
	m_camera.destroy();
	if (m_active) {
		m_core.destroy();
		m_hull.destroy();
	}
}

void Player::updateCamera(Controls& controls) {
	glm::mat4 transform = m_base.cmpTransform_getTransform();
	glm::vec3 zDir = glm::normalize(transform[2]);

	glm::mat4 offset = glm::mat4(1);
	if (m_active) {
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
	}
	m_camera.cmpCamera_setOffset(offset);
	m_camera.cmpTransform_setTransform(m_base.cmpTransform_getTransform());
}

void Player::updateAnimations(float dt) {
	if (m_active) {
		m_core.cmpTransform_rotate(-90 * dt, { 2, 3, 5 });

		auto v = m_hull.cmpRigidDynamic_getLinearVelocity();
		m_hull.cmpRigidDynamic_addTorque(v*dt*0.1f);
	}
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
	rotMat = glm::rotate(rotMat, mouseDelta.y / 100.f, {1, 0, 0});

	m_base.cmpTransform_setTransform(transform * rotMat);
	if (m_active) {
		m_hull.cmpRigidDynamic_addForce(m_movementDir * dt * speed);
	}
	else {
		m_base.cmpTransform_setPos(m_base.cmpTransform_getPos() + m_movementDir * dt * speed);
	}
}

void Player::updateMechanics(Controls& controls, float dt) {
	if (m_active) {
		m_energy = std::min<float>(m_energy, 100);

		m_inventory.update(*this); // update all items in inventory

		m_energy += (m_energy * 0.1 + 5) * dt;
		m_energy = std::min<float>(m_energy, 100.f);
		m_spawnProtection -= dt;

		client::sendPlayerMove(*this);
	}
	else {
		m_spawnTimeout -= dt;
		if (m_spawnTimeout < 0)
			spawn();
		ImGui::Text(std::to_string(m_spawnTimeout).c_str());
	}
}

void Player::update(Controls& controls, float dt) {
	if (m_active) {
		glm::vec3 pos = m_hull.cmpTransform_getPos(); // hull determines the position
		m_core.cmpTransform_setPos(pos);
		m_base.cmpTransform_setPos(pos);
	}
	updateCamera(controls);
}

void Player::damage(float damage) {
	if (m_spawnProtection > 0)
		return;
	m_health -= damage;
	if (m_health <= 0) {
		kill();
	}
	client::sendPlayerDamage(damage, *this, m_username);
}

void Player::localSpawn(Zap::ActorLoader& loader) {
	loader.flags = loader.flags | Zap::ActorLoader::eReuseActor;
	m_core = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerCore.zac"), &m_scene);
	m_hull = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerHull.zac"), &m_scene);
	m_hull.cmpRigidDynamic_setAngularDamping(.5);
	m_hull.cmpRigidDynamic_setLinearDamping(.9);
	m_energy = getMaxEnergy();
	m_health = getMaxHealth();
	m_active = true;
}

void Player::localKill() {
	if (m_active) {
		m_core.destroy();
		m_hull.destroy();
	}
	m_active = false;
}

void Player::spawn(Zap::ActorLoader loader) {
	localSpawn(loader);
	m_spawnProtection = 5;
	client::sendPlayerSpawn(m_username);
}

void Player::kill() {
	if (m_active) {
		m_spawnTimeout = 5;
		client::sendPlayerDeath(m_username);
	}
	localKill();
}

void Player::spendEnergy(float energy) {
	m_energy -= energy;
}

float Player::getHealth() {
	return m_health;
}

float Player::getMaxHealth() {
	return 100;
}

float Player::getEnergy() {
	return m_energy;
}

float Player::getMaxEnergy() {
	return 100;
}

PlayerInventory& Player::getInventory() {
	return m_inventory;
}

std::string Player::getUsername() {
	return m_username;
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

void Player::syncSpawn() {
	Zap::ActorLoader loader;
	localSpawn(loader);
}

void Player::syncDeath() {
	localKill();
}

void Player::syncMove(glm::mat4 transform) {
	if(m_active)
		setTransform(transform);
}

void Player::syncDamage(float damage, float newHealth) {
	m_health = newHealth;
}