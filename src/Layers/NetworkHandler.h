#pragma once

#include "SockUitls.h"

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

class NetworkHandler {
public:
	NetworkHandler();
	virtual ~NetworkHandler();

	bool isValid(); 

protected:
	bool m_isValid = true;

	// multithreading
	std::mutex m_mFlags;

	bool shouldRunThreads();

	void assertSocket(int val, std::string msg);
};

class NetworkHandlerServer : public NetworkHandler {
public:
	NetworkHandlerServer(uint16_t port);
	~NetworkHandlerServer();

	// the file descriptors used in the poll command
	// (#0:tcp server)
	// (#1:udp server)
	// index can be converted to corresponding clientSocket index by -2
	std::vector<pollfd> m_pollfds = {};

private:
	SocketData m_serverSocket = {};
	std::vector<SocketData> m_clientSockets = {}; // stores clientsockets to send the worldstate to

	void setupServerSocket(uint16_t port);

	std::thread m_loopThread;
	void loop();

	void acceptClient();

	void handlePoll(int pollCount);

	std::thread m_recvLoopThread;
	void recvLoop();
};

class NetworkHandlerClient : public NetworkHandler {
public:
	NetworkHandlerClient(std::string ip, uint16_t port);

private:
	SocketData m_serverSocket = {};

	void setupServerSocket(int family, int protocol, sockaddr* addr, int addrlen);

	void recvLoop();
};