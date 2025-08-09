#include "Layers/Game.h"

#include "glm.hpp"
#include "imgui.h"

#include "Player.h"

#include <string>

ReplicationObject* playerCreate(WorldDataClient& world) {
	auto sptr = std::make_shared<PlayerClient>(*world.game.spScene);
	world.game.players.push_back(sptr);
	return sptr.get();
}

void playerDestroy(WorldDataClient& world, ReplicationObject* obj) {
	auto it = world.game.players.begin();
	for (auto spPlayer : world.game.players) {
		if (spPlayer.get() == obj) {
			world.game.players.erase(it); // remove destroyed player
			return;
		}
		it++;
	}
}

Player::Player(Zap::Scene& scene)
	: m_scene(scene)
{
	Zap::ActorLoader loader;
	loader.flags = loader.flags | Zap::ActorLoader::eReuseActor;
	m_base = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerBase.zac"), &m_scene);
}

Player::~Player() {
	m_base.destroy();
	if (m_active) {
		m_hull.destroy();
	}
}

void PlayerClient::updateCamera(Controls& controls) {
	if (m_mode == eWEAPON)
		controls.cameraMode = Controls::eFIRST_PERSON;
	else
		controls.cameraMode = Controls::eTHIRD_PERSON;

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

void PlayerClient::updateAnimations(float dt) {
	if (m_active) {
		m_core.cmpTransform_rotate(-90 * dt, { 2, 3, 5 });
	}
}

void Player::updateMechanics(Controls& controls, float dt) {
	if (m_active) {
		m_energy = std::min<float>(m_energy, 100);

		m_inventory.update(*this); // update all items in inventory

		m_energy += (m_energy * 0.1 + 5) * dt;
		m_energy = std::min<float>(m_energy, 100.f);
		m_spawnProtection -= dt;
	}
	else {
		m_spawnTimeout -= dt;
		if (m_spawnTimeout < 0)
			spawn();
	}
}

void Player::update(float dt) {
	if (m_active) {
		glm::vec3 pos = m_hull.cmpTransform_getPos(); // hull determines the position
		m_base.cmpTransform_setPos(pos);

		auto v = m_hull.cmpRigidDynamic_getLinearVelocity();
		m_hull.cmpRigidDynamic_addTorque(v * dt * 0.1f);
	}
}

uint32_t Player::classId() { return 'PLYR'; }

void Player::readFromReplication(std::shared_ptr<ReplicationPacket> spPacket, uint32_t status) {
	bool oldActive = m_active;
	m_active = spPacket->readb();
	if (oldActive != m_active) {
		if (m_active) { // detect spawn/kill
			spawn();
		}
		else {
			kill();
		}
	}
	m_mode = spPacket->readu32();

	m_base.cmpTransform_setPos(spPacket->readVec3());

	if (m_active) {
		m_hull.cmpTransform_setTransform(spPacket->readMat4());
		m_hull.cmpRigidDynamic_setLinearVelocity(spPacket->readVec3());
		m_hull.cmpRigidDynamic_setAngularVelocity(spPacket->readVec3());
		m_hull.cmpRigidDynamic_updatePose();
	}

	m_health = spPacket->readf();
	m_energy = spPacket->readf();
	
	m_kills = spPacket->readu32();
	m_deaths = spPacket->readu32();
	m_damage = spPacket->readf();
}

void Player::writeToReplication(std::shared_ptr<ReplicationPacket> spPacket, uint32_t status) {
	spPacket->write(m_active);
	spPacket->write(m_mode);

	spPacket->write(m_base.cmpTransform_getPos());

	if (m_active) {
		spPacket->write(m_hull.cmpTransform_getTransform());
		spPacket->write(m_hull.cmpRigidDynamic_getLinearVelocity());
		spPacket->write(m_hull.cmpRigidDynamic_getAngularVelocity());
	}

	spPacket->write(m_health);
	spPacket->write(m_energy);

	spPacket->write(m_kills);
	spPacket->write(m_deaths);
	spPacket->write(m_damage);
}

PlayerServer::PlayerServer(Zap::Scene& scene)
	: Player(scene)
{}

PlayerServer::~PlayerServer() {}

void PlayerServer::update(float dt) {
	Player::update(dt);
}

void PlayerClient::updateFocused(float dt, Controls& controls, const InputState& input, InputHandlerClient& inputHandler) {
	updateCamera(controls);
	float speed = 25;
	auto transform = m_base.cmpTransform_getTransform();
	glm::vec3 moveVec = glm::vec3(transform * glm::vec4(input.getMoveDir(), 0)) * dt * speed;

	if (m_active) {
		m_hull.cmpRigidDynamic_addForce(moveVec);
	}
	else {
		m_base.cmpTransform_setPos(m_base.cmpTransform_getPos() + moveVec);
	}

	m_base.cmpTransform_setTransform(transform * input.getRotationDeltaMat());

	// switch mode
	if (input.hasSwitchedMode()) {
		spendEnergy(10);
		m_energy = std::max<float>(m_energy, 0);
		if (m_mode == eWEAPON)
			m_mode = eABILITY;
		else
			m_mode = eWEAPON;
	}

	inputHandler.pushAction();
}

void PlayerClient::update(float dt) {
	updateAnimations(dt);
	Player::update(dt);
	if (m_active) {
		glm::vec3 pos = m_hull.cmpTransform_getPos(); // hull determines the position
		m_core.cmpTransform_setPos(pos);
	}

	m_events = m_recordEvents;
	m_recordEvents = eNONE;
}

void Player::damage(float damage) {
	if (m_spawnProtection > 0)
		return;
	m_health -= damage;
	if (m_health <= 0) {
		//client::sendPlayerDamage(damage + m_health, m_health, m_username, "");
		kill();
	}
	//client::sendPlayerDamage(damage, m_health, m_username, "");
}

void Player::damage(float damage, const Player& damager) {
	if (m_spawnProtection > 0)
		return;
	m_health -= damage;
	if (m_health <= 0) {
		kill(damager);
	}
	//client::sendPlayerDamage(damage, m_health, m_username, damager.m_username);
}

void PlayerClient::damage(float damage, const Player& damager) {
	Player::damage(damage, damager);
	m_recordEvents |= eDAMAGE_TAKEN;
}

void Player::localSpawn() {
	Zap::ActorLoader loader;
	loader.flags = loader.flags | Zap::ActorLoader::eReuseActor;
	m_hull = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerHull.zac"), &m_scene);
	m_hull.cmpRigidDynamic_setAngularDamping(.5);
	m_hull.cmpRigidDynamic_setLinearDamping(.9);
	m_energy = getMaxEnergy();
	m_health = getMaxHealth();
	m_active = true;
}

void PlayerClient::localSpawn() {
	Zap::ActorLoader loader;
	loader.flags = loader.flags | Zap::ActorLoader::eReuseActor;
	m_core = loader.load(std::filesystem::path(ACTOR_DIR) / std::filesystem::path("PlayerCore.zac"), &m_scene);
	m_recordEvents |= eSPAWN;
	Player::localSpawn();
}

void Player::localKill() {
	if (m_active) {
		m_hull.destroy();
		m_deaths++;
	}
	m_active = false;
}

void PlayerClient::localKill() {
	Player::localKill();
	if (m_active) {
		m_core.destroy();
		m_recordEvents |= eDEATH;
	}
}

void Player::spawn() {
	localSpawn();
	m_spawnProtection = 5;
	//client::sendPlayerSpawn(m_username);
}

void Player::kill() {
	if (m_active) {
		m_spawnTimeout = 5;
		//client::sendPlayerDeath(m_username, "");
	}
	localKill();
}

void Player::kill(const Player& killer) {
	if (m_active) {
		m_spawnTimeout = 5;
		//client::sendPlayerDeath(m_username, killer.m_username);
	}
	localKill();
}

void Player::spendEnergy(float energy) {
	m_energy -= energy;
}

void PlayerClient::spendEnergy(float energy) {
	Player::spendEnergy(energy);
	m_recordEvents |= eENERGY_SPENT;
}

void PlayerClient::disableInput() {
	m_recvInput = false;
}

void PlayerClient::enableInput() {
	m_recvInput = true;
}

bool PlayerClient::receivesInput() {
	return m_recvInput;
}

bool Player::isAlive() {
	return m_active;
}

bool Player::isWeaponMode() {
	return m_mode == eWEAPON;
}

bool Player::isAbilityMode() {
	return m_mode == eABILITY;
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

uint32_t Player::getKills() {
	return m_kills;
}

uint32_t Player::getDeaths() {
	return m_deaths;
}

float Player::getDamage() {
	return m_damage;
}

float Player::getSpawnTimeout() {
	return m_spawnTimeout;
}

float Player::getSpawProtectionTimeout() {
	return m_spawnProtection;
}

PlayerInventory& Player::getInventory() {
	return m_inventory;
}

std::string Player::getUsername() {
	return m_username;
}

Zap::Actor PlayerClient::getCamera() {
	return m_camera;
}

Zap::Actor Player::getPhysicsActor() {
	return m_hull;
}

glm::mat4 PlayerClient::getCameraTransform() {
	return m_camera.cmpTransform_getTransform();
}

void Player::setTransform(glm::mat4 transform) {
	m_hull.cmpTransform_setTransform(transform);
	m_hull.cmpRigidDynamic_updatePose();
}

glm::mat4 Player::getTransform() {
	return m_hull.cmpTransform_getTransform();
}

bool PlayerClient::hasTakenDamage() { return ZP_IS_FLAG_ENABLED(m_events, eDAMAGE_TAKEN); }
bool PlayerClient::hasSpentEnergy() { return ZP_IS_FLAG_ENABLED(m_events, eENERGY_SPENT); }
bool PlayerClient::hasDied()        { return ZP_IS_FLAG_ENABLED(m_events, eDEATH); }
bool PlayerClient::hasSpawned()     { return ZP_IS_FLAG_ENABLED(m_events, eSPAWN); }
bool PlayerClient::hasDoneDamage()  { return ZP_IS_FLAG_ENABLED(m_events, eDAMAGE_DONE); }
bool PlayerClient::hasKilled()      { return ZP_IS_FLAG_ENABLED(m_events, eKILL); }

PlayerClient::PlayerClient(Zap::Scene& scene)
	: Player(scene)
{
	m_camera = Zap::Actor(); // creating a camera to follow player
	m_scene.attachActor(m_camera);
	m_camera.addTransform(glm::mat4(1));
	m_camera.addCamera();
}

PlayerClient::~PlayerClient() {
	m_camera.destroy();
	if (m_active) {
		m_core.destroy();
	}
}