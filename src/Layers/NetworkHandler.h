#pragma once

#include "SockUitls.h"
#include "Objects/Packets.h"

#include "Zap/UUID.h"

#include <cstdint>
#include <string>
#include <mutex>
#include <thread>

struct SocketData { // combine the socket and its address into one type, cause they're always needed when using both tcp and udp.
	int stream = -1;
	int dgram = -1;
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

struct IncomingPacket {
	bool isDgram;
	int type;
	std::shared_ptr<Packet> spPacket;
	int streamSocket; // if isDgram is true this socket is invalid
	sockaddr_storage addr; // if isDgram is false this address is invalid
};

class NetworkHandler {
public:
	NetworkHandler();
	virtual ~NetworkHandler();

	bool isValid(); 

protected:
	bool m_isValid = true;

	// multithreading
	std::mutex m_mFlags;


	std::mutex m_mIncomingPackets;
	std::vector<IncomingPacket> m_incomingPackets = {};

	void terminateThreads();

	bool shouldRunThreads();

	void assertSocket(int val, std::string msg);

	void recvIncoming(int socketStream);

	void recvIncomingDgram(int socketDgram);

	std::thread m_loopThread;
	virtual void loop() = 0;

	std::thread m_recvLoopThread;
	virtual void recvLoop() = 0;
};

class NetworkHandlerServer : public NetworkHandler {
public:
	NetworkHandlerServer(uint16_t port);
	~NetworkHandlerServer();

private:
	// the file descriptors used in the poll command
	// (#0:tcp server)
	// (#1:udp server)
	// index can be converted to corresponding clientSocket index by -2
	std::vector<pollfd> m_pollfds = {};

	SocketData m_serverSocket = {};
	std::unordered_map<Zap::UUID, SocketData> m_clientSocketMap = {}; // stores clientsockets to send the worldstate to

	uint32_t m_eraseOffset = 0;

	void setupServerSocket(uint16_t port);

	void handleIncomingPackets();

	void loop();

	void acceptClient();

	//void disconnectClient(int index);

	void handlePoll(int pollCount);

	void recvLoop();
};

class NetworkHandlerClient : public NetworkHandler {
public:
	NetworkHandlerClient(std::string ip, uint16_t port);
	~NetworkHandlerClient();

	// thread safe function
	// returns true if the client is fully registered by the server
	bool isFullyConnected();
private:
	Zap::UUID m_id;

	std::mutex m_mConnectionStatus;
	bool m_isDgramRegistered = false; // connection status
	bool m_isStreamRegistered = false;

	pollfd m_pollDgram = {};
	pollfd m_pollStream = {};

	SocketData m_serverSocket = {};

	void setupServerSocket(int family, int protocol, sockaddr* addr, int addrlen);

	void handleIncomingPackets();

	void loop();

	void handlePoll(int pollCount);

	void recvLoop();
};