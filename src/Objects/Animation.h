#pragma once

class Animation {
public:
	// starts the animation
	void play();

	// stops the animation
	// when played again the animation will continue from where it was last stopped
	void stop();

	// sets the internal timer to 0
	void reset();

	// needs to be called somewhere in the main loop
	virtual void update(float dt) = 0;

protected:
	bool isPlaying();

	// gives a value from 0 to 1 where 0 is the start of the animation and 1 is the end
	float timeFactor();

	void addDeltaTime(float dt);

private:
	bool m_active = false;
	float m_timer = 0;
	float m_duration = 0;
};