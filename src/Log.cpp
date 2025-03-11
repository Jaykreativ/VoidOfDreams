#include "Log.h"

namespace logger {
	std::vector<std::string> _regionStack;
	
	void initLog() {
		_regionStack = {};
	}

	void terminateLog() {}

	void beginRegion(std::string name) {
		_regionStack.push_back(name);
	}

	void endRegion() {
		_regionStack.pop_back();
	}

	void error(std::string msg) {
		for (std::string region : _regionStack)
			printf("%s -> ", region.c_str());
		fprintf(stderr, ":\nERROR: %s\n", msg.c_str());
	}
	void errorRaw(std::string msg) {
		fprintf(stderr, "%s\n", msg.c_str());
	}

	void warning(std::string msg) {
		for (std::string region : _regionStack)
			printf("%s -> ", region.c_str());
		printf(":\nWARNING: %s\n", msg.c_str());
	}
	void warningRaw(std::string msg) {
		printf("%s\n", msg.c_str());
	}

	void info(std::string msg) {
		for (std::string region : _regionStack)
			printf("%s -> ", region.c_str());
		printf(":\n%s\n", msg.c_str());
	}
	void infoRaw(std::string msg) {
		printf("%s\n", msg.c_str());
	}

	std::string getCurrentRegion() {
		return _regionStack.back();
	}

	const std::vector<std::string>& getRegionStack() {
		return _regionStack;
	}
}