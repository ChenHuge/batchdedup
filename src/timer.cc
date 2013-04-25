#include "timer.h"

Timer::Timer()
{
    duration_ = 0.0;
    is_running_ = false;
}

void Timer::Start()
{
    if (is_running_)
        return;
    is_running_ = true;
    gettimeofday(&start_time_, NULL);
}

double Timer::Stop()
{
    if (!is_running_)
        return 0.0;

    is_running_ = false;
    timeval endTime;
    long seconds, useconds;
    gettimeofday(&endTime, NULL);
    seconds  = endTime.tv_sec  - start_time_.tv_sec;
    useconds = endTime.tv_usec - start_time_.tv_usec;
    double last_duration = (seconds + useconds/1000000.0) * 1000;
    duration_ += last_duration;
    return last_duration;
}

double Timer::Reset()
{
    Stop();
    double old_duration = duration_;
    duration_ = 0.0;
    return old_duration;
}

double Timer::GetDuration()
{
    return duration_;
}

map<string, Timer> TimerPool::timer_map_;

void TimerPool::Start(const string& timer_name)
{
    TimerPool::timer_map_[timer_name].Start();
}

double TimerPool::Stop(string const &timer_name)
{
    return TimerPool::timer_map_[timer_name].Stop();
}

double TimerPool::Reset(string const &timer_name)
{
    return TimerPool::timer_map_[timer_name].Reset();
}

void TimerPool::Print(const string& timer_name)
{
    double last_duration = TimerPool::timer_map_[timer_name].Stop();
    double total_duration = TimerPool::timer_map_[timer_name].GetDuration();
    LOG_INFO("Duration of [" << timer_name << "] in ms: " <<
                 "last " << last_duration << " , total " << total_duration);
}

void TimerPool::PrintAll()
{
    map<string, Timer>::iterator it;
    double duration;
    for (it = timer_map_.begin(); it != timer_map_.end(); it++) {
        it->second.Stop();
        duration = it->second.GetDuration();
        LOG_INFO("Total duration of ["<< it->first <<
                     "] in ms: " << duration);
    }
}
