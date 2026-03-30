#include "timer.h"


timer::timer(){
	start_time = hr_clock::now();
}


void timer::resetTime(){
	start_time = hr_clock::now();
}	


float timer::elapsed() {
	hr_clock::time_point curr_time = hr_clock::now();

	micro_seconds elapsed = std::chrono::duration_cast<micro_seconds>(curr_time -
																	   start_time);

	return  (float)(static_cast<double>(elapsed.count()) *0.001);
}



float timer::dt() {
	hr_clock::time_point curr_time = hr_clock::now();

	micro_seconds elapsed = std::chrono::duration_cast<micro_seconds>(curr_time -
																	   last_time);
	last_time = curr_time;

	//return seconds:
	return   (float)(   static_cast<double>(elapsed.count()) *0.000001   );
}