#pragma once

// defines
#define ACTOR_DIR "./Actors/"

// define user-conversions between ImVec and glm::vec
#include "glm.hpp"
#define IM_VEC2_CLASS_EXTRA\
	operator glm::vec2() { return glm::vec2(x, y); }\
	ImVec2(glm::vec2& vec)\
		: x(vec.x), y(vec.y)\
	{}\
	const ImVec2(const glm::vec2& vec)\
		: x(vec.x), y(vec.y)\
	{}

#define IM_VEC4_CLASS_EXTRA\
	operator glm::vec4() { return glm::vec4(x, y, z, w); }\
	ImVec4(glm::vec4& vec)\
		: x(vec.x), y(vec.y), z(vec.z), w(vec.w)\
	{}\
	const ImVec4(const glm::vec4& vec)\
		: x(vec.x), y(vec.y), z(vec.z), w(vec.w)\
	{}

#include "Shares/Controls.h"

void runGame();