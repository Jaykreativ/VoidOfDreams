#pragma once

#include "Layers/ReplicationManager.h"
#include "Objects/Item.h"
#include "Objects/Animation.h"

#include "Zap/Scene/Actor.h"

#include "glm.hpp"

struct WorldDataServer;

// can be sent over the network to trigger the effect
class BeamEffectRPC : public RPCObject {
public:
	BeamEffectRPC() = default;
	BeamEffectRPC(glm::vec3 origin,	glm::vec3 direction,float length)
		: origin(origin), direction(direction), length(length)
	{}

	uint32_t classId() override;

	void call(WorldDataClient& world) override;

	void readFromReplication(std::shared_ptr<ReplicationPacket> spPacket, uint32_t status, ReplicationManager& manager) override;

	void writeToReplication(std::shared_ptr<ReplicationPacket> spPacket, uint32_t status, ReplicationManager& manager) override;

	glm::vec3 origin;
	glm::vec3 direction;
	float length;
};
inline ReplicationObject* beamEffectRPCCreate(WorldDataClient& world) {
	return new BeamEffectRPC();
}
inline void beamEffectRPCDestroy(WorldDataClient& world, ReplicationObject* obj) {
	delete obj;
}

class Ray : public Weapon {
public:
	Ray();

	void update(Player& player, PlayerInventory::iterator iterator, const InputState& input, WorldDataServer& world) override;

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
	int m_alternateSide = 0;
};