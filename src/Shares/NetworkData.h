#pragma once

#include <string>

struct NetworkData {
	std::string username = "user"; // this username serves as an id for the client
	std::string port = "12525";
	std::string ip = "127.0.0.1";

	// server specific
	int backlog = 10;
};