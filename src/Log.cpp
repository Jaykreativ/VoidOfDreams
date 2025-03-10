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

}