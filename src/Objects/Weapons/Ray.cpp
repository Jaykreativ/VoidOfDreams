#include "Ray.h"

Ray::Ray(WorldData& world)
	: m_world(world)
{}

class RayFilter : physx::PxQueryFilterCallback {
	physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) {

	}
};

void Ray::update(Player& player, PlayerInventory::iterator iterator) {
	if (m_isTriggered) {
		m_isTriggered = false; // one time trigger
		Zap::Scene::RaycastOutput out = {};
		if (m_world.scene->raycast(player.getTransform()[3], player.getCameraTransform()[2], 1000, &out)) {
			printf("ray hit (%fm)\n", out.distance);
		}
	}
}