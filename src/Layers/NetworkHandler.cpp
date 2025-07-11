#include "NetworkHandler.h"

NetworkHandler::NetworkHandler() {}

bool NetworkHandler::isValid() {
	return m_isValid;
}

void NetworkHandler::assertSocket(int val, std::string msg) {
	if (val == -1) {
		sock::printLastError(msg.c_str());
		m_isValid = false;
	}
}

NetworkHandlerServer::NetworkHandlerServer(uint16_t port)
	: NetworkHandler()
{}

NetworkHandlerClient::NetworkHandlerClient(std::string ip, uint16_t port)
	: NetworkHandler()
{
	addrinfo hints;
	addrinfo* serverInfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	int status;
	if ((status = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &serverInfo)) != 0) { // get adress from ip and port
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		m_isValid = false;
	}

	if (!isValid()) // retrieve address failed
		return;

	for (addrinfo* p = serverInfo; p != nullptr; p = p->ai_next) { // print the ip to which the client is connecting
		if (p->ai_family == AF_INET) {
			printf("connecting to IPv4 Address: %s\n", sock::addrToPresentationIPv4(reinterpret_cast<sockaddr_in*>(p->ai_addr)->sin_addr).c_str());
		}
		if (p->ai_family == AF_INET6) {
			printf("connecting to IPv6 Address: %s\n", sock::addrToPresentationIPv6(reinterpret_cast<sockaddr_in6*>(p->ai_addr)->sin6_addr).c_str());
		}
	}

	setupServerSocket(serverInfo->ai_family, serverInfo->ai_protocol, serverInfo->ai_addr, serverInfo->ai_addrlen);

	freeaddrinfo(serverInfo);
}

void NetworkHandlerClient::setupServerSocket(int family, int protocol, sockaddr* addr, int addrlen) {
	m_serverSocket.stream = socket(family, SOCK_STREAM, protocol);
	assertSocket(m_serverSocket.stream, "socket(stream)");
	m_serverSocket.dgram = socket(family, SOCK_DGRAM, protocol);
	assertSocket(m_serverSocket.dgram, "socket(dgram)");
	m_serverSocket.addr = *reinterpret_cast<sockaddr_storage*>(addr);

	if (!isValid())
		return;

	assertSocket(connect(m_serverSocket.stream, addr, addrlen), "connect(stream)");
}