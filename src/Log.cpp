#include "Log.h"

#include <thread>
#include <sstream>
#include <array>

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

	struct RegionFrameTime {
		RegionFrameTime* parent = nullptr;
		std::unordered_map<std::string, RegionFrameTime> children = {};

		std::string region = "";
		static const size_t sampleCount = 60;
		std::array<float, sampleCount> durations;

		RegionTime time = {};

		void shiftSamples(float duration) {
			for (size_t i = 0; i < sampleCount-1; i++) {
				durations[i] = durations[i + 1];
			}
			durations[sampleCount - 1] = duration;
		}

		float average() {
			float sum = 0;
			for (auto duration : durations)
				sum += duration;
			return sum / durations.size();
		}
	};

	struct ThreadStorage {
		std::vector<std::string> regionStack;

		// time storage
		RegionTimeProfile rootTimeProfile = {};
		RegionTimeProfile* pCurrentTimeProfile = &rootTimeProfile;

		// frameview
		bool isInFrame = false;
		RegionFrameTime rootFrameTime = {};
		RegionFrameTime* pCurrentFrameTime = &rootFrameTime;
	};

	RegionTimePoint _programStartTime = std::chrono::high_resolution_clock::now();
	RegionDuration _timelineValidDuration = std::chrono::seconds(10); // default to ten seconds to view

	std::unordered_map<std::thread::id, ThreadStorage> _threads = {};

	void frameTimeBegin(std::string region);

	void frameTimeEnd(std::string region);

	void beginRegion(std::string name) {
		auto& data = _threads[std::this_thread::get_id()];

#ifdef LOG_GLOBAL_TIMESTAMPS
		data.pCurrentTimeProfile->childProfiles.push_back({ data.pCurrentTimeProfile}); // make a new time profile in the tree
		data.pCurrentTimeProfile = &data.pCurrentTimeProfile->childProfiles.back();

		data.pCurrentTimeProfile->region = name;
		data.pCurrentTimeProfile->time.beginTime = std::chrono::high_resolution_clock::now(); // save the begin time
#endif // LOG_GLOBAL_TIMESTAMPS

		frameTimeBegin(name);

		data.regionStack.push_back(name);
	}

	void endRegion() {
		auto& data = _threads[std::this_thread::get_id()];

		#ifdef LOG_GLOBAL_TIMESTAMPS
		data.pCurrentTimeProfile->time.endTime = std::chrono::high_resolution_clock::now(); // save the end time
		data.pCurrentTimeProfile = data.pCurrentTimeProfile->parentProfile; // go back to parent
		#endif // LOG_GLOBAL_TIMESTAMPS

		frameTimeEnd(data.regionStack.back());

		data.regionStack.pop_back();
	}

	void error(std::string msg) {
		auto& data = _threads[std::this_thread::get_id()];

		for (std::string region : data.regionStack)
			printf("%s -> ", region.c_str());
		fprintf(stderr, ":\nERROR: %s\n", msg.c_str());
	}
	void errorRaw(std::string msg) {
		fprintf(stderr, "%s\n", msg.c_str());
	}

	void warning(std::string msg) {
		auto& data = _threads[std::this_thread::get_id()];

		for (std::string region : data.regionStack)
			printf("%s -> ", region.c_str());
		printf(":\nWARNING: %s\n", msg.c_str());
	}
	void warningRaw(std::string msg) {
		printf("%s\n", msg.c_str());
	}

	void info(std::string msg) {
		auto& data = _threads[std::this_thread::get_id()];

		for (std::string region : data.regionStack)
			printf("%s -> ", region.c_str());
		printf(":\n%s\n", msg.c_str());
	}
	void infoRaw(std::string msg) {
		printf("%s\n", msg.c_str());
	}

	std::string getCurrentRegion() {
		auto& data = _threads[std::this_thread::get_id()];

		return data.regionStack.back();
	}

	const std::vector<std::string>& getRegionStack() {
		auto& data = _threads[std::this_thread::get_id()];

		return data.regionStack;
	}

	void setTimelineValidDuration(float duration) {
		_timelineValidDuration = std::chrono::seconds((int64_t)duration);
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
		auto& data = _threads[std::this_thread::get_id()];

		for (uint32_t i = 0; i < data.rootTimeProfile.childProfiles.size(); i++) {
			cleanProfile(data.rootTimeProfile.childProfiles[i], i);
		}
	}

	void frameTimeBegin(std::string region) {
#ifdef LOG_FRAME_TIMESTAMPS
		auto& data = _threads[std::this_thread::get_id()];

		auto* newFrameTime = &data.pCurrentFrameTime->children[region];
		newFrameTime->parent = data.pCurrentFrameTime;
		newFrameTime->region = region;
		newFrameTime->time.beginTime = std::chrono::high_resolution_clock::now();
		data.pCurrentFrameTime = newFrameTime;
#endif // LOG_FRAME_TIMESTAMPS
	}

	void frameTimeEnd(std::string region) {
#ifdef LOG_FRAME_TIMESTAMPS
		auto& data = _threads[std::this_thread::get_id()];
		
		data.pCurrentFrameTime->time.endTime = std::chrono::high_resolution_clock::now();
		float duration = std::chrono::duration_cast<RegionDurationSFloat>(data.pCurrentFrameTime->time.endTime - data.pCurrentFrameTime->time.beginTime).count();
		data.pCurrentFrameTime->shiftSamples(duration);
		data.pCurrentFrameTime = data.pCurrentFrameTime->parent;
#endif // LOG_FRAME_TIMESTAMPS
	}

	void beginFrame() {
		auto& data = _threads[std::this_thread::get_id()];

		data.isInFrame = true;
		data.rootFrameTime.parent = nullptr;
		data.rootFrameTime.region = "root";
		data.rootFrameTime.time.beginTime = std::chrono::high_resolution_clock::now();
		data.pCurrentFrameTime = &data.rootFrameTime;
	}

	void endFrame() {
		auto& data = _threads[std::this_thread::get_id()];

		data.isInFrame = false;
		data.rootFrameTime.time.endTime = std::chrono::high_resolution_clock::now();
		float duration = std::chrono::duration_cast<RegionDurationSFloat>(data.pCurrentFrameTime->time.endTime - data.pCurrentFrameTime->time.beginTime).count();
		data.pCurrentFrameTime->shiftSamples(duration);
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
#ifndef LOG_GLOBAL_TIMESTAMPS
		ImGui::Text("Global Timestamps are disabled");
#else
		auto& data = _threads[std::this_thread::get_id()];


		_timelineGuiData.width = ImGui::GetContentRegionAvail().x;
		_timelineGuiData.now = std::chrono::high_resolution_clock::now();
		_timelineGuiData.zero = (glm::vec2)ImGui::GetWindowContentRegionMin() + (glm::vec2)ImGui::GetWindowPos();

		// input for view duration
		float duration = std::chrono::duration_cast<RegionDurationSFloat>(_timelineGuiData.viewDuration).count();
		float maxDuration = std::chrono::duration_cast<RegionDurationSFloat>(_timelineValidDuration).count();
		ImGui::SliderFloat("view duration", &duration, 0.001, maxDuration);
		_timelineGuiData.viewDuration = std::chrono::round<RegionDuration>(RegionDurationSFloat(duration));

		drawRegionProfile(data.rootTimeProfile, 0);
#endif
	}

	struct FrameProfileGuiData {
		glm::vec2 contentMin = {};
		glm::vec2 contentMax = {};
	} _frameProfileGuiData;

	void drawFrameRegion(RegionFrameTime& frameTime, uint32_t depth) {
		auto* drawList = ImGui::GetWindowDrawList();

		float xPos = _frameProfileGuiData.contentMin.x;
		float xOffset = ImGui::GetCursorPosX();
		float xSize = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - 100;
		float yPos = _frameProfileGuiData.contentMin.y - ImGui::GetScrollY();
		float yOffset = ImGui::GetCursorPosY();
		float yOffsetText = ImGui::GetCursorPosY();
		float ySize = ImGui::GetTextLineHeightWithSpacing()*frameTime.children.size();

		float frameTimeAvg = frameTime.average();

		for (auto& childPair : frameTime.children) { // draw color boxes
			float childAvg = childPair.second.average();

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

			float xDurationSize = xSize * childAvg / frameTimeAvg;
			drawList->AddRectFilled(
				{ xPos + xOffset, yPos + yOffset },
				{ xPos + xOffset + xDurationSize, yPos + yOffset + ySize},
				color);
			xOffset += xDurationSize;
		}
		xOffset = ImGui::GetCursorPosX();
		yOffset = ImGui::GetCursorPosY();
		yOffsetText = ImGui::GetCursorPosY();
		for (auto& childPair : frameTime.children) { // draw labels
			float childAvg = childPair.second.average();

			float xDurationSize = xSize * childAvg / frameTimeAvg;
			std::string text = childPair.second.region + "(" + std::to_string(childAvg*1000) + "ms)";
			drawList->AddText({ xPos + xOffset, yPos + yOffsetText }, ImGui::GetColorU32({1, 1, 1, 1}), text.data(), text.data() + text.size());
			ImGui::GetFontSize();
			xOffset += xDurationSize;
			yOffsetText += ImGui::GetTextLineHeightWithSpacing();
		}
		ImGui::SetCursorPosY(yOffset + ySize + 10);

		for (auto& childPair : frameTime.children) {
			drawFrameRegion(childPair.second, depth + 1);
		}

	}

	void drawFrameProfileImGui() {
#ifndef LOG_FRAME_TIMESTAMPS
		ImGui::Text("Frame Timestamps are disabled");
#else
		auto& data = _threads[std::this_thread::get_id()];
		for (auto& dataPair : _threads) {
			auto& data = dataPair.second;
			std::stringstream ss;
			ss << std::hex << std::this_thread::get_id();
			ImGui::BeginChild(("FrameProfile##Thread" + ss.str()).c_str());
			ImGui::Text(("Thread (" + ss.str() + ")").c_str());

			_frameProfileGuiData.contentMin = (glm::vec2)ImGui::GetWindowPos();
			_frameProfileGuiData.contentMax = _frameProfileGuiData.contentMin + (glm::vec2)ImGui::GetContentRegionAvail();

			drawFrameRegion(data.rootFrameTime, 0);
			ImGui::EndChild();
		}
#endif
	}
}