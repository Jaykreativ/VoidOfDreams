#include "Ray.h"

#include "Layers/Network.h"

#include "Zap/Scene/Actor.h"
#include "Zap/Scene/Material.h"

const float _energyCost = 10;
const float _damage = 10;

const std::filesystem::path _beamModel = "Models/Cube.obj";

Ray::Beam::Beam(WorldData& world, glm::vec3 origin, glm::vec3 direction, float length)
	: m_world(world)
{
	Zap::ModelLoader loader;
	auto model = loader.load(_beamModel);
	std::lock_guard<std::mutex> lk(world.mScene);
	world.game.spScene->attachActor(m_actor);
	m_actor.addTransform();
	m_actor.addModel(model);
	{
		Zap::Material mat;
		mat.setEmissive({1, 1, 1, 10});
		m_actor.cmpModel_setMaterial(mat);
	}
	m_actor.cmpTransform_setPos(origin + direction*(length/2.f));
	float angle = acos(glm::dot(direction, glm::vec3(1, 0, 0))/glm::length(direction));
	glm::vec3 cross = glm::cross(direction, glm::vec3(1, 0, 0));
	m_actor.cmpTransform_rotate(glm::degrees(-angle), cross);
	m_actor.cmpTransform_setScale(length /2.f, .025, .025);

	m_animation = std::make_shared<BeamAnimation>(*this);
	world.animations.push_back(m_animation);
}

Ray::Beam::~Beam() {
	m_actor.destroy();
}

void Ray::Beam::BeamAnimation::removeFromWorld() {
	for (auto it = m_beam.m_world.game.rayBeams.begin(); it != m_beam.m_world.game.rayBeams.end(); ++it)
		if (it->get() == &m_beam) {
			m_beam.m_world.game.rayBeams.erase(it);
			break;
		}
}

Ray::Beam::BeamAnimation::BeamAnimation(Ray::Beam& beam)
	: Animation(.1f), m_beam(beam)
{}

void Ray::Beam::BeamAnimation::update(float dt) {
	if (timeFactor() > 0.99f)
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
		glm::vec3 origin = player.getTransform()[3] + (m_alternateSide-.5f)*2*player.getTransform()[0];
		glm::vec3 direction = player.getCameraTransform()[2];
		client::sendRay(origin, direction, player.getUsername());
		player.spendEnergy(_energyCost);

		// shoot beam
		Zap::Scene::RaycastOutput out = {};
		RayFilter filter;
		filter.excludedActor = player.getPhysicsActor();
		bool hit = false;
		{
			std::lock_guard<std::mutex> lk(m_world.mScene);
			hit = m_world.game.spScene->raycast(origin, glm::normalize(direction), 1000, &out, &filter);
		}
		if (hit)
			m_world.game.rayBeams.push_back(std::make_unique<Beam>(m_world, origin, glm::normalize(direction), out.distance));
		else
			m_world.game.rayBeams.push_back(std::make_unique<Beam>(m_world, origin, glm::normalize(direction), 1000));
		m_alternateSide = !m_alternateSide;
	}
	m_isTriggered = false; // one time trigger
}

void Ray::processRay(glm::vec3 origin, glm::vec3 direction, WorldData& world, Player& checkPlayer, Player& senderPlayer) {
	Zap::Scene::RaycastOutput out = {};
	RayFilter filter;
	filter.excludedActor = senderPlayer.getPhysicsActor();
	bool hit = false;
	{
		std::lock_guard<std::mutex> lk(world.mScene);
		hit = world.game.spScene->raycast(origin, glm::normalize(direction), 1000, &out, &filter);
	}
	if (hit) {
		if (out.actor == checkPlayer.getPhysicsActor()) {
			checkPlayer.damage(_damage, senderPlayer);
		}
		world.game.rayBeams.push_back(std::make_unique<Beam>(world, origin, direction, out.distance));
	}
	else
		world.game.rayBeams.push_back(std::make_unique<Beam>(world, origin, direction, 1000));
}