#pragma once

#include <chrono>
#include <iostream>

class timer {
    static std::chrono::time_point<std::chrono::steady_clock> start_point[];
    static double duration[];
public:
    enum activity_t : unsigned char { IO = 0, DECOMPOSE = 1, RECT_DUAL = 2, PORT_ASSIGNMENT = 3 };
    static void init(void);
    static void start(activity_t);
    static void stop(activity_t);
    static void print_times(std::ostream &);
};
