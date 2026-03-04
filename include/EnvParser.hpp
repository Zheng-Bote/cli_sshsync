/**
 * SPDX-FileComment: Header for EnvParser configuration handling
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: MIT
 *
 * @file EnvParser.hpp
 * @brief Header for EnvParser configuration handling.
 * @version 1.1.0
 * @date 2026-03-04
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license MIT License
 */
#ifndef ENV_PARSER_HPP
#define ENV_PARSER_HPP

#include <string>
#include <unordered_map>
#include <optional>

/**
 * @class EnvParser
 * @brief Parses key-value pairs from a .env file and provides typed access to them.
 */
class EnvParser {
public:
    /**
     * @brief Constructor for EnvParser.
     * @param env_filepath Path to the .env file to parse.
     */
    explicit EnvParser(const std::string& env_filepath);

    /**
     * @brief Loads and parses the environment file.
     * @return True if successfully loaded, false if file missing or unreadable.
     */
    bool load();

    /**
     * @brief Retrieves the password associated with a specific username.
     * @param username The username used to form the key "USER_{username}_PASSWORD".
     * @return Optional string containing the password, or std::nullopt if missing.
     */
    std::optional<std::string> getPassword(const std::string& username) const;

    /**
     * @brief Retrieves the private key file path associated with a specific username.
     * @param username The username used to form the key "USER_{username}_KEYFILE".
     * @return Optional string containing the path to the private key, or std::nullopt if missing.
     */
    std::optional<std::string> getPrivateKeyFile(const std::string& username) const;

    /**
     * @brief Retrieves the target directory associated with a specific host.
     * @param host The hostname used to form the key "HOST_{host}_TARGETDIR".
     * @return Optional string containing the target directory path, or std::nullopt if missing.
     */
    std::optional<std::string> getTargetDir(const std::string& host) const;

    /**
     * @brief Retrieves a generic key from the environment file.
     * @param key The specific environment key to look up (e.g. "LOG_LEVEL").
     * @return Optional string containing the value, or std::nullopt if missing.
     */
    std::optional<std::string> getValue(const std::string& key) const;

private:
    std::string filepath_;
    std::unordered_map<std::string, std::string> env_map_;
};

#endif // ENV_PARSER_HPP
