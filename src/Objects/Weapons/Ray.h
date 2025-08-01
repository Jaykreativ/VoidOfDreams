#pragma once

#include "Objects/Item.h"
#include "Objects/Animation.h"

#include "Zap/Scene/Actor.h"

#include "glm.hpp"

struct WorldDataClient;

class Ray : public Weapon{
public:
	Ray(WorldDataClient& world);

	void update(Player& player, PlayerInventory::iterator iterator) override;

	static void processRay(glm::vec3 origin, glm::vec3 direction, WorldDataClient& world, Player& checkPlayer, Player& senderPlayer);

	class Beam {
	public:
		Beam(WorldDataClient& world, glm::vec3 origin, glm::vec3 direction, float length);
		~Beam();

	private:
		WorldDataClient& m_world;
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
	WorldDataClient& m_world;

	int m_alternateSide = 0;
};