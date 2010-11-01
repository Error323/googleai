#ifndef _TIMER_HDR_
#define _TIMER_HDR_

struct TimerState;
class Timer {
public:
	Timer();
	~Timer();

	void Tick();
	void Tock();

	// return time elapsed between
	// Tick() and last Tock() call
	// in seconds
	double Time() const;

private:
	TimerState* state;
};

#endif
