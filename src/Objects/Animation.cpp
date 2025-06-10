#include "Animation.h"

#include <algorithm>

Animation::Animation(float duration)
	: m_duration(duration)
{}

void Animation::play() {
	m_active = true;
}

void Animation::stop() {
	m_active = false;
}

void Animation::reset() {
	m_timer = 0;
}

bool Animation::isPlaying() {
	return m_active;
}

float Animation::timeFactor() {
	return std::clamp<float>(m_timer / m_duration, 0, 1);
}

void Animation::addDeltaTime(float dt) {
	m_timer += dt;
}