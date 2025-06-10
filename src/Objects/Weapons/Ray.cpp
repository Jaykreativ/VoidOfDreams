#include "Ray.h"

#include "Layers/Network.h"

#include "Zap/Scene/Actor.h"

const float _energyCost = 10;
const float _damage = 10;

Ray::Beam::Beam(WorldData& world, glm::vec3 origin, glm::vec3 direction)
	: m_world(world), m_origin(origin), m_direction(direction)
{
	printf("new beam!!\n");
	m_animation = std::make_shared<BeamAnimation>(*this);
	world.animations.push_back(m_animation);
}

Ray::Beam::~Beam() {
	printf("beam gone\n");
}

void Ray::Beam::BeamAnimation::removeFromWorld() {
	for (auto it = m_beam.m_world.rayBeams.begin(); it != m_beam.m_world.rayBeams.end(); ++it)
		if (it->get() == &m_beam) {
			m_beam.m_world.rayBeams.erase(it);
			break;
		}
}

Ray::Beam::BeamAnimation::BeamAnimation(Ray::Beam& beam)
	: Animation(1), m_beam(beam)
{}

void Ray::Beam::BeamAnimation::update(float dt) {
	if (timeFactor() > 0.9f)
		removeFromWorld();
	addDeltaTime(dt);
}

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
	if (m_isTriggered && player.isWeaponMode() && (player.getEnergy() >= _energyCost)) {
		glm::vec3 origin = player.getTransform()[3];
		glm::vec3 direction = player.getCameraTransform()[2];
		client::sendRay(origin, direction, player.getUsername());
		player.spendEnergy(_energyCost);

		// shoot beam
		m_world.rayBeams.push_back(std::make_unique<Beam>(m_world, origin, direction));
	}
	m_isTriggered = false; // one time trigger
}

void Ray::processRay(glm::vec3 origin, glm::vec3 direction, WorldData& world, Player& checkPlayer, Player& senderPlayer) {
	Zap::Scene::RaycastOutput out = {};
	RayFilter filter;
	filter.excludedActor = senderPlayer.getPhysicsActor();
	if (world.scene->raycast(origin, direction, 1000, &out, &filter)) {
		if (out.actor == checkPlayer.getPhysicsActor()) {
			checkPlayer.damage(_damage, senderPlayer);
		}
	}
}