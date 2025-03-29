#include "Network.h"

#include "Shares/NetworkData.h"

#include "glm.hpp"

#include <thread>
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
			delete buf;
			return;
		}
		uint32_t dataSize = unpackHeader(buf);
		delete buf;

		buf = new char[dataSize];
		bytesRead = recv(socket, buf, dataSize, 0); // get just data
		if (bytesRead == -1) {
			sock::printLastError("Packet::recv data");
			delete buf;
			return;
		}
		unpackData(buf, dataSize);
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
			packet.sendTo(serverSocket);
			if (packet.msg.compare("esc") == 0)
				break;
		}

		sock::close(serverSocket);

		printf("client done\n");
	}
}

void runClient(NetworkData network) {
	client::thread = std::thread(client::loop, network);
}

void terminateClient() {
	client::thread.join();
}

namespace server {
	std::thread thread;

	struct Player {
		float transform[16] = {0};
	};

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
			exit(1);
		}

		// creating the socket
		int serverSocket;
		if ((serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) < 0) {
			sock::printLastError("socket");
			exit(2);
		}

		// bind to port
		if (bind(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) {
			sock::printLastError("bind");
			exit(4);
		}

		if (listen(serverSocket, network.backlog) < 0) {
			sock::printLastError("listen");
			exit(5);
		}

		freeaddrinfo(serverInfo);

		return serverSocket;
	}

	void loop(NetworkData network) {
		int serverSocket = getServerSocket(network);

		sockaddr_storage clientAddr; // accept client
		socklen_t addrSize = sizeof clientAddr;
		int clientSocket;
		if ((clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize)) < 0) {
			sock::printLastError("accept");
			exit(errno);
		}
		printf("Client connected: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&clientAddr)).c_str());

		while (true) {
			Packet packet = Packet::receiveFrom(clientSocket);
			if (packet.msg.compare("esc") == 0)
				break;
			printf("message received: %s\n", packet.msg.c_str());
		}

		printf("server done\n");
	}
}

void runServer(NetworkData network) {
	server::thread = std::thread(server::loop, network);
}

void terminateServer() {
	server::thread.join();
}