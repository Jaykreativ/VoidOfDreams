#pragma once

#include <string>

struct NetworkData {
	std::string username = "user";
	std::string port = "12525";
	std::string ip = "127.0.0.1";

	// server specific
	int backlog = 10;
};