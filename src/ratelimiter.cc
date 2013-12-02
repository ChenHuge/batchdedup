#include <time.h>
#include "ratelimiter.h"

RateLimiter::RateLimiter(double rate)
{
    initialized = false;
    this->rate = rate;
}

//wait for the specified number of units to be ready
//forced wait on first call, after that units are buffered over time
//can start buffering without forcing wait by using units=0
//always returns units
double RateLimiter::WaitFor(double units) {
    if (rate <= 0) {
        return units;
    } else if (!initialized) {
        initialized = true;
        clock_gettime(CLOCK_MONOTONIC, &mark_time_);
    }

    if (units <= 0) {
        return units;
    } else {
        //first compute number of nanoseconds to wait for
        long wait_time = (long)((units / rate) * 1000000000.0);
        mark_time_.tv_nsec += wait_time;
        //carry overflow in nanoseconds to seconds
        mark_time_.tv_sec += (mark_time_.tv_nsec / 1000000000);
        mark_time_.tv_nsec = mark_time_.tv_nsec % 1000000000;
        //now wait until the next mark time, doesn't block if it's already passed
        //we use the monotonic clock to avoid issues with setting the time, etc, since we only care about duration
        clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &mark_time_, NULL);
        return units;
    }
}
