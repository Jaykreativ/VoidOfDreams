#include "NetworkHandler.h"

#include "Shares/World.h"
#include "Objects/Player.h"

#include <chrono>

#define SERVER_BACKLOG 10

#define CLIENT_HELLO_FREQUENCY_S 0.1f

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

bool NetworkHandlerServer::ClientData::isFullyConnected() {
	return m_isDgramConnected && m_isStreamConnected;
}

void NetworkHandlerServer::ClientData::connectionMade(bool isDgram) {
	m_isDgramConnected |= isDgram;
	m_isStreamConnected |= !isDgram;
}

NetworkHandlerServer::NetworkHandlerServer(uint16_t port)
	: NetworkHandler()
{
	Zap::SceneDesc desc{};
	desc.gravity = { 0, 0, 0 };
	m_world.spScene = std::make_shared<Zap::Scene>();
	m_world.spScene->init(desc);

	setupServerSocket(port);

	m_loopThread = std::thread(&NetworkHandlerServer::loop, this);
	m_recvLoopThread = std::thread(&NetworkHandlerServer::recvLoop, this);
}

NetworkHandlerServer::~NetworkHandlerServer() {
	terminateThreads();
	assertSocket(sock::closeSocket(m_serverSocket.stream), "closeSocket(serverSocket.stream)");
	assertSocket(sock::closeSocket(m_serverSocket.dgram), "closeSocket(serverSocket.dgram)");
	for(auto& clientSocket : m_clients)
		assertSocket(sock::closeSocket(clientSocket.second.socket.stream), "closeSocket(clientSocket.stream)");
	m_clients.clear();

	m_pollfds.clear();

	m_world.spScene->destroy();
	printf("server terminated\n");
}

void NetworkHandlerServer::sendToAll(Packet& packet) {
	for(auto clientPair : m_clients)
		packet.sendTo(clientPair.second.socket.stream);
}
void NetworkHandlerServer::sendToAllDgram(Packet& packet) {
	for(auto clientPair : m_clients)
		packet.sendToDgram(m_serverSocket.dgram, reinterpret_cast<const sockaddr*>(&clientPair.second.socket.addr));
}

void NetworkHandlerServer::handleHelloPacket(IncomingPacket& inPacket) {
	HelloPacket* packet = reinterpret_cast<HelloPacket*>(inPacket.spPacket.get());
	auto id = packet->id;
	m_clients[id].connectionMade(inPacket.isDgram);
	auto username = packet->username;
	m_clients[id].username = username;

	// send a welcome packet to the new client
	WelcomePacket welcome;
	welcome.fromDgram = inPacket.isDgram;
	if (inPacket.isDgram) {
		m_clients[id].socket.addr = inPacket.addr;
		welcome.sendToDgram(m_serverSocket.dgram, reinterpret_cast<sockaddr*>(&inPacket.addr));
		printf("register dgram address\n");
	}
	else {
		m_clients[id].socket.stream = inPacket.streamSocket;
		welcome.sendTo(inPacket.streamSocket);
		printf("register stream socket\n");
	}
}

void NetworkHandlerServer::handleIncomingPackets() {
	std::lock_guard<std::mutex> lk(m_mIncomingPackets);
	for (IncomingPacket inPacket : m_incomingPackets) {
		switch (inPacket.type)
		{
		case PacketType::eHello: {
			handleHelloPacket(inPacket);
			break;
		}
		default:
			break;
		}
	}
	m_incomingPackets.clear();
}

void NetworkHandlerServer::loop() {
	int count = 0;
	while (shouldRunThreads()) {
		Sleep(50);
		handleIncomingPackets();

		for (auto& clientPair : m_clients) {
			auto id = clientPair.first;
			auto& client = clientPair.second;

			if (client.isFullyConnected()) {
				if (m_world.players.count(id)) {
					glm::mat4 mat(1);
					mat[3] = glm::vec4(sin(count++/5.f+id%1000)*5, 0, 0, 1);
					m_world.players.at(id)->setTransform(mat);
					auto spPacket = m_replicationManager.replicateUpdate(m_world.players.at(id).get(), 0);
					sendToAll(*spPacket);
				}
				else { // when the client has no corresponding player, create a new player
					auto spPlayer = std::make_shared<PlayerServer>(*m_world.spScene);
					m_world.players[id] = spPlayer;
					//spPlayer->getInventory().setItem(std::make_shared<Ray>(m_world), 0);
					//spPlayer->getInventory().setItem(std::make_shared<SimpleTrigger>(ImGuiMouseButton_Left), 1);
					//spPlayer->getInventory().setItem(std::make_shared<Dash>(), 2);
					//spPlayer->getInventory().setItem(std::make_shared<SimpleTrigger>(ImGuiKey_LeftShift), 3);
					spPlayer->spawn();
					auto spPacket = m_replicationManager.replicateCreate(m_world.players.at(id).get());
					sendToAll(*spPacket);
				}
			}
		}
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

NetworkHandlerClient::NetworkHandlerClient(std::string ip, uint16_t port, std::string username)
	: NetworkHandler(), m_id(), m_username(username)
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

	printf("client terminated\n");
}

bool NetworkHandlerClient::isFullyConnected() {
	std::lock_guard<std::mutex> lk(m_mConnectionStatus);
	return m_isDgramRegistered && m_isStreamRegistered;
}

void NetworkHandlerClient::replicateWorldState(WorldDataClient& world) {
	std::lock_guard<std::mutex> lk(m_mReplicationManager);
	m_replicationManager.processReplication(world);
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

void NetworkHandlerClient::handleWelcomePacket(IncomingPacket& inPacket) {
	std::lock_guard<std::mutex> lk(m_mConnectionStatus);
	if (reinterpret_cast<WelcomePacket*>(inPacket.spPacket.get())->fromDgram) {
		if (!m_isDgramRegistered) {
			m_isDgramRegistered = true;
			printf("dgram fully registered\n");
		}
	}
	else {
		if (!m_isStreamRegistered) {
			m_isStreamRegistered = true;
			printf("stream fully registered\n");
		}
	}

}

void NetworkHandlerClient::handleReplicationPacket(IncomingPacket& inPacket) {
	std::lock_guard<std::mutex> lk(m_mReplicationManager);
	m_replicationManager.addReplication(std::dynamic_pointer_cast<ReplicationPacket>(inPacket.spPacket));
}

void NetworkHandlerClient::handleIncomingPackets() {
	std::lock_guard<std::mutex> lk(m_mIncomingPackets);
	for (IncomingPacket& inPacket : m_incomingPackets) {
		switch (inPacket.type)
		{
		case PacketType::eWelcome: {
			handleWelcomePacket(inPacket);
			break;
		}
		case PacketType::eReplication: {
			handleReplicationPacket(inPacket);
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
	hello.username = m_username;
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