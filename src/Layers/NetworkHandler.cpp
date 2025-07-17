#include "NetworkHandler.h"

#include <chrono>

#define SERVER_BACKLOG 10

#define CLIENT_HELLO_FREQUENCY_S 0.5f

NetworkHandler::NetworkHandler() {}

NetworkHandler::~NetworkHandler() {}

bool NetworkHandler::isValid() {
	return m_isValid;
}

void NetworkHandler::terminateThreads() {
	{
		std::lock_guard<std::mutex> lk(m_mFlags);
		m_isValid = false; // invalidate object after destruction
	}

	if (m_loopThread.joinable())
		m_loopThread.join();
	if (m_recvLoopThread.joinable())
		m_recvLoopThread.join();
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

void NetworkHandler::recvIncoming(int socketStream) {
	int type;
	auto spPacket = Packet::receiveFrom(type, socketStream);

	std::lock_guard<std::mutex> lk(m_mIncomingPackets);
	m_incomingPackets.push_back({ false, type, spPacket, socketStream });
}

void NetworkHandler::recvIncomingDgram(int socketDgram) {
	sockaddr_storage addr;
	int addrlen = sizeof(sockaddr_storage);

	int type;
	auto spPacket = Packet::receiveFromDgram(type, socketDgram, reinterpret_cast<sockaddr*>(&addr), &addrlen);

	std::lock_guard<std::mutex> lk(m_mIncomingPackets);
	m_incomingPackets.push_back({ true, type, spPacket, 0, addr });
}

NetworkHandlerServer::NetworkHandlerServer(uint16_t port)
	: NetworkHandler()
{
	setupServerSocket(port);

	m_loopThread = std::thread(&NetworkHandlerServer::loop, this);
	m_recvLoopThread = std::thread(&NetworkHandlerServer::recvLoop, this);
}

NetworkHandlerServer::~NetworkHandlerServer() {
	terminateThreads();
	assertSocket(sock::closeSocket(m_serverSocket.stream), "closeSocket(serverSocket.stream)");
	assertSocket(sock::closeSocket(m_serverSocket.dgram), "closeSocket(serverSocket.dgram)");
	for(auto& clientSocket : m_clientSocketMap)
		assertSocket(sock::closeSocket(clientSocket.second.stream), "closeSocket(clientSocket.stream)");
	m_clientSocketMap.clear();

	m_pollfds.clear();
}

void NetworkHandlerServer::handleIncomingPackets() {
	std::lock_guard<std::mutex> lk(m_mIncomingPackets);
	for (IncomingPacket inPacket : m_incomingPackets) {
		switch (inPacket.type)
		{
		case PacketType::eHello: {
			auto id = reinterpret_cast<HelloPacket*>(inPacket.spPacket.get())->id;
			WelcomePacket welcome;
			welcome.fromDgram = inPacket.isDgram;
			if (inPacket.isDgram) {
				m_clientSocketMap[id].addr = inPacket.addr;
				welcome.sendToDgram(m_serverSocket.dgram, reinterpret_cast<sockaddr*>(&inPacket.addr));
				printf("register dgram address\n");
			}
			else {
				m_clientSocketMap[id].stream = inPacket.streamSocket;
				welcome.sendTo(inPacket.streamSocket);
				printf("register stream socket\n");
			}
			
			break;
		}
		default:
			break;
		}
	}
	m_incomingPackets.clear();
}

void NetworkHandlerServer::loop() {
	while (shouldRunThreads()) {
		Sleep(1);
		handleIncomingPackets();
	}
}

void NetworkHandlerServer::acceptClient() {
	sockaddr_storage clientAddr;
	socklen_t addrSize = sizeof clientAddr;

	SocketData socketData;
	assertSocket((socketData.stream = accept(m_serverSocket.stream, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize)), "accept");

	pollfd clientPollfd; // only for stream clients
	clientPollfd.fd = socketData.stream;
	clientPollfd.events = POLLIN;
	clientPollfd.revents = 0;
	m_pollfds.push_back(clientPollfd); // add pollfd

	printf("client connected: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&clientAddr)).c_str());
}

//void NetworkHandlerServer::disconnectClient(int index) { TODO disconnect the client if no disconnect packet was sent, cleanup with looping through the map and searching for the invalid client manually
//	SocketData socket = m_clientSocketMap[index];
//	printf("client disconnected: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&socket.addr)).c_str());
//
//	if (sock::closeSocket(socket.stream) < 0) {
//		sock::printLastError("close(stream)");
//		exit(sock::lastError());
//	}
//
//	m_clientSocketMap.erase(m_clientSocketMap.begin() + index); // delete the clients socket data
//	m_pollfds.erase(m_pollfds.begin() + index + 1); // delete the clients pollfd, +1 for the server pollfd
//	m_eraseOffset++;
//}

void NetworkHandlerServer::handlePoll(int pollCount) {
	int checkedPollCount = 0;

	pollfd serverStreamPollfd = m_pollfds[0];
	if (serverStreamPollfd.revents & POLLIN) { // accept client
		acceptClient();
		checkedPollCount++;
	}
	pollfd serverDgramPollfd = m_pollfds[1];
	if (serverDgramPollfd.revents & POLLIN) { // recvClientDgram
		int type = 0;
		recvIncomingDgram(m_serverSocket.dgram);
		checkedPollCount++;
	}

	// go through all clients
	// i is the index of the client in clientSockets
	m_eraseOffset = 0; // offset the index by the times erase was used as erase shifts all remaining indices by -1
	for (size_t i = 0; i < m_pollfds.size() - 2; i++) { // go through all client sockets
		pollfd poll = m_pollfds[i + 2]; // +2 for the 2 server sockets
		if (poll.revents & POLLHUP) {
			//disconnectClient(i - m_eraseOffset);
		}
		if (poll.revents & POLLIN) {
			recvIncoming(poll.fd);
		}

		if (poll.revents & (POLLIN | POLLHUP)) // add checked if poll had events
			checkedPollCount++;
		if (checkedPollCount >= pollCount) // return when all polls are checked
			return;
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
	: NetworkHandler(), m_id()
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

	m_pollStream.fd = m_serverSocket.stream;
	m_pollStream.events = POLLIN;
	m_pollStream.revents = 0;
	m_pollDgram.fd = m_serverSocket.dgram;
	m_pollDgram.events = POLLIN;
	m_pollDgram.revents = 0;

	m_loopThread = std::thread(&NetworkHandlerClient::loop, this);
	m_recvLoopThread = std::thread(&NetworkHandlerClient::recvLoop, this);
}

NetworkHandlerClient::~NetworkHandlerClient() {
	terminateThreads();
	assertSocket(sock::closeSocket(m_serverSocket.stream), "closeSocket(serverSocket.stream)");
	assertSocket(sock::closeSocket(m_serverSocket.dgram), "closeSocket(serverSocket.dgram)");
}

bool NetworkHandlerClient::isFullyConnected() {
	std::lock_guard<std::mutex> lk(m_mConnectionStatus);
	return m_isDgramRegistered && m_isStreamRegistered;
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

void NetworkHandlerClient::handleIncomingPackets() {
	std::lock_guard<std::mutex> lk(m_mIncomingPackets);
	for (IncomingPacket inPacket : m_incomingPackets) {
		switch (inPacket.type)
		{
		case PacketType::eWelcome: {
			std::lock_guard<std::mutex> lk(m_mConnectionStatus);
			if (reinterpret_cast<WelcomePacket*>(inPacket.spPacket.get())->fromDgram) {
				if(!m_isDgramRegistered) {
					m_isDgramRegistered = true;
					printf("dgram fully registered\n");
				}
			}
			else {
				if(!m_isStreamRegistered) {
					m_isStreamRegistered = true;
					printf("stream fully registered\n");
				}
			}
			break;
		}
		default:
			printf("unexpected packet type being processed (%i)\n", inPacket.type);
			break;
		}
	}
	m_incomingPackets.clear();
}

void NetworkHandlerClient::loop() {
	std::chrono::steady_clock::time_point lastSent = std::chrono::high_resolution_clock::now();
	HelloPacket hello; // send hello over tcp and udp
	hello.id = m_id;
	hello.sendTo(m_serverSocket.stream);
	hello.sendToDgram(m_serverSocket.dgram, reinterpret_cast<sockaddr*>(&m_serverSocket.addr));

	while (shouldRunThreads()) {
		handleIncomingPackets();

		if (isFullyConnected()) {

		}
		else {
			auto now = std::chrono::high_resolution_clock::now();
			float sinceLastSent = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastSent).count();
			if (sinceLastSent >= CLIENT_HELLO_FREQUENCY_S) { // resend hello if there was no answer from the server
				HelloPacket hello; // send hello over tcp and udp
				hello.id = m_id;
				hello.sendTo(m_serverSocket.stream);
				hello.sendToDgram(m_serverSocket.dgram, reinterpret_cast<sockaddr*>(&m_serverSocket.addr));
				lastSent = std::chrono::high_resolution_clock::now();
			}
		}
	}
}

void NetworkHandlerClient::handlePoll(int pollCount) {
	if (m_pollStream.revents & POLLIN) { // received a tcp packet
		recvIncoming(m_serverSocket.stream);
	}
	if (m_pollDgram.revents & POLLIN) { // received a udp packet
		recvIncomingDgram(m_serverSocket.dgram);
	}
}

void NetworkHandlerClient::recvLoop() {
	while (shouldRunThreads()) {
		pollfd pollfds[2] = { m_pollStream, m_pollDgram };
		int pollCount = sock::pollState(pollfds, 2, 100); // fetch events of the given pollfds
		if (pollCount == 0)
			continue;
		assertSocket(pollCount, "poll");
		m_pollStream = pollfds[0];
		m_pollDgram = pollfds[1];

		handlePoll(pollCount);
	}
}