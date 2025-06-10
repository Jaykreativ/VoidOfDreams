#pragma once

#include "Objects/Item.h"
#include "Objects/Animation.h"

#include "Zap/Scene/Actor.h"

#include "glm.hpp"

struct WorldData;

class Ray : public Weapon{
public:
	Ray(WorldData& world);

	void update(Player& player, PlayerInventory::iterator iterator) override;

	static void processRay(glm::vec3 origin, glm::vec3 direction, WorldData& world, Player& checkPlayer, Player& senderPlayer);

	class Beam {
	public:
		Beam(WorldData& world, glm::vec3 origin, glm::vec3 direction);
		~Beam();

	private:
		WorldData& m_world;
		Zap::Actor m_actor;
		glm::vec3 m_origin;
		glm::vec3 m_direction;

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
	WorldData& m_world;

	int m_alternateSide;
};