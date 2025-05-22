#include "Ray.h"

#include "Layers/Network.h"

#include "Zap/Scene/Actor.h"

Ray::Ray(WorldData& world)
	: m_world(world)
{}

class RayFilter : public physx::PxQueryFilterCallback {
public:
	Zap::Actor excludedActor;

	physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) {
		PX_UNUSED(filterData); PX_UNUSED(shape); PX_UNUSED(shape); PX_UNUSED(queryFlags);
		if ((uint64_t)(actor->userData) == excludedActor.getHandle())
			return physx::PxQueryHitType::eNONE;
		return physx::PxQueryHitType::eBLOCK;
	}

	physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit, const physx::PxShape* shape, const physx::PxRigidActor* actor) {
		PX_UNUSED(filterData); PX_UNUSED(hit); PX_UNUSED(shape); PX_UNUSED(actor);
		return physx::PxQueryHitType::eBLOCK;
	}
};

void Ray::update(Player& player, PlayerInventory::iterator iterator) {
	if (m_isTriggered) {
		m_isTriggered = false; // one time trigger
		client::sendRay(player.getTransform()[3], player.getCameraTransform()[2], player.getUsername());
	}
}

void Ray::processRay(glm::vec3 origin, glm::vec3 direction, WorldData& world, Player& checkPlayer, Player& senderPlayer) {
	Zap::Scene::RaycastOutput out = {};
	RayFilter filter;
	filter.excludedActor = senderPlayer.getPhysicsActor();
	if (world.scene->raycast(origin, direction, 1000, &out, &filter)) {
		printf("ray hit (%fm)\n", out.distance);
	}
}