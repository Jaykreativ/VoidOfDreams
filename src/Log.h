#pragma once

namespace log {
	void initLog();

	void terminateLog();

	void beginRegion(std::string name);

	void endRegion();

	void error(std::string msg);
	void errorRaw(std::string msg);

	void warning(std::string msg);
	void warningRaw(std::string msg);

	void info(std::string msg);
	void infoRaw(std::string msg);

	std::string getCurrentRegion();

	const std::stack<std::string, std::vector<std::string>>& getRegionStack();
}
