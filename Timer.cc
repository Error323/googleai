#include <unistd.h>

#define USE_POSIX_TIMERS (false && (defined _POSIX_TIMERS) && (defined _POSIX_CPUTIME))

#if (USE_POSIX_TIMERS)
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "Timer.h"



struct TimerState {
	#if (USE_POSIX_TIMERS)
		struct timespec t0;
		struct timespec t1;
		struct timespec dt;

		void tick() { clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0); }
		void tock() { clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1); }
		double time() {
			if ((t1.tv_nsec - t0.tv_nsec) < 0) {
				dt.tv_sec = t1.tv_sec - t0.tv_sec - 1;
				dt.tv_nsec = 1000000000L + t1.tv_nsec - t0.tv_nsec;
			} else {
				dt.tv_sec = t1.tv_sec - t0.tv_sec;
				dt.tv_nsec = t1.tv_nsec - t0.tv_nsec;
			}

			const double secs =
				(dt.tv_sec < 1.0)?
				((static_cast<double>(dt.tv_nsec) * 1e-9) +         0):
				((static_cast<double>(dt.tv_nsec) * 1e-9) + dt.tv_sec);

			return secs;
		}
	#else
		struct timeval t0;
		struct timeval t1;
		struct timeval dt;

		void tick() { gettimeofday(&t0, NULL); }
		void tock() { gettimeofday(&t1, NULL); }
		double time() {
			dt.tv_sec = t1.tv_sec - t0.tv_sec;
			dt.tv_usec = t1.tv_usec - t0.tv_usec;

			const double secs = (dt.tv_sec + (static_cast<double>(dt.tv_usec) * 1e-6));
			return secs;
		}
	#endif
};



Timer::Timer() { state = new TimerState(); }
Timer::~Timer() { delete state; }

void Timer::Tick() { state->tick(); }
void Timer::Tock() { state->tock(); }

double Timer::Time() const { return (state->time()); }
