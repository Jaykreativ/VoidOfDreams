#include "Packets.h"

// takes a ptr to an already allocated chunk of memory and packs the string into it
// the ptr will point to the end of the packed string
void packString(char*& buf, std::string string) {
	uint32_t size = htonl(string.size());
	memcpy(buf, &size, sizeof(uint32_t)); buf += sizeof(uint32_t);
	memcpy(buf, string.data(), string.size()); buf += string.size();
}

// takes a ptr to a packed string
// the ptr will point to the end of the packed string
std::string unpackString(const char*& buf) {
	uint32_t usernameSize = ntohl(reinterpret_cast<const uint32_t*>(buf)[0]); buf += sizeof(uint32_t);
	auto str = std::string(buf, usernameSize); buf += usernameSize;
	return str;
}

// Packet
void Packet::sendTo(int socket, int flags) {
	uint32_t len = fullSize();
	char* buf = new char[len];
	auto* delBuf = buf;
	pack(buf);

	uint32_t offset = 0;
	while (offset < len) {
		int bytesSent = send(socket, buf + offset, len - offset, 0);
		if (bytesSent == -1) {
			sock::printLastError("Packet::send");
			delete[] delBuf;
			return;
		}
		offset += bytesSent;
	}
	delete[] delBuf;
}

void Packet::sendToDgram(int socket, const sockaddr* addr, int flags) {
	char buf[UDP_PACKET_BUFFER_SIZE];
	pack(buf);

	int addrlen;
	if (addr->sa_family == AF_INET)
		addrlen = sizeof(sockaddr_in);
	else if (addr->sa_family == AF_INET6)
		addrlen = sizeof(sockaddr_in6);
	else {
		printf("Packet::sendToDgram address family not supported\n");
		exit(0);
	}
	int bytesSent = sendto(socket, buf, UDP_PACKET_BUFFER_SIZE, 0, addr, addrlen); // send header only
	if (bytesSent == -1) {
		sock::printLastError("Packet::sendto header");
		return;
	}
}

std::shared_ptr<Packet> Packet::receiveFrom(int& type, int socket, int flags) {
	char* buf = new char[headerSize()];
	int bytesRead = recv(socket, buf, headerSize(), 0); // get just header
	if (bytesRead == -1) {
		sock::printLastError("Packet::recv header");
		delete[] buf;
		return nullptr;
	}
	uint32_t dataSize;
	unpackHeader(buf, dataSize, type);
	delete[] buf;

	buf = new char[dataSize];
	const char* constBuf = buf;
	bytesRead = recv(socket, buf, dataSize, 0); // get just data
	if (bytesRead == -1) {
		sock::printLastError("Packet::recv data");
		delete[] buf;
		return nullptr;
	}

	std::shared_ptr<Packet> spPacket;
	switch (type)
	{
		//case eMESSAGE: {
		//	spPacket = std::make_shared<MessagePacket>();
		//	spPacket->unpackData(buf, dataSize);
		//	break;
		//}
	case eCONNECT: {
		spPacket = std::make_shared<ConnectPacket>();
		break;
	}
	case eDISCONNECT: {
		spPacket = std::make_shared<DisconnectPacket>();
		break;
	}
	case eMOVE: {
		spPacket = std::make_shared<MovePacket>();
		break;
	}
	case eDamage: {
		spPacket = std::make_shared<DamagePacket>();
		break;
	}
	case eSpawn: {
		spPacket = std::make_shared<SpawnPacket>();
		break;
	}
	case eDeath: {
		spPacket = std::make_shared<DeathPacket>();
		break;
	}
	case eRay: {
		spPacket = std::make_shared<RayPacket>();
		break;
	}
	default:
		break;
	}
	spPacket->unpackGeneralData(constBuf);
	spPacket->unpackData(constBuf, dataSize-spPacket->generalDataSize());
	delete[] buf;

	return spPacket;
}

std::shared_ptr<Packet> Packet::receiveFromDgram(int& type, int socket, sockaddr* addr, int* addrlen, int flags) {
	char buf[UDP_PACKET_BUFFER_SIZE];
	const char* constBuf = buf;
	int bytesRead = recvfrom(socket, buf, UDP_PACKET_BUFFER_SIZE, 0, addr, addrlen); // get just header
	if (bytesRead == -1) {
		sock::printLastError("Packet::recvfrom");
		return nullptr;
	}
	uint32_t dataSize;
	unpackHeader(constBuf, dataSize, type);
	constBuf += headerSize();

	std::shared_ptr<Packet> spPacket;
	switch (type)
	{
		//case eMESSAGE: {
		//	spPacket = std::make_shared<MessagePacket>();
		//	spPacket->unpackData(ptr, dataSize);
		//	break;
		//}
	case eCONNECT: {
		spPacket = std::make_shared<ConnectPacket>();
		break;
	}
	case eDISCONNECT: {
		spPacket = std::make_shared<DisconnectPacket>();
		break;
	}
	case eMOVE: {
		spPacket = std::make_shared<MovePacket>();
		break;
	}
	case eDamage: {
		spPacket = std::make_shared<DamagePacket>();
		break;
	}
	case eSpawn: {
		spPacket = std::make_shared<SpawnPacket>();
		break;
	}
	case eDeath: {
		spPacket = std::make_shared<DeathPacket>();
		break;
	}
	case eRay: {
		spPacket = std::make_shared<RayPacket>();
		break;
	}
	default:
		break;
	}
	spPacket->unpackGeneralData(constBuf);
	spPacket->unpackData(constBuf, dataSize - spPacket->generalDataSize());
	return spPacket;

}

uint32_t Packet::fullSize() {
	return headerSize() + generalDataSize() + dataSize();
}

uint32_t Packet::headerSize() {
	return 2 * sizeof(uint32_t);
}

uint32_t Packet::generalDataSize() {
	return sizeof(uint32_t) + username.size();
}

void Packet::packHeader(char* buf, const int type) {
	uint32_t* uintBuf = reinterpret_cast<uint32_t*>(buf);
	uintBuf[0] = htonl(generalDataSize() + dataSize());
	uintBuf[1] = htonl(type);
}

void Packet::unpackHeader(const char* buf, uint32_t& size, int& type) {
	const uint32_t* uintBuf = reinterpret_cast<const uint32_t*>(buf);
	size = ntohl(uintBuf[0]);
	type = ntohl(uintBuf[1]);
}

void Packet::packGeneralData(char*& buf, const int type) {
	packHeader(buf, type); buf += headerSize();
	packString(buf, username);
}

void Packet::unpackGeneralData(const char*& buf) {
	username = unpackString(buf);
}

// MessagePacket
//uint32_t MessagePacket::dataSize() {
//		return sizeof(uint32_t) + id.size() + sizeof(uint32_t) + msg.size();
//	}
//
//void MessagePacket::pack(char* buf) {
//		packHeader(buf, eMESSAGE); buf += headerSize();
//		/* data */
//		uint32_t idSize = htonl(id.size());
//		memcpy(buf, &idSize, sizeof(uint32_t));    buf += sizeof(uint32_t);
//		memcpy(buf, id.data(), id.size());         buf += id.size();
//		uint32_t msgSize = htonl(msg.size());
//		memcpy(buf, &msgSize, sizeof(uint32_t));   buf += sizeof(uint32_t);
//		memcpy(buf, msg.data(), msg.size());       buf += msg.size();
//}
//
//void MessagePacket::unpackData(const char* buf, uint32_t size) {
//		uint32_t idSize = ntohl(reinterpret_cast<const uint32_t*>(buf)[0]); buf += sizeof(uint32_t);
//		id = std::string(buf, idSize); buf += idSize;
//		uint32_t msgSize = ntohl(reinterpret_cast<const uint32_t*>(buf)[0]); buf += sizeof(uint32_t);
//		msg = std::string(buf, msgSize); buf += msgSize;
//}

// ConnectPacket
uint32_t ConnectPacket::dataSize() {
	return 0;
}

void ConnectPacket::pack(char* buf) {
	packGeneralData(buf, eCONNECT);
	/* data */
}

void ConnectPacket::unpackData(const char* buf, uint32_t size) {}

// DisconnectPacket
uint32_t DisconnectPacket::dataSize() {
	return 0;
}

void DisconnectPacket::pack(char* buf) {
	packGeneralData(buf, eDISCONNECT);
	/* data */
}

void DisconnectPacket::unpackData(const char* buf, uint32_t size) {}

// MovePacket
uint32_t MovePacket::dataSize() {
	return sizeof(glm::mat4);
}

void MovePacket::pack(char* buf) {
	packGeneralData(buf, eMOVE);
	/* data */
	sock::htonMat4(transform, buf);
}

void MovePacket::unpackData(const char* buf, uint32_t size) {
	sock::ntohMat4(buf, transform);
}

// DamagePacket
uint32_t DamagePacket::dataSize() {
	return 2*sizeof(float);
}

void DamagePacket::pack(char* buf) {
	packGeneralData(buf, eDamage);
	/* data */
	uint32_t nDamage = htonf(damage);
	memcpy(buf, &nDamage, sizeof(uint32_t)); buf += sizeof(uint32_t);
	uint32_t nHealth = htonf(health);
	memcpy(buf, &nHealth, sizeof(uint32_t));
}

void DamagePacket::unpackData(const char* buf, uint32_t size) {
	damage = ntohf(reinterpret_cast<const uint32_t*>(buf)[0]);
	health = ntohf(reinterpret_cast<const uint32_t*>(buf)[1]);
}

// SpawnPacket
uint32_t SpawnPacket::dataSize() {
	return 0;
}

void SpawnPacket::pack(char* buf) {
	packGeneralData(buf, eSpawn);
	/* data */
}

void SpawnPacket::unpackData(const char* buf, uint32_t size) {}

// DeathPacket
uint32_t DeathPacket::dataSize() {
	return 0;
}

void DeathPacket::pack(char* buf) {
	packGeneralData(buf, eDeath);
	/* data */
}

void DeathPacket::unpackData(const char* buf, uint32_t size) {}

// RayPacket
uint32_t RayPacket::dataSize() {
	return 2*sizeof(glm::vec3);
}

void RayPacket::pack(char* buf) {
	packGeneralData(buf, eRay);
	/* data */
	sock::htonVec3(origin, buf); buf += sizeof(glm::vec3);
	sock::htonVec3(direction, buf);
}

void RayPacket::unpackData(const char* buf, uint32_t size) {
	sock::ntohVec3(buf, origin); buf += sizeof(glm::vec3);
	sock::ntohVec3(buf, direction);
}
