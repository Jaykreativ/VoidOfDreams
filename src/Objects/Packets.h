#pragma once

#include "SockUitls.h"

#include "glm.hpp"

#include <string>
#include <memory>

#define UDP_PACKET_BUFFER_SIZE 1472

enum PacketType {
	eMESSAGE = 1,
	eCONNECT = 2,
	eDISCONNECT = 3,
	eMOVE = 4,
	eDamage = 5,
	eSpawn = 6,
	eDeath = 7,
	eUDP_CONNECT = 8,
	eRay = 100
};

class Packet {
public:
	// general data
	// all packets have this data
	std::string username = "";

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

class ConnectPacket : public Packet {
	friend class Packet;
public:
	// data

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class UDPConnectPacket : public Packet {
	friend class Packet;
public:
	// data

protected:
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

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class MovePacket : public Packet {
	friend class Packet;
public:
	// data
	glm::mat4 transform = glm::mat4(1);

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class DamagePacket : public Packet {
	friend class Packet;
public:
	//data
	std::string usernameDamager = "";
	float damage = 0;
	float health = 0;

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class SpawnPacket : public Packet {
	friend class Packet;
public:
	//data

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class DeathPacket : public Packet {
	friend class Packet;
public:
	//data
	std::string usernameKiller = "";

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

// Item Packetsgit 
class RayPacket : public Packet {
	friend class Packet;
public:
	//data
	glm::vec3 origin = { 0, 0, 0 };
	glm::vec3 direction = { 0, 0, 0 };

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};
