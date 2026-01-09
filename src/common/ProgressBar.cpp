#include "ProgressBar.h"
#include "Params.h"
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>
#include <chrono>

namespace termcolor {
    const std::string reset  = "\033[0m";
    const std::string green  = "\033[32m";
    const std::string yellow = "\033[33m";
    const std::string cyan   = "\033[36m";
    const std::string bold   = "\033[1m";
}

void ProgressBar::print_progress_bar(double percentage,
                        std::chrono::steady_clock::time_point start_time,
                        uint32_t framerate,
                        uint32_t samples)
{
    // If logging happened, move progress bar to a new line
    if (ProgressBar::g_log_printed) {
        std::cout << std::endl;
    }
    ProgressBar::g_log_printed = false;

    auto current_time = std::chrono::steady_clock::now();
    double elapsed_seconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - start_time).count() / 1000.0;

    double estimated_total_seconds =
        percentage > 0.0 ? (elapsed_seconds / percentage) : INFINITY;
    double remaining_seconds = estimated_total_seconds - elapsed_seconds;

    auto format_time = [](double sec) {
        if (sec >= 3600)
            return std::to_string(int(sec / 3600)) + "h " +
                   std::to_string(int((int(sec) % 3600) / 60)) + "m";
        if (sec >= 60)
            return std::to_string(int(sec / 60)) + "m " +
                   std::to_string(int(sec) % 60) + "s";
        return std::to_string(int(sec)) + "s";
    };

    std::string elapsed_str   = format_time(elapsed_seconds);
    std::string remaining_str = format_time(remaining_seconds);

    // Terminal width
    struct winsize w{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int cols = w.ws_col;

    const int bar_width = 50;
    int pos = static_cast<int>(bar_width * percentage);

    std::string bar = "[" + termcolor::green + termcolor::bold;
    if (pos == 0) {
        bar += ">" + std::string(bar_width - 1, ' ');
    } else if (pos >= bar_width) {
        bar += std::string(bar_width, '=');
    } else {
        bar += std::string(pos - 1, '=');
        bar += ">";
        bar += std::string(bar_width - pos - 1, ' ');
    }
    bar += termcolor::reset + "]";

    std::string output =
        "\r" + bar +
        " " + std::to_string(int(percentage * 100)) + "% "
        "(Elapsed: " + elapsed_str +
        ", Remaining: " + remaining_str + ") - "
        "FPS: " + std::to_string(framerate) + " - "
        "Samples: " + std::to_string(samples);

    // Clear rest of line
    int leftover = cols - static_cast<int>(output.size());
    if (leftover > 0)
        output += std::string(leftover, ' ');

    std::cout << output << std::flush;

    if (percentage >= 1.0) {
        std::cout << std::endl;
    }
}


void ProgressBar::Update(uint32_t framerate, uint32_t samples) {
    static auto time = std::chrono::steady_clock::now();
    print_progress_bar(float(samples) / float(Params::GetSampleCount()), time, framerate, samples);
}