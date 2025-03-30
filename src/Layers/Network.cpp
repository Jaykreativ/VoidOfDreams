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

const int msgSize = 50;

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

class Packet {
public:
	// data
	std::string msg = "";

	void sendTo(int socket, int flags = 0) {
		uint32_t len = fullSize();
		char* buf = new char[len];
		pack(buf);

		uint32_t offset = 0;
		while (offset < len) {
			int bytesSent = send(socket, buf+offset, len-offset, 0);
			if (bytesSent == -1) {
				sock::printLastError("Packet::send");
				delete buf;
				return;
			}
			offset += bytesSent;
		}
	}

	static Packet receiveFrom(int socket, int flags = 0) {
		Packet packet;
		packet.receive(socket, flags);
		return packet;
	}

private:
	uint32_t fullSize() {
		return sizeof(uint32_t) + msg.size();
	}

	uint32_t size() {
		return msg.size();
	}

	// packs the data into the given buffer, buffer needs to have the same size as packet.size()
	void pack(char* buf) {
		uint32_t offset = 0;
		uint32_t nsize = htonl(size()); // the header of the packet
		/* header */
		memcpy(buf + offset, &nsize, sizeof(uint32_t)); offset += sizeof(uint32_t);
		/* data */
		memcpy(buf + offset, msg.data(), msg.size());   offset += msg.size();
	}

	void receive(int socket, int flags) {
		char* buf = new char[sizeof(uint32_t)];
		int bytesRead = recv(socket, buf, sizeof(uint32_t), 0); // get just header
		if (bytesRead == -1) {
			sock::printLastError("Packet::recv header");
			delete[] buf;
			return;
		}
		uint32_t dataSize = unpackHeader(buf);
		delete[] buf;

		buf = new char[dataSize];
		bytesRead = recv(socket, buf, dataSize, 0); // get just data
		if (bytesRead == -1) {
			sock::printLastError("Packet::recv data");
			delete[] buf;
			return;
		}
		unpackData(buf, dataSize);
		delete[] buf;
	}

	// takes just the header
	// returns the size of the data stored in the packet
	uint32_t unpackHeader(const char* buf) {
		const uint32_t* uintBuf = reinterpret_cast<const uint32_t*>(buf);
		return ntohl(uintBuf[0]);
	}

	// takes just the data part
	void unpackData(const char* buf, uint32_t size) {
		msg = std::string(buf, size);
	}
};

namespace client {
	std::mutex mTerminate; // controls access to variables for terminating the client
	volatile bool shouldStop = false;
	std::thread thread;

	void loop(NetworkData network) {
		addrinfo hints;
		addrinfo* serverInfo;
		int serverSocket;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		int status;
		if ((status = getaddrinfo(network.ip.c_str(), network.port.c_str(), &hints, &serverInfo)) != 0) { // get adress from ip and port
			fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		}

		for (addrinfo* p = serverInfo; p != nullptr; p = p->ai_next) { // print the ip that the client is connecting to
			if (p->ai_family == AF_INET) {
				printf("connecting to IPv4 Address: %s\n", sock::addrToPresentationIPv4(reinterpret_cast<sockaddr_in*>(p->ai_addr)->sin_addr).c_str());
			}
			if (p->ai_family == AF_INET6) {
				printf("connecting to IPv6 Address: %s\n", sock::addrToPresentationIPv6(reinterpret_cast<sockaddr_in6*>(p->ai_addr)->sin6_addr).c_str());
			}
		}

		serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		if (serverSocket < 0) {
			sock::printLastError("socket");
		}

		if (connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) { // connect to the server specified by the ip in network
			sock::printLastError("connect");
		}

		freeaddrinfo(serverInfo);

		while (true) {
			Packet packet;
			printf(">> ");
			getline(std::cin, packet.msg);

			{ // stop
				std::lock_guard<std::mutex> lk(mTerminate);
				if (shouldStop)
					break;
			}

			packet.sendTo(serverSocket);
		}

		sock::close(serverSocket);

		printf("client done\n");
	}
}

void runClient(NetworkData network) {
	client::thread = std::thread(client::loop, network);
}

void terminateClient() {
	{
		std::lock_guard<std::mutex> lk(client::mTerminate);
		client::shouldStop = true;
	}
	client::thread.join();
	client::shouldStop = false;
}

namespace server {
	std::mutex mTerminate; // controls access to variables for terminating the server
	volatile bool shouldStop = false;
	std::thread thread;

	struct Player {
		float transform[16] = {0};
	};

	std::vector<int> clientSockets;

	// the file descriptors used in the poll command
	// (#0:server)
	std::vector<pollfd> pollfds;
	int pollTimeout = -1; // no timeout

	enum PollResult {
		eSUCCESS
	};

	void acceptClient(int serverSocket, std::vector<int>& clientSockets, std::vector<pollfd>& pollfds) {
		sockaddr_storage clientAddr;
		socklen_t addrSize = sizeof clientAddr;
		int clientSocket;
		if ((clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize)) < 0) {
			sock::printLastError("accept");
			exit(sock::lastError());
		}
		clientSockets.push_back(clientSocket); // add socket
		pollfd clientPollfd;
		clientPollfd.fd = clientSocket;
		clientPollfd.events = POLLIN;
		clientPollfd.revents = 0;
		pollfds.push_back(clientPollfd); // add pollfd

		printf("Client connected: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&clientAddr)).c_str());
	}

	void recvClient(int socket) {
		Packet packet = Packet::receiveFrom(socket);
		printf("message received: %s\n", packet.msg.c_str());
	}

	void disconnectClient(int socket, int index, std::vector<int>& clientSockets, std::vector<pollfd>& pollfds) {
		sockaddr clientAddr; // find address to print
		socklen_t addrSize = sizeof clientAddr;
		getpeername(socket, &clientAddr, &addrSize);
		printf("Client disconnected: %s", sock::addrToPresentation(&clientAddr).c_str());

		if (sock::close(socket) < 0) {
			sock::printLastError("close");
			exit(sock::lastError());
		}
		clientSockets.erase(clientSockets.begin()+index); // delete the client from the vector
		pollfds.erase(pollfds.begin()+index+1); // delete the clients pollfd, +1 for the server pollfd
	}

	PollResult handlePoll(std::vector<pollfd>& pollfds, int pollCount, std::vector<int>& clientSockets) {
		if (pollCount == 0) // return if there are no polls
			return eSUCCESS;

		int checkedPollCount = 0;

		pollfd serverPollfd = pollfds[0];
		if (serverPollfd.revents & POLLIN) { // accept client
			acceptClient(serverPollfd.fd, clientSockets, pollfds);
			checkedPollCount++;
		}

		// go through all clients
		// i is the index of the client in clientSockets
		int eraseOffset = 0; // offset the index by the times erase was used as erase shifts all remaining indices by -1
		for (int i = 0; i < pollfds.size()-1; i++) {
			pollfd poll = pollfds[i+1];
			if (poll.revents & POLLIN) {
				recvClient(poll.fd);
			}
			if (poll.revents & POLLHUP) {
				disconnectClient(poll.fd, i-eraseOffset, clientSockets, pollfds);
				eraseOffset++;
			}

			if (poll.revents & (POLLIN | POLLHUP)) // add checked if poll had events
				checkedPollCount++;
			if (checkedPollCount >= pollCount) // return when all polls are checked
				return eSUCCESS;
		}
	}

	int getServerSocket(NetworkData& network) {
		addrinfo hints;
		addrinfo* serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		int status;
		if ((status = getaddrinfo(NULL, network.port.c_str(), &hints, &serverInfo)) != 0) { // turn the port into a full address, ip is null because its a server
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
			exit(sock::lastError());
		}

		// creating the socket
		int serverSocket;
		if ((serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) < 0) {
			sock::printLastError("socket");
			exit(sock::lastError());
		}

		// enable port reuse
		const char yes = 1;
		if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
			sock::printLastError("setsockopt");
			exit(sock::lastError());
		}

		// bind to port
		if (bind(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) {
			sock::printLastError("bind");
			exit(sock::lastError());
		}

		if (listen(serverSocket, network.backlog) < 0) {
			sock::printLastError("listen");
			exit(sock::lastError());
		}

		freeaddrinfo(serverInfo);

		return serverSocket;
	}

	void loop(NetworkData network) {
		int serverSocket = getServerSocket(network);
		pollfd serverPollfd; // make a poll fd for the server socket, gets an event when a new client connects
		serverPollfd.fd = serverSocket;
		serverPollfd.events = POLLIN;
		serverPollfd.revents = 0;
		pollfds.push_back(serverPollfd);
		printf("server running\n");

		while (true) {
			{
				std::lock_guard<std::mutex> lk(mTerminate);
				if (shouldStop)
					break;
			}

			int pollCount = sock::pollState(pollfds.data(), pollfds.size(), pollTimeout);// fetch events of the given pollfds
			if (pollCount == -1) {
				sock::printLastError("poll");
				exit(sock::lastError());
			}

			if (handlePoll(pollfds, pollCount, clientSockets) != eSUCCESS)
				printf("handlePoll failed\n");
		}

		if (sock::close(serverSocket) < 0) {
			sock::printLastError("close");
			exit(sock::lastError());
		}
		printf("server done\n");
	}
}

void runServer(NetworkData network) {
	server::thread = std::thread(server::loop, network);
}

void terminateServer() {
	{
		std::lock_guard<std::mutex> lk(server::mTerminate);
		server::shouldStop = true;
	}
	server::thread.join();
	server::shouldStop = false;
}