#pragma once

#include <string>
#include <vector>

namespace logger {

	struct Settings {
		bool enableGlobalTimestamps = false;
		bool enableFrameTimestamps = false;
	};

	void initLog(Settings settings = Settings{});

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

	// duration in seconds
	void setTimelineValidDuration(float duration);

	// delete old profiles
	// this has to be called from time to time to not run out of memory
	void cleanTimeline();

	void beginFrame();

	void endFrame();

	void drawTimelineImGui();

	void drawFrameProfileImGui();
}
