#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <memory>

class Logger {
public:
    // Initializes the global spdlog instance with console and file sinks.
    // The log file will be named like <YYYY-MM-DD>.log or as provided in custom_path.
    // If custom_path is empty, it uses the current date in the current directory.
    static void Init(const std::string& custom_path = "", spdlog::level::level_enum level = spdlog::level::info);

    // Get the global logger instance
    static std::shared_ptr<spdlog::logger> Get();
};

#endif // LOGGER_HPP
