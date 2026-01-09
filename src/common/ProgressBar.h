#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <chrono>

struct ProgressBar {
    static void Update(uint32_t framerate, uint32_t samples);
    inline static bool g_log_printed = false;

private:
    static void print_progress_bar(double percentage, std::chrono::steady_clock::time_point start_time, uint32_t framerate, uint32_t samples);
};

#endif