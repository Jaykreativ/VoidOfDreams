#pragma once

#include "Layers/InputHandler.h"
#include "Shares/Controls.h"
#include "Objects/Inventory.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

class Player {
public:
	Player(Zap::Scene& scene, std::string username);
	~Player();

	// should only update mechanics in game, not in main menu
	void updateMechanics(Controls& controls, float dt);

	void damage(float damage);
	virtual void damage(float damage, const Player& damager);

	// spawn player without network sync
	virtual void localSpawn();

	// kill player without network sync
	virtual void localKill();

	void spawn();

	void kill();
	void kill(const Player& killer);

	virtual void spendEnergy(float energy);

	bool isAlive();

	bool isWeaponMode();

	bool isAbilityMode();

	float getHealth();

	float getMaxHealth();

	float getEnergy();

	float getMaxEnergy();

	uint32_t getKills();

	uint32_t getDeaths();

	float getDamage();

	float getSpawnTimeout();

	float getSpawProtectionTimeout();

	PlayerInventory& getInventory();

	std::string getUsername();

	Zap::Actor getPhysicsActor();

	void setTransform(glm::mat4 transform);

	glm::mat4 getTransform();

protected:
	// synchronized
	Zap::Actor m_base; // this is the actual transform of the player

	bool m_active = false; // switches between active and spectator mode TODO integrate into player modes
	enum Mode {
		eWEAPON = 0x0,
		eABILITY = 0x1
	} m_mode = eWEAPON;

	float m_health = 100;
	float m_energy = 100;

	uint32_t m_kills = 0;
	uint32_t m_deaths = 0;
	float m_damage = 0;
	//

	Zap::Scene& m_scene;

	PlayerInventory m_inventory;

	Zap::Actor m_hull; // the rotating hull

	std::string m_username;

	float m_spawnProtection = 5;
	float m_spawnTimeout = 5;

	virtual void update(float dt);
};

class PlayerServer : public Player {
	void update(float dt);
};

class PlayerClient : public Player {
public:
	PlayerClient(Zap::Scene& scene);
	~PlayerClient();

	// does all updates needed for the real player
	void updateFocused(float dt, Controls& controls, InputHandlerClient& input);

	// general update function for all players
	void update(float dt);

	void damage(float damage, const Player& damager);

	void localSpawn();

	void localKill();

	void spendEnergy(float energy);

	void disableInput();

	void enableInput();

	bool receivesInput();

	Zap::Actor getCamera();

	glm::mat4 getCameraTransform();

	// events
	bool hasTakenDamage();
	bool hasSpentEnergy();
	bool hasDied();
	bool hasSpawned();
	bool hasDoneDamage();
	bool hasKilled();

private:
	bool m_recvInput = false; // UI can block input

	Zap::Actor m_core; // the bright core in the centre
	Zap::Actor m_camera;

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

	void updateAnimations(float dt);
};