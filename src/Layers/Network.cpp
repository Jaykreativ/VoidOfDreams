#include "Network.h"

#include "SockUitls.h"
#include "Shares/NetworkData.h"
#include "Layers/Game.h"
#include "Objects/Packets.h"
#include "Objects/Weapons/Ray.h"

#include "glm.hpp"

#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include <cstring>
#include <stdio.h>

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

		int socklen = sizeof(sockaddr_storage);
		sockaddr_storage clientAddr;
		if (getsockname(_serverSocket.stream, reinterpret_cast<sockaddr*>(&clientAddr), &socklen) < 0) {
			sock::printLastError("getsockname(dgram)");
			return false;
		}
		printf("client address: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&clientAddr)).c_str());

		if (bind(_serverSocket.dgram, reinterpret_cast<sockaddr*>(&clientAddr), socklen) < 0) { // bind the udp socket to the same address as the stream socket
			sock::printLastError("bind(dgram)");
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
		sock::closeSocket(_serverSocket.stream);
		sock::closeSocket(_serverSocket.dgram);

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
			case eCONNECT: {
				ConnectPacket& packet = *reinterpret_cast<ConnectPacket*>(spPacket.get());
				std::lock_guard<std::mutex> lk(world.mPlayers);
				if (packet.username == network.username) { // set this clients player
					setupLocalPlayer(world, packet.username);
				}
				else {
					setupExternalPlayer(world, packet.username);
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
			case eDamage: {
				DamagePacket& packet = *reinterpret_cast<DamagePacket*>(spPacket.get());
				std::lock_guard<std::mutex> lk(world.mPlayers);
				if (world.players.count(packet.username)) {
					auto spPlayer = world.players.at(packet.username);
					spPlayer->syncDamage(packet.damage, packet.health);
				}
				break;
			}
			case eRay: {
				RayPacket& packet = *reinterpret_cast<RayPacket*>(spPacket.get());
				std::lock_guard<std::mutex> lk(world.mPlayers);
				if (std::shared_ptr<Player> spPlayer = world.pPlayer.lock()) {
					Ray::processRay(packet.origin, packet.direction, world, *spPlayer, *world.players.at(packet.username));
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

	void sendPlayerMove(Player& player) {
		std::lock_guard<std::mutex> lk(_mTerminate);
		if(_isConnected) {
			MovePacket packet;
			packet.username = player.getUsername();
			packet.transform = player.getTransform();
			packet.sendToDgram(_serverSocket.dgram, reinterpret_cast<const sockaddr*>(&_serverSocket.addr));
		}
	}

	void sendRay(glm::vec3 origin, glm::vec3 direction, std::string username) {
		std::lock_guard<std::mutex> lk(_mTerminate);
		if(_isConnected) {
			RayPacket packet;
			packet.username = username;
			packet.origin = origin;
			packet.direction = direction;
			packet.sendTo(_serverSocket.stream);
		}
	}

	void sendDamage(float damage, Player& player, std::string username) {
		std::lock_guard<std::mutex> lk(_mTerminate);
		if (_isConnected) {
			DamagePacket packet;
			packet.username = username;
			packet.damage = damage;
			packet.health = player.getHealth();
			packet.sendTo(_serverSocket.stream);
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
	std::vector<SocketData> _clients = {};
	int _eraseOffset = 0; // used in disconnect soeckt because 

	// the file descriptors used in the poll command
	// (#0:tcp server)
	// (#1:udp server)
	// index can be converted to corresponding clientSocket index by -2
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

		printf("client connected: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&clientAddr)).c_str());
	}

	void disconnectClient(int index) {
		SocketData socket = _clients[index];
		printf("client disconnected: %s\n", sock::addrToPresentation(reinterpret_cast<sockaddr*>(&socket.addr)).c_str());

		if (sock::closeSocket(socket.stream) < 0) {
			sock::printLastError("close(stream)");
			exit(sock::lastError());
		}

		_clients.erase(_clients.begin() + index); // delete the clients socket data
		_pollfds.erase(_pollfds.begin() + index + 1); // delete the clients pollfd, +1 for the server pollfd
		_eraseOffset++;
	}

	void handlePacket(SocketData& socket, std::shared_ptr<Packet> spPacket, int type, int clientIndex) {
		if (!spPacket.get()) {
			disconnectClient(clientIndex);
			return;
		}
		switch (type)
		{
		//case eMESSAGE: {
		//	MessagePacket& packet = *reinterpret_cast<MessagePacket*>(spPacket.get());
		//	printf("server msg: %s (%s)\n", packet.msg.c_str(), packet.id.c_str());
		//	for (int clientSocket : clientStorage.sockets)
		//		packet.sendTo(clientSocket);
		//	break;
		//}
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
		case eDamage: { // uses stream sockets
			DamagePacket& packet = *reinterpret_cast<DamagePacket*>(spPacket.get());
			for (auto clientSocket : _clients) {
				if (packet.username != clientSocket.username)
					packet.sendTo(clientSocket.stream);
			}
			break;
		}
		case eRay: { // uses strem sockets
			RayPacket& packet = *reinterpret_cast<RayPacket*>(spPacket.get());
			for (auto clientSocket : _clients) {
				if (packet.username != clientSocket.username)
					packet.sendTo(clientSocket.stream);
			}
			break;
		}
		default: {
			break;
		}
		}
	}

	void recvClientDgram(int clientIndex) {
		sockaddr_storage addr;
		int addrlen = sizeof(sockaddr_storage);

		int type;
		auto spPacket = Packet::receiveFromDgram(type, _serverSocket.dgram, reinterpret_cast<sockaddr*>(&addr), &addrlen);
		SocketData addrOnly;
		addrOnly.addr = addr;
		handlePacket(addrOnly, spPacket, type, clientIndex); // for dgram packets only their origin address is known while the sockets are unknown
	}

	void recvClient(SocketData& socket, int clientIndex) {
		int type;
		auto spPacket = Packet::receiveFrom(type, socket.stream);
		handlePacket(socket, spPacket, type, clientIndex);
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
			recvClientDgram(-1);
			checkedPollCount++;
		}

		// go through all clients
		// i is the index of the client in clientSockets
		int eraseOffset = 0; // offset the index by the times erase was used as erase shifts all remaining indices by -1
		for (size_t i = 0; i < _pollfds.size()-2; i++) { // go through all client sockets
			pollfd poll = _pollfds[i+2]; // +2 for the 2 server sockets
			if (poll.revents & POLLHUP) {
				disconnectClient(i - eraseOffset);
			}
			if (poll.revents & POLLIN) {
				recvClient(_clients[i - eraseOffset], i - eraseOffset);
			}
		
			if (poll.revents & (POLLIN | POLLHUP)) // add checked if poll had events
				checkedPollCount++;
			if (checkedPollCount >= pollCount) // return when all polls are checked
				return;
		}
	}

	// free all resources
	void freeResources() {
		if (sock::closeSocket(_serverSocket.stream) == -1)
			sock::printLastError("Server close(serverSocket.stream)");
		if (sock::closeSocket(_serverSocket.dgram) == -1)
			sock::printLastError("Server close(serverSocket.dgram)");
		for (const auto& socket : _clients)
			if (sock::closeSocket(socket.stream) == -1)
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