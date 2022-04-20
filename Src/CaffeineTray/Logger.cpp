#include "Logger.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Caffeine {

auto InitLogger (const fs::path& logFilePath) -> bool
{
    auto logger = spdlog::basic_logger_mt("file_logger", logFilePath.string(), true);
    logger->set_pattern("[%Y-%m-%d %T.%e][%8l]{%5t} %v");

    spdlog::flush_on(spdlog::level::info);
    spdlog::set_level(spdlog::level::level_enum::info);

#if defined(_DEBUG) && defined(_WIN32)
    auto vsdbgsink = std::make_shared<spdlog::sinks::windebug_sink_mt>();
    vsdbgsink->set_pattern("[%8l]{%5t} %v");
    logger->sinks().push_back(vsdbgsink);

    spdlog::flush_on(spdlog::level::debug);
    spdlog::set_level(spdlog::level::level_enum::debug);
#endif

    spdlog::set_default_logger(logger);

    return true;
}

} // namespace Caffeine
