#pragma once

#include "Objects/Item.h"
#include "Objects/Animation.h"

#include "Zap/Scene/Actor.h"

#include "glm.hpp"

struct WorldDataServer;

class Ray : public Weapon {
public:
	Ray();

	void update(Player& player, PlayerInventory::iterator iterator, const InputState& input, WorldDataServer& world) override;

	static void processRay(glm::vec3 origin, glm::vec3 direction, WorldDataServer& world, Player& checkPlayer, Player& senderPlayer);

	class Beam {
	public:
		Beam(WorldDataServer& world, glm::vec3 origin, glm::vec3 direction, float length);
		~Beam();

	private:
		WorldDataServer& m_world;
		Zap::Actor m_actor;

		class BeamAnimation : public Animation {
		public:
			BeamAnimation(Ray::Beam& beam);

			void removeFromWorld();

			void update(float dt) override;

		private:
			Ray::Beam& m_beam;
		};
		std::shared_ptr<Animation> m_animation;
	};

private:
	int m_alternateSide = 0;
};