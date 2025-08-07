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

void createPacket(int type, std::shared_ptr<Packet>& spPacket) {
	switch (type)
	{
		//case eMESSAGE: {
		//	spPacket = std::make_shared<MessagePacket>();
		//	spPacket->unpackData(buf, dataSize);
		//	break;
		//}
	case eHello: {
		spPacket = std::make_shared<HelloPacket>();
		break;
	}
	case eWelcome: {
		spPacket = std::make_shared<WelcomePacket>();
		break;
	}
	case eReplication: {
		spPacket = std::make_shared<ReplicationPacket>();
		break;
	}
	case eDisconnect: {
		spPacket = std::make_shared<DisconnectPacket>();
		break;
	}
	default:
		break;
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
	createPacket(type, spPacket);
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
	createPacket(type, spPacket);
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
	return 0;
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
}

void Packet::unpackGeneralData(const char*& buf) {}

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

// HelloPacket
uint32_t HelloPacket::dataSize() {
	return sizeof(uint64_t) + sizeof(uint32_t) + username.size();
}

void HelloPacket::pack(char* buf) {
	packGeneralData(buf, eHello);
	/* data */
	reinterpret_cast<uint64_t*>(buf)[0] = htonll(id); buf += sizeof(id);
	packString(buf, username);
}

void HelloPacket::unpackData(const char* buf, uint32_t size) {
	id = ntohll(reinterpret_cast<const uint64_t*>(buf)[0]); buf += sizeof(id);
	username = unpackString(buf);
}

// WelcomePacket
uint32_t WelcomePacket::dataSize() {
	return sizeof(bool);
}

void WelcomePacket::pack(char* buf) {
	packGeneralData(buf, eWelcome);
	/* data */
	reinterpret_cast<bool*>(buf)[0] = fromDgram;
}

void WelcomePacket::unpackData(const char* buf, uint32_t size) {
	fromDgram = reinterpret_cast<const bool*>(buf)[0];
}

// ReplicationPacket
uint32_t ReplicationPacket::dataSize() {
	return sizeof(uint32_t)*3 + sizeof(Zap::UUID) + m_data.size();
}

void ReplicationPacket::write(bool val) {
	auto oldSize = m_data.size();
	m_data.resize(oldSize + sizeof(val));
	memcpy(&m_data[oldSize], &val, sizeof(val));
}
void ReplicationPacket::write(float val) {
	auto oldSize = m_data.size();
	uint32_t nval = htonf(val);
	m_data.resize(oldSize + sizeof(nval));
	memcpy(&m_data[oldSize], &nval, sizeof(nval));
}
void ReplicationPacket::write(uint32_t val) {
	auto oldSize = m_data.size();
	uint32_t nval = htonl(val);
	m_data.resize(oldSize + sizeof(nval));
	memcpy(&m_data[oldSize], &nval, sizeof(nval));
}
void ReplicationPacket::write(glm::vec3 val) {
	write(val.x);
	write(val.y);
	write(val.z);
}
void ReplicationPacket::write(glm::vec4 val) {
	write(val.x);
	write(val.y);
	write(val.z);
	write(val.w);
}
void ReplicationPacket::write(glm::mat4 val) {
	write(val[0]);
	write(val[1]);
	write(val[2]);
	write(val[3]);
}

bool ReplicationPacket::readb() {
	bool val;
	size_t readSize = m_readOffset + sizeof(val);
	assert(m_data.size() >= readSize);
	memcpy(&val, &m_data[m_readOffset], sizeof(val));
	m_readOffset += sizeof(val);
	return val;
}
float ReplicationPacket::readf() {
	uint32_t nval;
	size_t readSize = m_readOffset + sizeof(nval);
	assert(m_data.size() >= readSize);
	memcpy(&nval, &m_data[m_readOffset], sizeof(nval));
	m_readOffset += sizeof(nval);
	float val = ntohf(nval);
	return val;
}
uint32_t ReplicationPacket::readu32() {
	uint32_t nval;
	size_t readSize = m_readOffset + sizeof(nval);
	assert(m_data.size() >= readSize);
	memcpy(&nval, &m_data[m_readOffset], sizeof(nval));
	m_readOffset += sizeof(nval);
	uint32_t val = ntohl(nval);
	return val;
}
glm::vec3 ReplicationPacket::readVec3() {
	glm::vec3 val;
	val.x = readf();
	val.y = readf();
	val.z = readf();
	return val;
}
glm::vec4 ReplicationPacket::readVec4() {
	glm::vec4 val;
	val.x = readf();
	val.y = readf();
	val.z = readf();
	val.w = readf();
	return val;
}
glm::mat4 ReplicationPacket::readMat4() {
	glm::mat4 val;
	val[0] = readVec4();
	val[1] = readVec4();
	val[2] = readVec4();
	val[3] = readVec4();
	return val;
}

void ReplicationPacket::pack(char* buf) {
	packGeneralData(buf, eReplication);
	/* data */
	uint32_t* uintBuf = reinterpret_cast<uint32_t*>(buf);
	uintBuf[0] = htonl(type);
	uintBuf[1] = htonl(classId);
	uintBuf[2] = htonl(status);
	uint64_t* idBuf = reinterpret_cast<uint64_t*>(&uintBuf[3]);
	idBuf[0] = htonll(objectId);
	buf = reinterpret_cast<char*>(&idBuf[1]);
	if(m_data.size() > 0)
		memcpy(buf, m_data.data(), m_data.size());
}

void ReplicationPacket::unpackData(const char* buf, uint32_t size) {
	const uint32_t* uintBuf = reinterpret_cast<const uint32_t*>(buf);
	type = ntohl(uintBuf[0]);
	classId = ntohl(uintBuf[1]);
	status = ntohl(uintBuf[2]);
	const uint64_t* idBuf = reinterpret_cast<const uint64_t*>(&uintBuf[3]);
	objectId = ntohll(idBuf[0]);
	buf = reinterpret_cast<const char*>(&idBuf[1]);
	size_t dataSize = size - sizeof(uint32_t) * 3 - sizeof(Zap::UUID);
	if (dataSize > 0) {
		m_data.resize(dataSize);
		memcpy(m_data.data(), buf, dataSize);
	}
}

// DisconnectPacket
uint32_t DisconnectPacket::dataSize() {
	return sizeof(uint64_t);
}

void DisconnectPacket::pack(char* buf) {
	packGeneralData(buf, eDisconnect);
	/* data */
	reinterpret_cast<uint64_t*>(buf)[0] = htonll(id); buf += sizeof(id);
}

void DisconnectPacket::unpackData(const char* buf, uint32_t size) {
	id = ntohll(reinterpret_cast<const uint64_t*>(buf)[0]); buf += sizeof(id);
}
