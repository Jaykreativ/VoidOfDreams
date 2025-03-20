#pragma once

#include <string>
#include <vector>

namespace logger {
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

	const std::vector<std::string>& getRegionStack();

	// delete old profiles
	// this has to be called from time to time to not run out of memory
	// duration in seconds
	void setTimelineDuration(float duration);

	void cleanTimeline();

	void drawTimelineImGui();
}
