// ============================================================
//  core/timer.cpp
// ============================================================

#include "timer.h"

#include <chrono>

namespace {

using Clock     = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

TimePoint g_start_time   = Clock::now();
TimePoint g_prev_time    = g_start_time;
float     g_delta_time   = 0.0f;

} // anonymous namespace

namespace Timer {

double get_time()
{
    auto now     = Clock::now();
    auto elapsed = std::chrono::duration<double>(now - g_start_time);
    return elapsed.count();
}

float delta_time()
{
    return g_delta_time;
}

void tick()
{
    auto now        = Clock::now();
    auto delta      = std::chrono::duration<float>(now - g_prev_time);
    g_delta_time    = delta.count();
    g_prev_time     = now;
}

} // namespace Timer
