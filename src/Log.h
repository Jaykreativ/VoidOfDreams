#pragma once

#include <string>
#include <vector>

// for time profiles use
// #define LOG_GLOBAL_TIMESTAMPS for a global timeline
// #define LOG_FRAME_TIMESTAMPS for a frame profile
#define LOG_GLOBAL_TIMESTAMPS
#define LOG_FRAME_TIMESTAMPS
namespace logger {
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
