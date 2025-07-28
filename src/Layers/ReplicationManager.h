#pragma once

#include "Objects/Packets.h"
#include "Shares/World.h"

#include "Zap/UUID.h"

#include <memory>

// interface class required for object replication
class ReplicationObject {
private:
	virtual uint32_t classId() = 0;

	// reads all members indicated by the status flag from the specified ReplicationPacket
	virtual void readFromReplication(std::shared_ptr<ReplicationPacket> spPacket, uint32_t status) = 0;

	// writes all members indicated by the status flag to the specified ReplicationPacket
	virtual void writeToReplication(std::shared_ptr<ReplicationPacket> spPacket, uint32_t status) = 0;

	friend class ReplicationManager;
	friend class ReplicationManagerClient;
	friend class ReplicationManagerServer;
};

typedef ReplicationObject* (*ObjectCreationFunction)(WorldData&);
class ObjectCreationRegistry {
public:
	static ObjectCreationRegistry& get();

	void addFunction(uint32_t classId, ObjectCreationFunction creationFunction);

	// calls the class creation function
	// returns nullptr on failure
	ReplicationObject* create(uint32_t classId, WorldData& world);

private:
	ObjectCreationRegistry(){}
	~ObjectCreationRegistry(){}

	std::unordered_map<uint32_t, ObjectCreationFunction> m_registry = {};
};

class LinkingContext {
public:
	bool hasObject(ReplicationObject* pObject);
	bool hasObject(Zap::UUID id);

	Zap::UUID getId(ReplicationObject* pObject, bool shouldAddUnkownObjects = false);

	ReplicationObject* getObject(Zap::UUID id);

	void addObject(ReplicationObject* pObject, Zap::UUID id);

	void removeObject(ReplicationObject* pObject);
	void removeObject(Zap::UUID id);

private:
	std::unordered_map<Zap::UUID, ReplicationObject*> m_idToObjectMap = {};
	std::unordered_map<ReplicationObject*, Zap::UUID> m_objectToIdMap = {};
};

// replication packets fed by the network are stored, the main loop is activating the processing of those packets
class ReplicationManager {
public:
	ReplicationManager() = default;
	virtual ~ReplicationManager() = default;

protected:
	LinkingContext m_linkingContext;
};

class ReplicationManagerClient : public ReplicationManager {
public:
	// used by the network layer to push replicate packets with thread safety
	void addReplication(std::shared_ptr<ReplicationPacket> spPacket);

	// processes the replication commands pushed by the network
	// has access to all objects it needs to replicate
	void processReplication(WorldData& world);

private:
	std::vector<std::shared_ptr<ReplicationPacket>> m_replications = {};
};

class ReplicationManagerServer : public ReplicationManager {
public:
	std::shared_ptr<ReplicationPacket> replicateCreate(ReplicationObject* object);

	std::shared_ptr<ReplicationPacket> replicateUpdate(ReplicationObject* object, uint32_t status);

	std::shared_ptr<ReplicationPacket> replicateDestroy(ReplicationObject* object);
};