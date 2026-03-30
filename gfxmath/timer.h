#pragma once
#include <chrono>

 class timer {
	 typedef std::chrono::high_resolution_clock hr_clock;
	 typedef std::chrono::microseconds micro_seconds;
	 typedef std::chrono::milliseconds milli_seconds;

public:
	timer();

	void resetTime();
	float dt(); //how much time elapsed since the previous dt() invocation

	//time elapsed since the last creation or reset (in milliseconds)
	float elapsed(); 


private:
	hr_clock::time_point start_time; //the time this m_clock was created
	hr_clock::time_point last_time; //the most recent time the m_clock was queried.
};

