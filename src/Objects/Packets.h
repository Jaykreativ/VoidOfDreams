#pragma once

#include "SockUitls.h"

#include "Zap/UUID.h"

#include "glm.hpp"

#include <string>
#include <memory>

#define UDP_PACKET_BUFFER_SIZE 1472

enum PacketType {
	eHello = 1,
	eWelcome = 2,
	eReplication = 3,
	eDisconnect = 4,
};

class Packet {
public:
	// general data
	// all packets have this data
	/* ... */

	// send this packet to the specified socket
	// socket has to be a stream socket or a connected dgram socket
	void sendTo(int socket, int flags = 0);

	// send this packet to the specified socket
	// socket has to be a dgram socket
	void sendToDgram(int socket, const sockaddr* addr, int flags = 0);

	// receive a packet from the specified socket
	// socket has to be a stream socket or a connected dgram socket
	static std::shared_ptr<Packet> receiveFrom(int& type, int socket, int flags = 0);

	// receive a packet from the specified socket
	// socket has to be a dgram socket
	// the address that sent the received packet will be written to addr with the size of adrrlen
	static std::shared_ptr<Packet> receiveFromDgram(int& type, int socket, sockaddr* addr, int* addrlen, int flags = 0);

protected:
	uint32_t fullSize();

	static uint32_t headerSize();

	uint32_t generalDataSize();

	virtual uint32_t dataSize() = 0;

	// packs just the header
	// takes the buffer which contains the network package
	void packHeader(char* buf, const int type);

	// takes just the header
	// returns the size of the data stored in the packet and the type of packet
	static void unpackHeader(const char* buf, uint32_t& size, int& type);

	// packs data present in all packets including the header
	// automatically moves the pointer
	void packGeneralData(char*& buf, const int type);
	
	// unpacks data present in all packets including the header
	// returns the size of the data stored in the packet and the type of packet
	// automatically moves the pointer
	void unpackGeneralData(const char*& buf);

	// takes the pointer to a buffer and fills it with the packed packet
	// should use packHeader
	virtual void pack(char* buf) = 0;

	// takes the pointer to the data part and fills the package with data
	virtual void unpackData(const char* buf, uint32_t size) = 0;
};

//class MessagePacket : public Packet {
//	friend class Packet;
//public:
//	// data
//	std::string id = "";
//	std::string msg = "";
//
//protected:
//	uint32_t dataSize();
//
//	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
//	void pack(char* buf);
//
//	// takes just the data part
//	void unpackData(const char* buf, uint32_t size);
//};

class HelloPacket : public Packet {
	friend class Packet;
public:
	// data
	Zap::UUID id;
	std::string username;

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class WelcomePacket : public Packet {
	friend class Packet;
public:
	// data
	bool fromDgram;

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class ReplicationPacket : public Packet {
	friend class Packet;
public:
	enum Type {
		eCREATE,
		eUPDATE,
		eDESTROY
	};

	void write(bool data);
	void write(float data);
	void write(uint32_t data);
	void write(glm::vec3 data);
	void write(glm::vec4 data);
	void write(glm::mat4 data);

	bool readb();
	float readf();
	uint32_t readu32();
	glm::vec3 readVec3();
	glm::vec4 readVec4();
	glm::mat4 readMat4();

	//data
	uint32_t type;
	uint32_t classId = 0;
	uint32_t status = 0;
	Zap::UUID objectId = 0;

protected:
	size_t m_readOffset = 0;

	// data
	std::vector<char> m_data = {};

	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class DisconnectPacket : public Packet {
	friend class Packet;
public:
	// data
	Zap::UUID id;

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};