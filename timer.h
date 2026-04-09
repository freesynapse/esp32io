#ifndef __TIMER_H
#define __TIMER_H

#include <string.h>
#include <chrono>
#include <stdio.h>

#include "types.h"

// scoped-based timer class
class Timer
{
public:
    Timer(const char* _func="", bool _print_result=false) : // RAII
        m_printResult(_print_result)
    {
        if (m_printResult)
        {
            memset(m_caller, '\0', 128);
            sprintf(m_caller, "%s", _func);
        }

        m_startTimepoint = std::chrono::high_resolution_clock::now();
    }

    ~Timer()
    {
        stop();
    }

    void stop()
    {
        auto endTimepoint = std::chrono::high_resolution_clock::now();
        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        auto duration = end - start;
        double ms = duration * 0.001;

        if (m_printResult)
            LOG_INFO("%s: %.3f ms.\n", m_caller, ms);
            
    }

    /* Elapsed time in microseconds. */
    long long getDeltaTime()
    {
        auto endTimepoint = std::chrono::high_resolution_clock::now();
        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        auto duration = end - start;

        return duration;
    }

    float getDeltaTimeMs() { return getDeltaTime() * 0.001f; }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimepoint;
    char m_caller[128] = "";
    bool m_printResult = false;

};

#endif // __TIMER_H