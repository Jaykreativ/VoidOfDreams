#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <vector>

enum ClientErrorActionBits {
	eTERMINATE_CLIENT = 0x1,
	eSWITCH_MAIN_MENU = 0x2
};
typedef uint32_t ClientErrorAction;

struct ClientError {
	ClientErrorAction actions;
	std::string description = "No error description available";
};

struct NetworkData {
	std::mutex mNetwork;
	std::string username = "user"; // this username serves as an id for the client
	std::string port = "12525";
	std::string ip = "zap.internet-box.ch";

	std::mutex mClient;

	std::vector<ClientError> clientErrorStack = {};

	std::mutex mServer;
	std::vector<std::string> playerList;

	// server specific
	const int backlog = 10;
};