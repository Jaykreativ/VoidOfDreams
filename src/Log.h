#pragma once

namespace log {
	void initLog();

	void terminateLog();

	void beginRegion(std::string name);

	void endRegion();

	std::string getCurrentRegion();

	const std::stack<std::string, std::vector<std::string>>& getRegionStack();
}
