/**
 * SPDX-FileComment: Implementation of EnvParser configuration handling
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: MIT
 *
 * @file EnvParser.cpp
 * @brief Implementation for EnvParser.
 * @version 1.1.0
 * @date 2026-03-04
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license MIT License
 */
#include "EnvParser.hpp"
#include "Logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

EnvParser::EnvParser(const std::string& env_filepath) : filepath_(env_filepath) {}

bool EnvParser::load() {
    std::ifstream file(filepath_);
    if (!file.is_open()) {
        spdlog::error("Could not open environment file: {}", filepath_);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove trailing \r if any (e.g. from Windows CRLF files)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Ignore empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            auto trim = [](std::string& s) {
                s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
            };
            trim(key);
            trim(value);

            if (!key.empty()) {
                env_map_[key] = value;
            }
        }
    }
    
    spdlog::debug("Loaded {} environment variables from {}", env_map_.size(), filepath_);
    return true;
}

std::optional<std::string> EnvParser::getPassword(const std::string& username) const {
    std::string key = "USER_" + username + "_PASSWORD";
    auto it = env_map_.find(key);
    if (it != env_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string> EnvParser::getPrivateKeyFile(const std::string& username) const {
    std::string key = "USER_" + username + "_KEYFILE";
    auto it = env_map_.find(key);
    if (it != env_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string> EnvParser::getTargetDir(const std::string& host) const {
    std::string key = "HOST_" + host + "_TARGETDIR";
    return getValue(key);
}

std::optional<std::string> EnvParser::getValue(const std::string& key) const {
    auto it = env_map_.find(key);
    if (it != env_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}
