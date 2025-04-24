#include "Log.h"

#define IM_VEC2_CLASS_EXTRA\
	operator glm::vec2() {return glm::vec2(x, y);}\
	ImVec2(glm::vec2& vec)\
		: x(vec.x), y(vec.y)\
	{}\
	const ImVec2(const glm::vec2& vec)\
		: x(vec.x), y(vec.y)\
	{}

#include "glm.hpp"
#include "imgui.h"

#include <chrono>
#include <string>
#include <unordered_map>

namespace logger {
	typedef std::chrono::nanoseconds RegionDuration;
	typedef std::chrono::duration<float> RegionDurationSFloat;
	typedef std::chrono::time_point<std::chrono::steady_clock> RegionTimePoint;

	struct RegionTime {
		RegionTimePoint beginTime = RegionTimePoint(RegionDuration(-1));
		RegionTimePoint endTime = RegionTimePoint(RegionDuration(-1));
	};

	struct RegionTimeProfile {
		RegionTimeProfile* parentProfile = nullptr;
		std::vector<RegionTimeProfile> childProfiles = {};

		std::string region = "";
		RegionTime time = {};
	};

	Settings _settings;

	std::vector<std::string> _regionStack;

	// time storage
	RegionTimePoint _programStartTime = std::chrono::high_resolution_clock::now();
	RegionTimeProfile _rootTimeProfile = {};
	RegionTimeProfile* _pCurrentTimeProfile = &_rootTimeProfile;
	RegionDuration _timelineValidDuration = std::chrono::seconds(10); // default to ten seconds to view

	// frameview
	bool _isInFrame = false;
	struct RegionFrameTime {
		RegionFrameTime* parent = nullptr;
		std::unordered_map<std::string, RegionFrameTime> children = {};

		std::string region = "";
		float duration = 0;
		size_t samples = 0;

		RegionTime time = {};
	};
	RegionFrameTime _rootFrameTime = {};
	RegionFrameTime* _pCurrentFrameTime = &_rootFrameTime;

	void initLog(Settings settings) {
		_settings = settings;

		if (_settings.enableGlobalTimestamps) {
			_rootTimeProfile.region = "root";
			_rootTimeProfile.time.beginTime = std::chrono::high_resolution_clock::now();
		}

		_regionStack = {};
	}

	void terminateLog() {}

	void frameTimeBegin(std::string region);

	void frameTimeEnd(std::string region);

	void beginRegion(std::string name) {
		if (_settings.enableGlobalTimestamps) {
			_pCurrentTimeProfile->childProfiles.push_back({_pCurrentTimeProfile}); // make a new time profile in the tree
			_pCurrentTimeProfile = &_pCurrentTimeProfile->childProfiles.back();

			_pCurrentTimeProfile->region = name;
			_pCurrentTimeProfile->time.beginTime = std::chrono::high_resolution_clock::now(); // save the begin time
		}

		if (_isInFrame && _settings.enableFrameTimestamps) {
			frameTimeBegin(name);
		}

		_regionStack.push_back(name);
	}

	void endRegion() {
		if (_settings.enableGlobalTimestamps) {
			_pCurrentTimeProfile->time.endTime = std::chrono::high_resolution_clock::now(); // save the end time
			_pCurrentTimeProfile = _pCurrentTimeProfile->parentProfile; // go back to parent
		}

		if (_isInFrame && _settings.enableFrameTimestamps) {
			frameTimeEnd(_regionStack.back());
		}

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
		_timelineValidDuration = std::chrono::seconds((int64_t)duration);
		printf("%lld\n", _timelineValidDuration.count());
	}

	void cleanProfile(RegionTimeProfile& profile, uint32_t& index) {
		if (profile.time.endTime > RegionTimePoint(RegionDuration(0)) &&
			std::chrono::high_resolution_clock::now() - profile.time.endTime > _timelineValidDuration
		) {
			profile.parentProfile->childProfiles.erase(profile.parentProfile->childProfiles.begin() + index); // TODO fix parent ptr getting invalid
			index--;
			return;
		}
		for (uint32_t i = 0; i < profile.childProfiles.size(); i++) {
			cleanProfile(profile.childProfiles[i], i);
		}
	}

	void cleanTimeline() {
		for (uint32_t i = 0; i < _rootTimeProfile.childProfiles.size(); i++) {
			cleanProfile(_rootTimeProfile.childProfiles[i], i);
		}
	}

	void frameTimeBegin(std::string region) {
		auto* newFrameTime = &_pCurrentFrameTime->children[region];
		newFrameTime->parent = _pCurrentFrameTime;
		newFrameTime->region = region;
		newFrameTime->time.beginTime = std::chrono::high_resolution_clock::now();
		_pCurrentFrameTime = newFrameTime;
	}

	void frameTimeEnd(std::string region) {
		_pCurrentFrameTime->time.endTime = std::chrono::high_resolution_clock::now();
		float duration = std::chrono::duration_cast<RegionDurationSFloat>(_pCurrentFrameTime->time.endTime - _pCurrentFrameTime->time.beginTime).count();
		_pCurrentFrameTime->duration = (_pCurrentFrameTime->duration * _pCurrentFrameTime->samples + duration) / (_pCurrentFrameTime->samples + 1);
		_pCurrentFrameTime->samples++;
		_pCurrentFrameTime = _pCurrentFrameTime->parent;
	}

	void beginFrame() {
		_isInFrame = true;
		_rootFrameTime.parent = nullptr;
		_rootFrameTime.region = "root";
		_rootFrameTime.time.beginTime = std::chrono::high_resolution_clock::now();
		_pCurrentFrameTime = &_rootFrameTime;
	}

	void endFrame() {
		_isInFrame = false;
		_rootFrameTime.time.endTime = std::chrono::high_resolution_clock::now();
		float duration = std::chrono::duration_cast<RegionDurationSFloat>(_pCurrentFrameTime->time.endTime - _pCurrentFrameTime->time.beginTime).count();
		_rootFrameTime.duration = (_rootFrameTime.duration * _rootFrameTime.samples + duration) / (_rootFrameTime.samples + 1);
		_rootFrameTime.samples++;
	}

	struct TimelineGuiData {
		float width = 0;
		RegionTimePoint now;
		RegionDuration viewDuration = std::chrono::round<RegionDuration>(RegionDurationSFloat(1));
		glm::vec2 zero = {0, 0};
	} _timelineGuiData;

	void drawRegionProfile(const RegionTimeProfile& profile, uint32_t depth) {
		auto* drawList = ImGui::GetWindowDrawList();

		float beginFromNow = std::chrono::duration_cast<std::chrono::duration<float>>(_timelineGuiData.now - profile.time.beginTime).count();
		float endFromNow;
		if (profile.time.endTime < RegionTimePoint(RegionDuration(0)))
			endFromNow = 0;
		else
			endFromNow = std::chrono::duration_cast<std::chrono::duration<float>>(_timelineGuiData.now - profile.time.endTime).count();
		float guiDuration = std::chrono::duration_cast<std::chrono::duration<float>>(_timelineGuiData.viewDuration).count();

		std::hash<std::string> strHasher	;
		auto seed = strHasher(profile.region); // generate random color from region string, same string results in same color
		float randColor[3];
		srand(seed); seed = rand();
		randColor[0] = (float)(seed % 256) / 256;
		srand(seed); seed = rand();
		randColor[1] = (float)(seed % 256) / 256;
		srand(seed); seed = rand();
		randColor[2] = (float)(seed % 256) / 256;

		drawList->AddRectFilled(
			_timelineGuiData.zero + glm::vec2(_timelineGuiData.width * std::max<float>(1 - beginFromNow / guiDuration, 0), 25*(depth)),
			_timelineGuiData.zero + glm::vec2(_timelineGuiData.width * std::max<float>(1 - endFromNow / guiDuration, 0), 25*(depth+1)),
			ImGui::GetColorU32({ randColor[0], randColor[1], randColor[2], .25}));

		if(ImGui::TreeNode(profile.region.c_str())){
			ImGui::Text(("begin was(ms): " + std::to_string(beginFromNow)).c_str());
			ImGui::Text(("end was(ms): " + std::to_string(endFromNow)).c_str());
			for (const auto& childProfile : profile.childProfiles)
				drawRegionProfile(childProfile, depth+1);

			ImGui::TreePop();
		}
	}

	void drawTimelineImGui() {
		if (!_settings.enableGlobalTimestamps) {
			ImGui::Text("Global Timestamps are disabled");
			return;
		}

		_timelineGuiData.width = ImGui::GetContentRegionAvail().x;
		_timelineGuiData.now = std::chrono::high_resolution_clock::now();
		_timelineGuiData.zero = (glm::vec2)ImGui::GetWindowContentRegionMin() + (glm::vec2)ImGui::GetWindowPos();

		// input for view duration
		float duration = std::chrono::duration_cast<RegionDurationSFloat>(_timelineGuiData.viewDuration).count();
		float maxDuration = std::chrono::duration_cast<RegionDurationSFloat>(_timelineValidDuration).count();
		ImGui::SliderFloat("view duration", &duration, 0.001, maxDuration);
		_timelineGuiData.viewDuration = std::chrono::round<RegionDuration>(RegionDurationSFloat(duration));

		drawRegionProfile(_rootTimeProfile, 0);
	}

	struct FrameProfileGuiData {
		glm::vec2 contentMin = {};
		glm::vec2 contentMax = {};
	} _frameProfileGuiData;

	void drawFrameRegion(RegionFrameTime& frameTime, uint32_t depth) {
		auto* drawList = ImGui::GetWindowDrawList();

		float xPos = _frameProfileGuiData.contentMin.x;
		float xOffset = ImGui::GetCursorPosX();
		float xSize = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		float yPos = _frameProfileGuiData.contentMin.y;
		float yOffset = ImGui::GetCursorPosY();
		float yOffsetText = ImGui::GetCursorPosY();
		float ySize = ImGui::GetTextLineHeightWithSpacing()*frameTime.children.size();

		for (auto& childPair : frameTime.children) {
			std::hash<std::string> strHasher;
			auto seed = strHasher(childPair.second.region); // generate random color from region string, same string results in same color
			float randColor[3];
			srand(seed); seed = rand();
			randColor[0] = (float)(seed % 256) / 256;
			srand(seed); seed = rand();
			randColor[1] = (float)(seed % 256) / 256;
			srand(seed); seed = rand();
			randColor[2] = (float)(seed % 256) / 256;
			ImU32 color = ImGui::GetColorU32({ randColor[0], randColor[1], randColor[2], .5 });

			float xDurationSize = xSize * childPair.second.duration / frameTime.duration;
			drawList->AddRectFilled(
				{ xPos + xOffset, yPos + yOffset },
				{ xPos + xOffset + xDurationSize, yPos + yOffset + ySize},
				color);
			xOffset += xDurationSize;
		}
		xOffset = ImGui::GetCursorPosX();
		yOffset = ImGui::GetCursorPosY();
		yOffsetText = ImGui::GetCursorPosY();
		for (auto& childPair : frameTime.children) {
			std::hash<std::string> strHasher;
			auto seed = strHasher(childPair.second.region); // generate random color from region string, same string results in same color
			float randColor[3];
			srand(seed); seed = rand();
			randColor[0] = (float)(seed % 256) / 256;
			srand(seed); seed = rand();
			randColor[1] = (float)(seed % 256) / 256;
			srand(seed); seed = rand();
			randColor[2] = (float)(seed % 256) / 256;
			ImU32 color = ImGui::GetColorU32({ randColor[0], randColor[1], randColor[2], .5 });

			float xDurationSize = xSize * childPair.second.duration / frameTime.duration;
			std::string text = childPair.second.region + "(" + std::to_string(childPair.second.duration*1000) + "ms)";
			drawList->AddText({ xPos + xOffset, yPos + yOffsetText }, ImGui::GetColorU32({1, 1, 1, 1}), text.data(), text.data() + text.size());
			xOffset += xDurationSize;
			yOffsetText += ImGui::GetTextLineHeightWithSpacing();
		}
		ImGui::SetCursorPosY(yOffset + ySize + 10);

		for (auto& childPair : frameTime.children) {
			drawFrameRegion(childPair.second, depth + 1);
		}

	}

	void clearSamples(RegionFrameTime& frameTime) {
		frameTime.samples = 0;
		for (auto& childPair : frameTime.children)
			clearSamples(childPair.second);
	};

	void drawFrameProfileImGui() {
		if (!_settings.enableFrameTimestamps) {
			ImGui::Text("Frame Timestamps are disabled");
			return;
		}

		if (ImGui::Button("reset")) {
			clearSamples(_rootFrameTime);
		}


		_frameProfileGuiData.contentMin = (glm::vec2)ImGui::GetWindowPos();
		_frameProfileGuiData.contentMax = _frameProfileGuiData.contentMin + (glm::vec2)ImGui::GetContentRegionAvail();

		drawFrameRegion(_rootFrameTime, 0);
	}
}