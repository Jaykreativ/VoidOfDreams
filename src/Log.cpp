#include "Log.h"

#include <string>
#include <vector>
#include <stack>

namespace log {
	std::stack<std::string, std::vector<std::string>> _regionStack;
	
	void initLog() {
		_regionStack = {};
	}

	void terminateLog() {}

	void beginRegion(std::string name) {
		_regionStack.push(name);
	}

	void endRegion() {
		_regionStack.pop();
	}

	void error(std::string msg) {
		fprintf(stderr, "%s", msg.c_str());
	}
	void errorRaw(std::string msg) {
		fprintf(stderr, "%s", msg.c_str());
	}

	void warning(std::string msg) {
		printf("%s", msg.c_str());
	}
	void warningRaw(std::string msg) {
		printf("%s", msg.c_str());
	}

	void info(std::string msg) {
		printf("%s", msg.c_str());
	}
	void infoRaw(std::string msg) {
		printf("%s", msg.c_str());
	}

	std::string getCurrentRegion() {

	}

	const std::stack<std::string, std::vector<std::string>>& getRegionStack() {

	}
}