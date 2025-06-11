#pragma once

#include "Shares/Controls.h"
#include "Objects/Inventory.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

class Player {
public:
	Player(Zap::Scene& scene, std::string username, Zap::ActorLoader loader = Zap::ActorLoader());
	~Player();

	void updateAnimations(float dt);

	void updateInputs(Controls& controls, float dt);

	// only this clients player is updated
	void updateMechanics(Controls& controls, float dt);

	void update(float dt);

	void damage(float damage);
	void damage(float damage, const Player& damager);

	void spawn(Zap::ActorLoader loader = Zap::ActorLoader());

	void kill();
	void kill(const Player& killer);

	void spendEnergy(float energy);

	bool isWeaponMode();

	bool isAbilityMode();

	float getHealth();

	float getMaxHealth();

	float getEnergy();

	float getMaxEnergy();

	uint32_t getKills();

	uint32_t getDeaths();

	float getDamage();

	PlayerInventory& getInventory();

	std::string getUsername();

	Zap::Actor getCamera();

	Zap::Actor getPhysicsActor();

	glm::mat4 getCameraTransform();

	glm::vec3 getMovementDirection();

	void setTransform(glm::mat4 transform);

	glm::mat4 getTransform();

	// events
	bool hasTakenDamage();
	bool hasSpentEnergy();
	bool hasDied();
	bool hasSpawned();
	bool hasDoneDamage();
	bool hasKilled();

	// network interaction/synchronization
	void syncSpawn();

	void syncDeath();
	void syncDeath(Player& killer);

	void syncMove(glm::mat4 transform);

	void syncDamage(Player& damager, float damage, float newHealth);

private:
	bool m_active = false;

	enum Mode {
		eWEAPON = 0x0,
		eABILITY = 0x1
	} m_mode = eWEAPON;

	Zap::Actor m_base; // this is the actual transform of the player
	Zap::Actor m_core; // the bright core in the centre
	Zap::Actor m_hull; // the rotating hull
	Zap::Actor m_camera;

	PlayerInventory m_inventory;

	float m_health = 100;
	float m_energy = 100;

	uint32_t m_kills = 0;
	uint32_t m_deaths = 0;
	float m_damage = 0;

	std::string m_username;
	Zap::Scene& m_scene;

	glm::vec3 m_movementDir = { 0, 0, 0 };
	float m_spawnProtection = 5;
	float m_spawnTimeout = 5;

	// events
	enum Events {
		eNONE = 0x0,
		eDAMAGE_TAKEN = 0x1,
		eENERGY_SPENT = 0x2,
		eDEATH = 0x4,
		eSPAWN = 0x8,
		eDAMAGE_DONE = 0x10,
		eKILL = 0x20
	};
	// records all events during a frame
	uint32_t m_recordEvents = eNONE;
	// contains all events that happened the last frame
	uint32_t m_events = eNONE;

	void updateCamera(Controls& controls);

	void localSpawn(Zap::ActorLoader& loader);

	void localKill();
};