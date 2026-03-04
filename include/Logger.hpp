/**
 * SPDX-FileComment: Header for Logger handling
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: MIT
 *
 * @file Logger.hpp
 * @brief Header for globally accessible spdlog Logger.
 * @version 1.1.0
 * @date 2026-03-04
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license MIT License
 */
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <memory>

/**
 * @class Logger
 * @brief Provides a static initialization and retrieval interface for a global spdlog instance.
 */
class Logger {
public:
    /**
     * @brief Initializes the global spdlog instance with console and file sinks.
     * The log file will be named like <YYYY-MM-DD>.log or as provided in custom_path.
     * If custom_path is empty, it uses the current date in the current directory.
     * 
     * @param custom_path Optional file path string for the log file.
     * @param level The spdlog::level to use across sinks.
     */
    static void Init(const std::string& custom_path = "", spdlog::level::level_enum level = spdlog::level::info);

    /**
     * @brief Get the global logger instance.
     * @return A shared pointer to the configured spdlog::logger.
     */
    static std::shared_ptr<spdlog::logger> Get();
};

#endif // LOGGER_HPP
