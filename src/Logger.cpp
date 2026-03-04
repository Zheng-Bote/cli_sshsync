/**
 * SPDX-FileComment: Implementation of Logger handling
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: MIT
 *
 * @file Logger.cpp
 * @brief Implementation for Logger.
 * @version 1.1.0
 * @date 2026-03-04
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license MIT License
 */
#include "Logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <vector>

void Logger::Init(const std::string& custom_path, spdlog::level::level_enum level) {
    std::string log_filename = custom_path;

    if (log_filename.empty()) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm parts;
#if defined(_WIN32)
        localtime_s(&parts, &now_c);
#else
        localtime_r(&now_c, &parts);
#endif
        std::stringstream ss;
        ss << std::put_time(&parts, "%Y-%m-%d") << ".log";
        log_filename = (std::filesystem::current_path() / ss.str()).string();
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(level);
    console_sink->set_pattern("[%^%l%$] %v"); // Simple pattern for console

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename, true); // true for append
    file_sink->set_level(level);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("ssh_copy", sinks.begin(), sinks.end());
    logger->set_level(level);
    
    // Set as default global logger
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
    spdlog::flush_on(spdlog::level::info);
}

std::shared_ptr<spdlog::logger> Logger::Get() {
    return spdlog::get("ssh_copy");
}
