#ifndef _RATELIMITER_H_
#define _RATELIMITER_H_

//acts as rate limiter for simulating real work
//the first call to WaitFor will require a wait. subsequent calls only require a wait if the neccessary units aren't ready.
//currently assumes no limit on buffering of units (i.e. doing other work for one second will mean that one second worth of units can be had immediately, and 10 minutes of units after 10 minutes).

#include <stdlib.h>
#include <sys/time.h>
//#include "batch_dedup_config.h"

using namespace std;

class RateLimiter {
private:
    double rate;
    timespec mark_time_;
    bool initialized;
public:
    //specify the maximum rate (in units/second)
    //rates <= 0 cause no rate limiting
    RateLimiter(double rate);

    //wait for the specified number of units to be ready
    //no wait is imposed if the units are already accumulated
    //the first call to WaitFor forces a wait (unless units <= 0)
    //can use units = 0 to start buffering without forcing wait
    //always returns requested number of units
    double WaitFor(double units);
};

#endif
