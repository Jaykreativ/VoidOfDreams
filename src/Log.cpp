#include "Log.h"

#include "imgui.h"

#include <chrono>
#include <thread>

namespace logger {
	typedef std::chrono::duration<float> RegionDurationS;
	typedef std::chrono::duration<float, std::milli> RegionDurationMS;
	typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float>> RegionTimePointS;
	typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<float, std::milli>> RegionTimePointMS;

	struct RegionTime {
		RegionTimePointMS beginTime = RegionTimePointMS(RegionDurationMS(-1));
		RegionTimePointMS endTime = RegionTimePointMS(RegionDurationMS(-1));
	};

	struct RegionTimeProfile {
		RegionTimeProfile* parentProfile = nullptr;
		std::vector<RegionTimeProfile> childProfiles = {};

		std::string region = "";
		RegionTime time = {};
	};

	std::vector<std::string> _regionStack;

	// time storage
	RegionTimePointMS _programStartTime = std::chrono::high_resolution_clock::now();
	RegionTimeProfile _rootTimeProfile = {};
	RegionTimeProfile* _pCurrentTimeProfile = &_rootTimeProfile;

	// time gui
	RegionDurationS _timelineValidDuration = RegionDurationS(10); // default to ten seconds to view

	void initLog() {
		_rootTimeProfile.region = "root";
		_rootTimeProfile.time.beginTime = std::chrono::high_resolution_clock::now();

		_regionStack = {};
	}

	void terminateLog() {}

	void beginRegion(std::string name) {
		_pCurrentTimeProfile->childProfiles.push_back({_pCurrentTimeProfile}); // make a new time profile in the tree
		_pCurrentTimeProfile = &_pCurrentTimeProfile->childProfiles.back();

		_pCurrentTimeProfile->region = name;
		_pCurrentTimeProfile->time.beginTime = std::chrono::high_resolution_clock::now(); // save the begin time
		_regionStack.push_back(name);
	}

	void endRegion() {
		_pCurrentTimeProfile->time.endTime = std::chrono::high_resolution_clock::now(); // save the end time
		_pCurrentTimeProfile = _pCurrentTimeProfile->parentProfile; // go back to parent

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

	void setTimelineValidDuration(float duration) {
		_timelineValidDuration = RegionDurationS(duration);
	}

	void cleanCache() {
		printf("cleaned cache!\n");
	}

	void drawRegionProfile(const RegionTimeProfile& profile) {
		if(ImGui::TreeNode(profile.region.c_str())){

			for (const auto& childProfile : profile.childProfiles)
				drawRegionProfile(childProfile);

			ImGui::TreePop();
		}
	}

	void drawTimelineImGui() {
		ImGui::Text("schoeoeni timeline");

		drawRegionProfile(_rootTimeProfile);
	}
}