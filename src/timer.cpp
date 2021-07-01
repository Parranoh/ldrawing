#include "include/timer.hpp"

std::chrono::time_point<std::chrono::steady_clock> timer::start_point[4] = { {}, {}, {}, {} };
double timer::duration[4] = { 0.0, 0.0, 0.0, 0.0 };

void timer::start(activity_t act)
{
    start_point[act] = std::chrono::steady_clock::now();
}

void timer::stop(activity_t act)
{
    auto end_point = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end_point - start_point[act];
    duration[act] += diff.count();
}

void timer::print_times(std::ostream &os)
{
    os
        << duration[IO] << ' '
        << duration[DECOMPOSE] << ' '
        << duration[RECT_DUAL] << ' '
        << duration[PORT_ASSIGNMENT] << std::endl;
}
