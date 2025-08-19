#include "ReplicationManager.h"

#include "NetworkHandler.h"
#include "Objects/Player.h"
#include "Log.h"

// all replication objects need to be registered here
void ObjectCreationRegistry::initCreationRegistry() {
	ObjectCreationRegistry::get().addFunctions('PLYR', { playerCreate, playerDestroy });
	ObjectCreationRegistry::get().addFunctions('PLID', { playerIdentifyRPCCreate, playerIdentifyRPCDestroy });
}

ObjectCreationRegistry& ObjectCreationRegistry::get() {
	static ObjectCreationRegistry registry;
	return registry;
}

void ObjectCreationRegistry::addFunctions(uint32_t classId, FunctionPair functions) {
	m_registry[classId] = functions;
}

ReplicationObject* ObjectCreationRegistry::create(uint32_t classId, WorldDataClient& world) {
	if (!m_registry.count(classId)) {
		logger::error("ObjectCreationRegistry::create(): creation function for class not registered");
		return nullptr;
	}
	return m_registry.at(classId).creation(world);
}

void ObjectCreationRegistry::destroy(uint32_t classId, WorldDataClient& world, ReplicationObject* object) {
	if (!m_registry.count(classId)) {
		logger::error("ObjectCreationRegistry::destroy(): destruction function for class not registered");
		return;
	}
	m_registry.at(classId).destruction(world, object);
}

bool LinkingContext::hasObject(ReplicationObject* pObject) {
	return m_objectToIdMap.count(pObject);
}

bool LinkingContext::hasObject(Zap::UUID id) {
	return m_idToObjectMap.count(id);
}

Zap::UUID LinkingContext::getId(ReplicationObject* pObject, bool shouldAddUnkownObjects) {
	if (hasObject(pObject))
		return m_objectToIdMap.at(pObject);
	else if (shouldAddUnkownObjects) {
		Zap::UUID id;
		addObject(pObject, id);
		return id;
	}
	else
		logger::error("LinkingContext::getId(): replication object unknown, id cannot be returned");
}

ReplicationObject* LinkingContext::getObject(Zap::UUID id) {
	if (hasObject(id))
		return m_idToObjectMap.at(id);
	else
		logger::warning("LinkingContext::getObject(): network id unknown, no object with this id registered");
	return nullptr;
}

void LinkingContext::addObject(ReplicationObject* pObject, Zap::UUID id) {
	m_idToObjectMap[id] = pObject;
	m_objectToIdMap[pObject] = id;
}

void LinkingContext::removeObject(ReplicationObject* pObject) {
	assert(m_objectToIdMap.count(pObject));
	Zap::UUID id = m_objectToIdMap.at(pObject);
	m_idToObjectMap.erase(id);
	m_objectToIdMap.erase(pObject);
}

void LinkingContext::removeObject(Zap::UUID id) {
	assert(m_idToObjectMap.count(id));
	ReplicationObject* pObject = m_idToObjectMap.at(id);
	m_idToObjectMap.erase(id);
	m_objectToIdMap.erase(pObject);
}

void ReplicationManagerClient::addReplication(std::shared_ptr<ReplicationPacket> spPacket) {
	m_replications.push_back(spPacket);
}

void ReplicationManagerClient::processReplication(WorldDataClient& world) {
	for (auto& replication : m_replications) {
		switch (replication->type) {
		case ReplicationPacket::eCREATE: {
			auto* pObject = ObjectCreationRegistry::get().create(replication->classId, world);
			m_linkingContext.addObject(pObject, replication->objectId);
			pObject->readFromReplication(replication, replication->status, *this);
			break;
		}
		case ReplicationPacket::eUPDATE: {
			auto* pObject = m_linkingContext.getObject(replication->objectId);
			if(pObject)
				pObject->readFromReplication(replication, replication->status, *this);
			break;
		}
		case ReplicationPacket::eDESTROY: {
			auto* pObject = m_linkingContext.getObject(replication->objectId);
			if (pObject)
				ObjectCreationRegistry::get().destroy(replication->classId, world, pObject);
			break;
		}
		case ReplicationPacket::eRPC: {
			auto* pObject = ObjectCreationRegistry::get().create(replication->classId, world);
			pObject->readFromReplication(replication, 0, *this);
			reinterpret_cast<RPCObject*>(pObject)->call(world);
			break;
		}
		}
	}
	m_replications.clear();
}

std::shared_ptr<ReplicationPacket> ReplicationManagerServer::replicateCreate(ReplicationObject* object) {
	auto spPacket = std::make_shared<ReplicationPacket>();
	spPacket->type = ReplicationPacket::eCREATE;
	spPacket->classId = object->classId();
	spPacket->status = UINT32_MAX;
	spPacket->objectId = m_linkingContext.getId(object, true);
	object->writeToReplication(spPacket, UINT32_MAX, *this); // every bit is enabled
	return spPacket;
}

std::shared_ptr<ReplicationPacket> ReplicationManagerServer::replicateUpdate(ReplicationObject* object, uint32_t status) {
	auto spPacket = std::make_shared<ReplicationPacket>();
	spPacket->type = ReplicationPacket::eUPDATE;
	spPacket->classId = object->classId();
	spPacket->status = status;
	spPacket->objectId = m_linkingContext.getId(object);
	object->writeToReplication(spPacket, status, *this);
	return spPacket;
}

std::shared_ptr<ReplicationPacket> ReplicationManagerServer::replicateDestroy(ReplicationObject* object) {
	auto spPacket = std::make_shared<ReplicationPacket>();
	spPacket->type = ReplicationPacket::eDESTROY;
	spPacket->classId = object->classId();
	spPacket->objectId = m_linkingContext.getId(object); // for destruction member data doesn't matter
	return spPacket;
}

std::shared_ptr<ReplicationPacket> ReplicationManagerServer::replicateRPC(RPCObject& object) {
	auto spPacket = std::make_shared<ReplicationPacket>();
	spPacket->type = ReplicationPacket::eRPC;
	spPacket->classId = object.classId();
	object.writeToReplication(spPacket, 0, *this);
	return spPacket;
}