#include "Network.h"

#include "Shares/NetworkData.h"

#include "glm.hpp"

#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include <cstring>
#include <stdio.h>

#ifdef _WIN32 // windows specific socket include

#include <WinSock2.h>
#include <WS2tcpip.h>

#elif __linux__

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

typedef in_addr IN_ADDR;
typedef in6_addr IN6_ADDR;

#endif 

#define UDP_PACKET_BUFFER_SIZE 1472

namespace sock {
	int close(int socket) {
#ifdef _WIN32
		return closesocket(socket);
#elif __linux__
		return close(socket);
#endif
	}

	int pollState(pollfd fds[], size_t nfds, int timeout) {
#ifdef _WIN32
		return WSAPoll(fds, nfds, timeout);
#elif __linux__
		return poll(fds, nfds, timeout);
#endif
	}

	IN_ADDR presentationToAddrIPv4(std::string presentation) {
		IN_ADDR addr;
		if (inet_pton(AF_INET, presentation.c_str(), &addr) <= 0) {
			fprintf(stderr, "error while decoding ip address\n");
			exit(3);
		}
		return addr;
	}
	IN6_ADDR presentationToAddrIPv6(std::string presentation) {
		IN6_ADDR addr;
		if (inet_pton(AF_INET6, presentation.c_str(), &addr) <= 0) {
			fprintf(stderr, "error while decoding ip address\n");
			exit(3);
		}
		return addr;
	}

	std::string addrToPresentationIPv4(IN_ADDR addr) {
		char ip4[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addr, ip4, INET_ADDRSTRLEN);
		return std::string(ip4);
	}
	std::string addrToPresentationIPv6(IN6_ADDR addr) {
		char ip6[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &addr, ip6, INET6_ADDRSTRLEN);
		return ip6;
	}
	std::string addrToPresentation(sockaddr* sa) {
		if (sa->sa_family == AF_INET) {
			return addrToPresentationIPv4(reinterpret_cast<sockaddr_in*>(sa)->sin_addr);
		}
		return addrToPresentationIPv6(reinterpret_cast<sockaddr_in6*>(sa)->sin6_addr);
	}

	int cmpAddr(const sockaddr* a, const sockaddr* b) {
		if (a->sa_family != b->sa_family)
			return -1;

		if (a->sa_family == AF_INET) {
			sockaddr_in sa4a = *reinterpret_cast<const sockaddr_in*>(a);
			sockaddr_in sa4b = *reinterpret_cast<const sockaddr_in*>(b);
			return *(uint32_t*)(&sa4a.sin_addr) - *(uint32_t*)(&sa4b.sin_addr) +
				sa4a.sin_port - sa4b.sin_port;
		}
		else if (a->sa_family == AF_INET6) {
			sockaddr_in6 sa6a = *reinterpret_cast<const sockaddr_in6*>(a);
			sockaddr_in6 sa6b = *reinterpret_cast<const sockaddr_in6*>(b);
			return memcmp((char*)&sa6a, (char*)&sa6b, sizeof(sockaddr_in6));
		}
		else {
			printf("cmpAddr: unsupported address family %i\n", (int)a->sa_family);
			exit(0);
		}
	}

	int lastError() {
#ifdef _WIN32
		return WSAGetLastError();
#elif __linux__
		return errno;
#endif
	}

	void printLastError(const char* msg) {
#ifdef _WIN32
		char* s = NULL;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&s, 0, NULL);
		fprintf(stderr, "%s: %s\n", msg, s);
#elif __linux__
		perror(msg);
#endif
	}
}

void htonMat4(const glm::mat4& mat4, void* nData) {
	uint32_t* buf = reinterpret_cast<uint32_t*>(nData);
	for (size_t i = 0; i < 16; i += 1) {
		uint32_t nf = htonf(mat4[floor(i/4)][i%4]);
		buf[i] = nf;
	}
}

void ntohMat4(const void* nData, glm::mat4& mat4) {
	const uint32_t* buf = reinterpret_cast<const uint32_t*>(nData);
	for (size_t i = 0; i < 16; i += 1) {
		float hf = ntohf(buf[i]);
		mat4[floor(i / 4)][i % 4] = hf;
	}
}

struct SocketData { // combine the socket and its address into one type, cause they're always needed when using both tcp and udp.
	std::string username;
	int stream;
	int dgram;
	sockaddr_storage addr; // the udp address

	sockaddr* getAddr() {
		return reinterpret_cast<sockaddr*>(&addr);
	}

	// returns 0 if equal
	int compAddr(const SocketData& socket) {
		auto* saa = reinterpret_cast<const sockaddr*>(&addr);
		auto* sab = reinterpret_cast<const sockaddr*>(&socket.addr);
		return sock::cmpAddr(saa, sab);
	}
};

/* Packets */

enum PacketType {
	eUDPInit = 1,
	eMESSAGE = 2,
	eCONNECT = 3,
	eDISCONNECT = 4,
	eMOVE = 5
};

class Packet {
public:
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

	virtual uint32_t dataSize() = 0;

	// packs just the header
	// takes the buffer which contains the network package
	void packHeader(char* buf, const int type);

	// takes just the header
	// returns the size of the data stored in the packet
	static void unpackHeader(const char* buf, uint32_t& size, int& type);

	// takes the pointer to a buffer and fills it with the packed packet
	// should use packHeader
	virtual void pack(char* buf) = 0;

	// takes the pointer to the data part and fills the package with data
	virtual void unpackData(const char* buf, uint32_t size) = 0;
};

// used by the server to store this clients dgram address for future udp communication
class UDPInitPacket : public Packet {
	friend class Packet;
public:
	// data
	std::string username = "";

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class MessagePacket : public Packet {
	friend class Packet;
public:
	// data
	std::string id = "";
	std::string msg = "";

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

class ConnectPacket : public Packet {
	friend class Packet;
public:
	// data
	std::string username = "";

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
	std::string username = "";

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
	std::string username = "";
	glm::mat4 transform = glm::mat4(1);

protected:
	uint32_t dataSize();

	// packs the data into the given buffer, buffer needs to have the same size as packet.fullSize()
	void pack(char* buf);

	// takes just the data part
	void unpackData(const char* buf, uint32_t size);
};

// Packet
void Packet::sendTo(int socket, int flags) {
	uint32_t len = fullSize();
	char* buf = new char[len];
	pack(buf);

	uint32_t offset = 0;
	while (offset < len) {
		int bytesSent = send(socket, buf + offset, len - offset, 0);
		if (bytesSent == -1) {
			sock::printLastError("Packet::send");
			delete[] buf;
			return;
		}
		offset += bytesSent;
	}
	delete[] buf;
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
		spPacket->unpackData(buf, dataSize);
		break;
	}
	case eDISCONNECT: {
		spPacket = std::make_shared<DisconnectPacket>();
		spPacket->unpackData(buf, dataSize);
		break;
	}
	case eMOVE: {
		spPacket = std::make_shared<MovePacket>();
		spPacket->unpackData(buf, dataSize);
		break;
	}
	default:
		break;
	}
	delete[] buf;

	return spPacket;
}

std::shared_ptr<Packet> Packet::receiveFromDgram(int& type, int socket, sockaddr* addr, int* addrlen, int flags) {
	char buf[UDP_PACKET_BUFFER_SIZE];
	int bytesRead = recvfrom(socket, buf, UDP_PACKET_BUFFER_SIZE, 0, addr, addrlen); // get just header
	if (bytesRead == -1) {
		sock::printLastError("Packet::recvfrom");
		return nullptr;
	}
	char* ptr = buf;
	uint32_t dataSize;
	unpackHeader(ptr, dataSize, type);
	ptr += headerSize();

	std::shared_ptr<Packet> spPacket;
	switch (type)
	{
		//case eMESSAGE: {
		//	spPacket = std::make_shared<MessagePacket>();
		//	spPacket->unpackData(ptr, dataSize);
		//	break;
		//}
	case eUDPInit: {
		spPacket = std::make_shared<UDPInitPacket>();
		spPacket->unpackData(ptr, dataSize);
		break;
	}
	case eCONNECT: {
		spPacket = std::make_shared<ConnectPacket>();
		spPacket->unpackData(ptr, dataSize);
		break;
	}
	case eDISCONNECT: {
		spPacket = std::make_shared<DisconnectPacket>();
		spPacket->unpackData(ptr, dataSize);
		break;
	}
	case eMOVE: {
		spPacket = std::make_shared<MovePacket>();
		spPacket->unpackData(ptr, dataSize);
		break;
	}
	default:
		break;
	}

	return spPacket;

}

uint32_t Packet::fullSize() {
		return headerSize() + dataSize();
	}

uint32_t Packet::headerSize() {
		return 2 * sizeof(uint32_t);
	}

void Packet::packHeader(char* buf, const int type) {
		uint32_t* uintBuf = reinterpret_cast<uint32_t*>(buf);
		uintBuf[0] = htonl(dataSize());
		uintBuf[1] = htonl(type);
	}

void Packet::unpackHeader(const char* buf, uint32_t& size, int& type) {
		const uint32_t* uintBuf = reinterpret_cast<const uint32_t*>(buf);
		size = ntohl(uintBuf[0]);
		type = ntohl(uintBuf[1]);
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
	return username.size();
}

void ConnectPacket::pack(char* buf) {
	packHeader(buf, eCONNECT); buf += headerSize();
	/* data */
	memcpy(buf, username.data(), username.size());
}

void ConnectPacket::unpackData(const char* buf, uint32_t size) {
	username = std::string(buf, size);
}

// UDPInitPacket
uint32_t UDPInitPacket::dataSize() {
	return username.size();
}

void UDPInitPacket::pack(char* buf) {
	packHeader(buf, eUDPInit); buf += headerSize();
	/* data */
	memcpy(buf, username.data(), username.size());
}

void UDPInitPacket::unpackData(const char* buf, uint32_t size) {
	username = std::string(buf, size);
}

// DisconnectPacket
uint32_t DisconnectPacket::dataSize() {
	return username.size();
}

void DisconnectPacket::pack(char* buf) {
	packHeader(buf, eDISCONNECT); buf += headerSize();
	/* data */
	memcpy(buf, username.data(), username.size());
}

void DisconnectPacket::unpackData(const char* buf, uint32_t size) {
	username = std::string(buf, size);
}

// MovePacket
uint32_t MovePacket::dataSize() {
	return sizeof(uint32_t) + username.size() + sizeof(glm::mat4);
}

void MovePacket::pack(char* buf) {
	packHeader(buf, eMOVE); buf += headerSize();
	/* data */
	uint32_t usernameSize = htonl(username.size());
	memcpy(buf, &usernameSize, sizeof(uint32_t)); buf += sizeof(uint32_t);
	memcpy(buf, username.data(), username.size()); buf += username.size();
	htonMat4(transform, buf);
}

void MovePacket::unpackData(const char* buf, uint32_t size) {
	uint32_t usernameSize = ntohl(reinterpret_cast<const uint32_t*>(buf)[0]); buf += sizeof(uint32_t);
	username = std::string(buf, usernameSize); buf += usernameSize;
	ntohMat4(buf, transform);
}

namespace client {
	std::mutex _mTerminate; // controls access to variables for terminating the client

	bool _isRunning = false;
	bool _isConnected = false; // used by threads outisde the client to determine if client is connected

	volatile bool _shouldStop = false;
	//std::thread sender;
	std::thread _receiver;

	SocketData _serverSocket;
	const size_t _pollfdCount = 2;
	pollfd _pollfds[_pollfdCount] = {};

	bool isRunning() {
		std::lock_guard<std::mutex> lk(_mTerminate);
		return _isRunning;
	}

	// initializes resources for client networking
	// returns false on failure
	bool start(NetworkData& network) {
		if (client::_isRunning)
			return true;
		client::_isRunning = true;

		addrinfo hints;
		addrinfo* serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;

		int status;
		if ((status = getaddrinfo(network.ip.c_str(), network.port.c_str(), &hints, &serverInfo)) != 0) { // get adress from ip and port
			fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
			return false;
		}

		for (addrinfo* p = serverInfo; p != nullptr; p = p->ai_next) { // print the ip that the client is connecting to
			if (p->ai_family == AF_INET) {
				printf("connecting to IPv4 Address: %s\n", sock::addrToPresentationIPv4(reinterpret_cast<sockaddr_in*>(p->ai_addr)->sin_addr).c_str());
			}
			if (p->ai_family == AF_INET6) {
				printf("connecting to IPv6 Address: %s\n", sock::addrToPresentationIPv6(reinterpret_cast<sockaddr_in6*>(p->ai_addr)->sin6_addr).c_str());
			}
		}

		_serverSocket.stream = socket(serverInfo->ai_family, SOCK_STREAM, serverInfo->ai_protocol);
		if (_serverSocket.stream < 0) {
			sock::printLastError("socket(stream)");
			return false;
		}
		_serverSocket.dgram = socket(serverInfo->ai_family, SOCK_DGRAM, serverInfo->ai_protocol);
		if (_serverSocket.dgram < 0) {
			sock::printLastError("socket(dgram)");
			return false;
		}
		_serverSocket.addr = *reinterpret_cast<sockaddr_storage*>(serverInfo->ai_addr);

		if (connect(_serverSocket.stream, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) { // connect to the server specified by the ip in network
			sock::printLastError("connect(stream)");
			return false;
		}
		_isConnected = true;

		//if (bind(_serverSocket.dgram, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) { // bind dgram socket to same port as 
		//	sock::printLastError("bind");
		//	exit(sock::lastError());
		//}

		_pollfds[0].fd = _serverSocket.stream; // init poll stream
		_pollfds[0].events = POLLIN;
		_pollfds[0].revents = 0;
		_pollfds[1].fd = _serverSocket.dgram; // init poll dgram
		_pollfds[1].events = POLLIN;
		_pollfds[1].revents = 0;

		freeaddrinfo(serverInfo);

		ConnectPacket connectPacket;
		connectPacket.username = network.username;
		connectPacket.sendTo(_serverSocket.stream);

		UDPInitPacket udpInitPacket;
		udpInitPacket.username = network.username;
		udpInitPacket.sendToDgram(_serverSocket.dgram, _serverSocket.getAddr());

		return true;
	}

	void stop(NetworkData& network, WorldData& world) {
		if (!client::_isRunning)
			return;
		client::_isRunning = false;

		if (_isConnected) {
			DisconnectPacket disconnectPacket;
			disconnectPacket.username = network.username;
			disconnectPacket.sendTo(_serverSocket.stream);
			_isConnected = false;
		}
		sock::close(_serverSocket.stream);
		sock::close(_serverSocket.dgram);

		{ // delete all players as the client is being terminated
			std::lock_guard<std::mutex> lk(world.mPlayers);
			world.players.clear();
		}

		printf("client done\n");
	}

	// terminates the client from a client thread
	void terminateInternal(NetworkData& network, WorldData& world) {
		std::lock_guard<std::mutex> lk(_mTerminate);
		_shouldStop = true;
		_receiver.detach();
		stop(network, world);
	}

	void handlePacket(NetworkData& network, WorldData& world, std::shared_ptr<Packet> spPacket, int type) {
		if (spPacket) { // check for receive failure
			switch (type)
			{
				//case eMESSAGE: {
				//	MessagePacket& packet = *reinterpret_cast<MessagePacket*>(spPacket.get());
				//	printf("client msg: %s (%s)\n", packet.msg.c_str(), packet.id.c_str());
				//	break;
				//}
			case eUDPInit: {
				UDPInitPacket& packet = *reinterpret_cast<UDPInitPacket*>(spPacket.get());
				printf("failed to init udp, retry!\n");
				packet.sendToDgram(_serverSocket.dgram, _serverSocket.getAddr());
				break;
			}
			case eCONNECT: {
				ConnectPacket& packet = *reinterpret_cast<ConnectPacket*>(spPacket.get());
				std::lock_guard<std::mutex> lk(world.mPlayers);
				world.players[packet.username] = std::make_shared<Player>(*world.scene); // add newly connected player
				if (packet.username == network.username) { // set this clients player
					world.pPlayer = world.players.at(packet.username);
				}
				break;
			}
			case eDISCONNECT: {
				DisconnectPacket& packet = *reinterpret_cast<DisconnectPacket*>(spPacket.get());
				std::lock_guard<std::mutex> lk(world.mPlayers);
				world.players.erase(packet.username); // delete the disconnected player
				break;
			}
			case eMOVE: {
				MovePacket& packet = *reinterpret_cast<MovePacket*>(spPacket.get());
				std::lock_guard<std::mutex> lk(world.mPlayers);
				if (world.players.count(packet.username)) {
					world.players.at(packet.username)->setTransform(packet.transform);
				}
				else {
					printf("%s cannot be moved because that client isn't connected\n", packet.username.c_str());
				}
				break;
			}
			default:
				break;
			}
		}
	}

	// return false if failed
	bool handlePoll(NetworkData& network, WorldData& world) {
		if (_pollfds[0].revents & POLLHUP) {
			printf("server closed connection\n");
			terminateInternal(network, world);
			return false;
		}
		if (_pollfds[0].revents & POLLIN) {
			int type;
			auto spPacket = Packet::receiveFrom(type, _serverSocket.stream);
			handlePacket(network, world, spPacket, type);
		}

		if (_pollfds[1].revents & POLLIN) {
			sockaddr_storage addr;
			socklen_t addrlen = sizeof(sockaddr_storage);
			int type;
			auto spPacket = Packet::receiveFromDgram(type, _serverSocket.dgram, reinterpret_cast<sockaddr*>(&addr), &addrlen);
			handlePacket(network, world, spPacket, type);
		}
		return true;
	}

	void receiverLoop(NetworkData network, WorldData* world) {
		while (true) {
			{ // stop
				std::lock_guard<std::mutex> lk(_mTerminate);
				if (_shouldStop)
					break;
			}

			int didPoll = sock::pollState(_pollfds, _pollfdCount, 100);
			if (didPoll == -1) {
				sock::printLastError("Client poll");
				exit(sock::lastError());
			}
			if (didPoll) {
				if (!handlePoll(network, *world)) {
					break;
				}
			}
		}
	}

	//void senderLoop(NetworkData network, WorldData* world) {
	//	while (true) {
	//		//MessagePacket packet;
	//		//packet.id = network.username;
	//		//printf(">> ");
	//		//getline(std::cin, packet.msg);
	//		{
	//			std::lock_guard<std::mutex> lk(world->mPlayers);
	//			if (std::shared_ptr<Player> spPlayer = world->pPlayer.lock()) {
	//				MovePacket packet;
	//				packet.username = network.username;
	//				packet.transform = spPlayer->getTransform();
	//				packet.sendTo(serverSocket);
	//			}
	//		}
	//		{ // stop
	//			std::lock_guard<std::mutex> lk(mTerminate);
	//			if (shouldStop)
	//				break;
	//		}
	//		//packet.sendTo(serverSocket);
	//	}
	//}

	void sendPlayerMove(NetworkData& network, WorldData& world) {
		std::lock_guard<std::mutex> lk(_mTerminate);
		if(_isConnected) if (std::shared_ptr<Player> spPlayer = world.pPlayer.lock()) {
			MovePacket packet;
			packet.username = network.username;
			packet.transform = spPlayer->getTransform();
			packet.sendToDgram(_serverSocket.dgram, reinterpret_cast<const sockaddr*>(&_serverSocket.addr));
		}
	}
}

bool runClient(NetworkData& network, WorldData& world) {
	if (!client::start(network)) {
		client::stop(network, world);
		return false;
	}
	client::_shouldStop = false;
	//client::sender = std::thread(client::senderLoop, network, &world);
	client::_receiver = std::thread(client::receiverLoop, network, &world);
	return true;
}

void terminateClient(NetworkData& network, WorldData& world) {
	{
		std::lock_guard<std::mutex> lk(client::_mTerminate);
		client::_shouldStop = true;
	}
	//client::sender.join();
	if(client::_receiver.joinable())
		client::_receiver.join();
	client::stop(network, world);
	client::_shouldStop = false;
}

namespace server {
	bool _isRunning = false;
	std::mutex _mTerminate; // controls access to variables for terminating the server
	volatile bool _shouldStop = false;
	std::thread _thread;

	SocketData _serverSocket;
	// this stores all current users
	// it's used for sending a new player all current players and for identifying clients
	std::vector<SocketData> _clients = {};
	int _eraseOffset = 0; // used in disconnect soeckt because 

	// the file descriptors used in the poll command
	// (#0:tcp server)
	// (#1:udp server)
	// index can be converted to corresponding clientSocket index by -1
	std::vector<pollfd> _pollfds = {};

	bool isRunning() {
		std::lock_guard<std::mutex> lk(_mTerminate);
		return _isRunning;
	}

	void acceptClient() {
		sockaddr_storage clientAddr;
		socklen_t addrSize = sizeof clientAddr;

		SocketData socketData;
		if ((socketData.stream = accept(_serverSocket.stream, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize)) == -1) {
			sock::printLastError("accept");
			exit(sock::lastError());
		}
		socketData.addr = clientAddr;
		_clients.push_back(socketData);

		pollfd clientPollfd; // only for stream clients
		clientPollfd.fd = socketData.stream;
		clientPollfd.events = POLLIN;
		clientPollfd.revents = 0;
		_pollfds.push_back(clientPollfd); // add pollfd, client will be inserted into the map when receiving the connect packet

		printf("client connected: %s:%u\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&clientAddr)).c_str(), reinterpret_cast<sockaddr_in*>(&clientAddr)->sin_port);
	}

	void disconnectClient(int index) {
		SocketData socket = _clients[index];
		printf("client disconnected: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&socket.addr)).c_str());

		if (sock::close(socket.stream) < 0) {
			sock::printLastError("close(stream)");
			exit(sock::lastError());
		}

		_clients.erase(_clients.begin() + index); // delete the clients socket data
		_pollfds.erase(_pollfds.begin() + index + 1); // delete the clients pollfd, +1 for the server pollfd
		_eraseOffset++;
	}

	void handlePacket(SocketData& socket, std::shared_ptr<Packet> spPacket, int type) {
		switch (type)
		{
			//case eMESSAGE: {
			//	MessagePacket& packet = *reinterpret_cast<MessagePacket*>(spPacket.get());
			//	printf("server msg: %s (%s)\n", packet.msg.c_str(), packet.id.c_str());
			//	for (int clientSocket : clientStorage.sockets)
			//		packet.sendTo(clientSocket);
			//	break;
			//}
		case eUDPInit: {
			UDPInitPacket& packet = *reinterpret_cast<UDPInitPacket*>(spPacket.get());
			
			bool found = false;
			for (auto& client : _clients) {
				if (client.username == packet.username) {
					client.addr = socket.addr;
					found = true;
				}
			}
			if (!found) { // send back the packet if failed
				printf("failed to init udp for %s\n", packet.username.c_str());
				packet.sendToDgram(_serverSocket.dgram, reinterpret_cast<sockaddr*>(&socket.addr));
			}
			else
				printf("init udp for %s\n", packet.username.c_str());
			break;
		}
		case eCONNECT: { // uses stream sockets
			ConnectPacket& packet = *reinterpret_cast<ConnectPacket*>(spPacket.get());
			{
				bool nameTaken = false;
				for (const auto& client : _clients)
					if (client.username == packet.username) {
						nameTaken = true;
						break;
					}
				if (nameTaken) {
					printf("%s already present, wont be accepted\n", packet.username.c_str());
					int i = 0;
					for (const auto& comp : _clients) {
						if (comp.stream == socket.stream) {
							disconnectClient(i);
						}
						i++;
					}
					break;
				}
			} // prevent multiple usernames

			socket.username = packet.username;
			printf("%s joined the server\n", packet.username.c_str());

			for (auto clientSocket : _clients) {
				packet.sendTo(clientSocket.stream); // tell all clients(including the new one) that a new player joined
				if (clientSocket.stream != socket.stream) {
					ConnectPacket connectPacket;
					connectPacket.username = clientSocket.username;
					connectPacket.sendTo(socket.stream); // send the new client all clients that where already present
				}
			}

			break;
		}
		case eDISCONNECT: { // uses stream sockets
			DisconnectPacket& packet = *reinterpret_cast<DisconnectPacket*>(spPacket.get());

			{
				bool nameTaken = false;
				for (const auto& client : _clients)
					if (client.username == packet.username) {
						nameTaken = true;
						break;
					}
				if (!nameTaken) {
					printf("%s not present, already disconnected\n", packet.username.c_str());
					break;
				}
			} // prevent multiple disconnects

			printf("%s left the server\n", packet.username.c_str());
			for (auto clientSocket : _clients) {
				if (clientSocket.stream != socket.stream)
					packet.sendTo(clientSocket.stream);
			}
			break;
		}
		case eMOVE: { // uses dgram sockets
			MovePacket& packet = *reinterpret_cast<MovePacket*>(spPacket.get());
			for (auto clientSocket : _clients) {
				if (packet.username != clientSocket.username)
					packet.sendToDgram(_serverSocket.dgram, reinterpret_cast<const sockaddr*>(&clientSocket.addr));
			}
			break;
		}
		default: {
			break;
		}
		}
	}

	void recvClientDgram() {
		sockaddr_storage addr;
		int addrlen = sizeof(sockaddr_storage);

		int type;
		auto spPacket = Packet::receiveFromDgram(type, _serverSocket.dgram, reinterpret_cast<sockaddr*>(&addr), &addrlen);
		SocketData addrOnly;
		addrOnly.addr = addr;
		handlePacket(addrOnly, spPacket, type); // for dgram packets only their origin address is known while the sockets are unknown
	}

	void recvClient(SocketData& socket) {
		int type;
		auto spPacket = Packet::receiveFrom(type, socket.stream);
		handlePacket(socket, spPacket, type);
	}

	void handlePoll(int pollCount) {
		if (pollCount == 0) // return if there are no polls
			return;

		int checkedPollCount = 0;

		pollfd serverStreamPollfd = _pollfds[0];
		if (serverStreamPollfd.revents & POLLIN) { // accept client
			acceptClient();
			checkedPollCount++;
		}
		pollfd serverDgramPollfd = _pollfds[1];
		if (serverDgramPollfd.revents & POLLIN) { // recvClientDgram
			recvClientDgram();
			checkedPollCount++;
		}

		// go through all clients
		// i is the index of the client in clientSockets
		int eraseOffset = 0; // offset the index by the times erase was used as erase shifts all remaining indices by -1
		for (size_t i = 0; i < _pollfds.size()-2; i++) { // go through all client sockets
			pollfd poll = _pollfds[i+2]; // +2 for the 2 server sockets
			if (poll.revents & POLLIN) {
				recvClient(_clients[i - eraseOffset]);
			}
			if (poll.revents & POLLHUP) {
				disconnectClient(i - eraseOffset);
			}
		
			if (poll.revents & (POLLIN | POLLHUP)) // add checked if poll had events
				checkedPollCount++;
			if (checkedPollCount >= pollCount) // return when all polls are checked
				return;
		}
	}

	// free all resources
	void freeResources() {
		if (sock::close(_serverSocket.stream) == -1)
			sock::printLastError("Server close(serverSocket.stream)");
		if (sock::close(_serverSocket.dgram) == -1)
			sock::printLastError("Server close(serverSocket.dgram)");
		for (const auto& socket : _clients)
			if (sock::close(socket.stream) == -1)
				sock::printLastError("Server close(clientSocket)");

		_pollfds.clear();
		_clients.clear();
	}

	SocketData getServerSocket(NetworkData& network) {
		SocketData socketData;

		addrinfo hints;
		addrinfo* serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_flags = AI_PASSIVE;

		int status;
		if ((status = getaddrinfo(NULL, network.port.c_str(), &hints, &serverInfo)) != 0) { // turn the port into a full address, ip is null because its a server
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
			exit(sock::lastError());
		}

		// creating the socket
		if ((socketData.stream = socket(serverInfo->ai_family, SOCK_STREAM, serverInfo->ai_protocol)) < 0) {
			sock::printLastError("socket");
			exit(sock::lastError());
		}
		if ((socketData.dgram = socket(serverInfo->ai_family, SOCK_DGRAM, serverInfo->ai_protocol)) < 0) {
			sock::printLastError("socket");
			exit(sock::lastError());
		}

		// enable port reuse
		const char yes = 1;
		if (setsockopt(socketData.stream, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
			sock::printLastError("setsockopt");
		}
		if (setsockopt(socketData.dgram, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
			sock::printLastError("setsockopt");
		}

		// bind to port
		if (bind(socketData.stream, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) {
			sock::printLastError("bind");
			exit(sock::lastError());
		}
		if (bind(socketData.dgram, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) {
			sock::printLastError("bind");
			exit(sock::lastError());
		}

		if (listen(socketData.stream, network.backlog) < 0) {
			sock::printLastError("listen");
			exit(sock::lastError());
		}

		socketData.addr = *reinterpret_cast<sockaddr_storage*>(serverInfo->ai_addr);

		pollfd serverStreamPollfd; // make a poll fd for the server socket, gets an event when a new client connects
		serverStreamPollfd.fd = socketData.stream;
		serverStreamPollfd.events = POLLIN;
		serverStreamPollfd.revents = 0;
		_pollfds.push_back(serverStreamPollfd);
		pollfd serverDgramPollfd; // make a poll fd for the server socket, gets an event when a clientSocketDgram sends data
		serverDgramPollfd.fd = socketData.dgram;
		serverDgramPollfd.events = POLLIN;
		serverDgramPollfd.revents = 0;
		_pollfds.push_back(serverDgramPollfd);

		freeaddrinfo(serverInfo);

		return socketData;
	}

	void loop(NetworkData network) {
		_serverSocket = getServerSocket(network);
		printf("server running\n");

		while (true) {
			{
				std::lock_guard<std::mutex> lk(_mTerminate);
				if (_shouldStop) { 
					break;
				}
			}

			int pollCount = sock::pollState(_pollfds.data(), _pollfds.size(), 100);// fetch events of the given pollfds
			if (pollCount == 0)
				continue;

			if (pollCount == -1) {
				sock::printLastError("poll");
				exit(sock::lastError());
			}

			handlePoll(pollCount);
		}

		freeResources();

		printf("server done\n");
	}
}

void runServer(NetworkData network) {
	if (server::_isRunning)
		return;
	server::_isRunning = true;

	server::_shouldStop = false;
	server::_thread = std::thread(server::loop, network);
}

void terminateServer() {
	if (!server::_isRunning)
		return;
	{
		std::lock_guard<std::mutex> lk(server::_mTerminate);
		server::_shouldStop = true;
	}
	server::_thread.join();
	server::_shouldStop = false;
	server::_isRunning = false;
}