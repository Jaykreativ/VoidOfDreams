#pragma once

#include <string>

struct NetworkData {
	std::string username = "user"; // this username serves as an id for the client
	std::string port = "12525";
	std::string ip = "zap.internet-box.ch";

	// server specific
	int backlog = 10;
};