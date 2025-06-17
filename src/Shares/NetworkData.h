#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <vector>

struct NetworkData {
	std::mutex mNetwork;
	std::string username = "user"; // this username serves as an id for the client
	std::string port = "12525";
	std::string ip = "zap.internet-box.ch";

	std::mutex mServer;
	std::vector<std::string> playerList;

	// server specific
	const int backlog = 10;
};