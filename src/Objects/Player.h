#include "Shares/Keybinds.h"

#include "Zap/Zap.h"
#include "Zap/FileLoader.h"
#include "Zap/Scene/Scene.h"
#include "Zap/Scene/Actor.h"

class Player {
public:
	Player(Zap::Scene& scene, Zap::ActorLoader loader = Zap::ActorLoader());
	~Player();

	void updateAnimations(float dt);

	void updateInputs(Keybinds& keybinds, float dt);

	Zap::Actor getCamera();

private:
	Zap::Actor m_actor;
	Zap::Actor m_camera;

	void updateCamera();
};