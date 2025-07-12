#include "NetworkHandler.h"

#define SERVER_BACKLOG 10

NetworkHandler::NetworkHandler() {}

NetworkHandler::~NetworkHandler() {}

bool NetworkHandler::isValid() {
	return m_isValid;
}

bool NetworkHandler::shouldRunThreads() {
	std::lock_guard<std::mutex> lk(m_mFlags);
	return m_isValid;
}

void NetworkHandler::assertSocket(int val, std::string msg) {
	if (val == -1) {
		sock::printLastError(msg.c_str());
		std::lock_guard<std::mutex> lk(m_mFlags);
		m_isValid = false;
	}
}

NetworkHandlerServer::NetworkHandlerServer(uint16_t port)
	: NetworkHandler()
{
	setupServerSocket(port);

	m_recvLoopThread = std::thread(&NetworkHandlerServer::recvLoop, this);
}

NetworkHandlerServer::~NetworkHandlerServer() {
	{
		std::lock_guard<std::mutex> lk(m_mFlags);
		m_isValid = false; // invalidate object after destruction
	}

	if(m_loopThread.joinable())
		m_loopThread.join();
	if(m_recvLoopThread.joinable())
		m_recvLoopThread.join();

	assertSocket(sock::closeSocket(m_serverSocket.stream), "closeSocket(serverSocket.stream)");
	assertSocket(sock::closeSocket(m_serverSocket.dgram), "closeSocket(serverSocket.dgram)");
	for(auto& clientSocket : m_clientSockets)
		assertSocket(sock::closeSocket(m_serverSocket.stream), "closeSocket(clientSocket.stream)");
	m_clientSockets.clear();

	m_pollfds.clear();
}

void NetworkHandlerServer::loop() {

}

void NetworkHandlerServer::acceptClient() {
	sockaddr_storage clientAddr;
	socklen_t addrSize = sizeof clientAddr;

	SocketData socketData;
	assertSocket((socketData.stream = accept(m_serverSocket.stream, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize)), "accept");
	m_clientSockets.push_back({ socketData });

	pollfd clientPollfd; // only for stream clients
	clientPollfd.fd = socketData.stream;
	clientPollfd.events = POLLIN;
	clientPollfd.revents = 0;
	m_pollfds.push_back(clientPollfd); // add pollfd

	printf("client connected: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&clientAddr)).c_str());
}

void NetworkHandlerServer::handlePoll(int pollCount) {
	int checkedPollCount = 0;

	pollfd serverStreamPollfd = m_pollfds[0];
	if (serverStreamPollfd.revents & POLLIN) { // accept client
		acceptClient();
		checkedPollCount++;
	}
}

void NetworkHandlerServer::recvLoop() {
	while (shouldRunThreads()) {
		int pollCount = sock::pollState(m_pollfds.data(), m_pollfds.size(), 100);// fetch events of the given pollfds
		if (pollCount == 0)
			continue;
		assertSocket(pollCount, "poll");

		handlePoll(pollCount);
	}
}

void NetworkHandlerServer::setupServerSocket(uint16_t port) {
	addrinfo hints;
	addrinfo* serverInfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	int status;
	if ((status = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &serverInfo)) != 0) { // turn the port into a full address, ip is null because its a server
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		m_isValid = false;
	}

	if (!isValid()) // retrieve address failed
		return;

	m_serverSocket.stream = socket(serverInfo->ai_family, SOCK_STREAM, serverInfo->ai_protocol);
	assertSocket(m_serverSocket.stream, "socket(stream)");
	m_serverSocket.dgram = socket(serverInfo->ai_family, SOCK_DGRAM, serverInfo->ai_protocol);
	assertSocket(m_serverSocket.dgram, "socket(dgram)");
	m_serverSocket.addr = *reinterpret_cast<sockaddr_storage*>(serverInfo->ai_addr);

	if (!isValid()) // creating sockets failed
		return;

	assertSocket(bind(m_serverSocket.stream, serverInfo->ai_addr, serverInfo->ai_addrlen), "bind(stream)");
	assertSocket(bind(m_serverSocket.dgram, serverInfo->ai_addr, serverInfo->ai_addrlen), "bind(dgram)");

	if (!isValid()) // binding sockets failed
		return;

	assertSocket(listen(m_serverSocket.stream, SERVER_BACKLOG), "listen");

	if (!isValid()) // listening failed
		return;

	pollfd serverStreamPollfd; // make a poll fd for the server socket, gets an event when a new client connects
	serverStreamPollfd.fd = m_serverSocket.stream;
	serverStreamPollfd.events = POLLIN;
	serverStreamPollfd.revents = 0;
	m_pollfds.push_back(serverStreamPollfd);
	pollfd serverDgramPollfd; // make a poll fd for the server socket, gets an event when a clientSocketDgram sends data
	serverDgramPollfd.fd = m_serverSocket.dgram;
	serverDgramPollfd.events = POLLIN;
	serverDgramPollfd.revents = 0;
	m_pollfds.push_back(serverDgramPollfd);

	freeaddrinfo(serverInfo);
}

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

void NetworkHandlerClient::recvLoop() {

}