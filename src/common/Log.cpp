#include "common/Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>
#include <vector>
#include "ProgressBar.h"

static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> s_Loggers;

void Log::Init() {
    const std::filesystem::path log_file{"Raytracer.log"};

    if (std::filesystem::exists(log_file)) {
        std::filesystem::remove(log_file);
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

static std::string ConvertSourceFilenameToLoggerLabel(std::string name) {
    name = split(name, '/').back();
    name = split(name, '\\').back();
    name = split(name, '.').front();
    return name;
}

void Log::CreateLoggerIfNotExists(const std::string& name) {
    if (s_Loggers.find(name) != s_Loggers.end()) {
        return;
    }

    std::vector<spdlog::sink_ptr> logSinks;
    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Raytracer.log", false));

    logSinks[0]->set_pattern("%^[%T] %n: %v%$");
    logSinks[1]->set_pattern("[%T] [%l] %n: %v");

    std::string logger_label = std::string("[") + ConvertSourceFilenameToLoggerLabel(name) + std::string("]");
    s_Loggers[name] = std::make_shared<spdlog::logger>(logger_label, begin(logSinks), end(logSinks));
    spdlog::register_logger(s_Loggers.at(name));
    s_Loggers.at(name)->set_level(spdlog::level::trace);
    s_Loggers.at(name)->flush_on(spdlog::level::trace);
}

std::shared_ptr<spdlog::logger>& Log::GetLogger(const std::string& _file_) {
    if (!ProgressBar::g_log_printed) {
        std::cout << std::endl;
    }
    ProgressBar::g_log_printed = true;
    Log::CreateLoggerIfNotExists(_file_);
    return s_Loggers.at(_file_);
}