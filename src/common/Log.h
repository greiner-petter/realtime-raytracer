#pragma once
#include <memory>
#include <filesystem>
// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#pragma warning(pop)

class Log {
public:
    static void Init();

    static std::shared_ptr<spdlog::logger>& GetLogger();
};

// Main Logging Macro
#define RT_LOG(_LogLevel, ...) ::Log::GetLogger()->log((::spdlog::level::level_enum)_LogLevel, __VA_ARGS__)

#define RT_TRACE(...)    RT_LOG(::spdlog::level::trace, __VA_ARGS__)
#define RT_INFO(...)     RT_LOG(::spdlog::level::info, __VA_ARGS__)
#define RT_WARN(...)     RT_LOG(::spdlog::level::warn, __VA_ARGS__)
#define RT_ERROR(...)    RT_LOG(::spdlog::level::err, __VA_ARGS__)
#define RT_CRITICAL(...) RT_LOG(::spdlog::level::critical, __VA_ARGS__)

#define RT_INTERNAL_ASSERT_IMPL(check, msg, ...) { if(!(check)) { RT_ERROR(::QS::LogCategory::Assert, msg, __VA_ARGS__); RT_DEBUGBREAK(); } }
#define RT_INTERNAL_ASSERT_WITH_MSG(check, ...) RT_INTERNAL_ASSERT_IMPL(check, "Assertion failed: {0}", __VA_ARGS__)
#define RT_INTERNAL_ASSERT_NO_MSG(check) RT_INTERNAL_ASSERT_IMPL(check, "Assertion '{0}' failed at {1}:{2}", RT_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)
#define RT_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define RT_INTERNAL_ASSERT_GET_MACRO(...) RT_EXPAND_MACRO( RT_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, RT_INTERNAL_ASSERT_WITH_MSG, RT_INTERNAL_ASSERT_NO_MSG) )

//#define RT_ASSERT(...) RT_EXPAND_MACRO( RT_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#define RT_ASSERT(...) /* nothing */