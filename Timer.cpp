#include "stdafx.h"
#include "Timer.h"

Timer::Timer()
{
    __int64 counts_per_second;
    QueryPerformanceFrequency((LARGE_INTEGER*)&counts_per_second);
    seconds_per_count_ = 1.0 / (double)counts_per_second;

    __int64 curr_time;
    QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);
    base_time_ = curr_time;
    prev_time_ = curr_time;
}

float Timer::TotalTime() const
{
    return (curr_time_ - base_time_) * seconds_per_count_;
}

float Timer::PlayTime() const
{
    if (is_stop_)
        return (stop_time_ - base_time_ - paused_time_) * seconds_per_count_;
    else
        return (curr_time_ - base_time_ - paused_time_) * seconds_per_count_;
}

float Timer::ElapsedTime() const
{
    return elapsed_time_;
}

int Timer::Fps()
{
    double fps = 1.0 / elapsed_time_;
    fps_buffer_[fps_buffer_index_] = fps;
    fps_buffer_index_ = (fps_buffer_index_ + 1) % kFpsBufferSize;
    double sum = std::accumulate(fps_buffer_.begin(), fps_buffer_.end(), 0.f);
    
    return sum /= kFpsBufferSize;
}

void Timer::Reset()
{
    __int64 curr_time;
    QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);
    base_time_ = curr_time;		
    paused_time_ = 0;	        
    stop_time_ = 0;		        
    prev_time_ = curr_time;		

    is_stop_ = false;
}

void Timer::Start()
{
    if (is_stop_)
    {
        is_stop_ = false;

        __int64 curr_time;
        QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);

        paused_time_ = curr_time - stop_time_;
        prev_time_ = curr_time;
        stop_time_ = 0;
    }
}

void Timer::Stop()
{
    if (!is_stop_)
    {
        is_stop_ = true;
    
        __int64 curr_time;
        QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);
    
        stop_time_ = curr_time;
    }
}

void Timer::Tick()
{
    if (is_stop_)
    {
        elapsed_time_ = 0.f;
        return;
    }

    __int64 curr_time;
    QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);

    curr_time_ = curr_time;

    elapsed_time_ = (curr_time_ - prev_time_) * seconds_per_count_;

    prev_time_ = curr_time_;

    if (elapsed_time_ < 0)
    {
        elapsed_time_ = 0;
    }

}
