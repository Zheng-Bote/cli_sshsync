#include <iostream>
#include <string>
#include <filesystem>
#include "cxxopts.hpp"
#include "Logger.hpp"
#include "EnvParser.hpp"
#include "SshClient.hpp"

int main(int argc, char* argv[]) {
    cxxopts::Options options("ssh_sync", "Recursively copy folders to remote server via SSH/SFTP");

    options.add_options()
        ("env", "Path to .env file", cxxopts::value<std::string>()->default_value((std::filesystem::current_path() / ".env").string()))
        ("log", "Path to log file (default: ./<YYYY-MM-DD>.log)", cxxopts::value<std::string>()->default_value(""))
        ("srcbase", "Optional path filter to begin syncing from", cxxopts::value<std::string>()->default_value(""))
        ("u,user", "SSH Username", cxxopts::value<std::string>())
        ("t,target", "Target Hostname/IP", cxxopts::value<std::string>())
        ("s,src", "Local Source Folder", cxxopts::value<std::string>())
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // Delay Logger initialization until we have ENV parsed
    std::string env_path = result["env"].as<std::string>();
    std::string log_path = result["log"].as<std::string>();

    if (!result.count("user") || !result.count("target") || !result.count("src")) {
        spdlog::error("Missing required arguments.");
        std::cerr << "Usage: ssh_sync --user <username> --target <target_host> --src <source_folder> [OPTIONS]\n";
        std::cerr << "Run with --help for more information.\n";
        return 1;
    }

    std::string username = result["user"].as<std::string>();
    std::string target_host = result["target"].as<std::string>();
    std::string source_folder = result["src"].as<std::string>();
    std::string srcbase = result["srcbase"].as<std::string>();

    // Parse Environment Variables early to determine log level
    EnvParser env_parser(env_path);
    if (!env_parser.load()) {
        Logger::Init(log_path); // fallback
        spdlog::error("Failed to load environment file. Ensure {} exists.", env_path);
        return 1;
    }

    // Determine Log Level
    spdlog::level::level_enum log_level = spdlog::level::info;
    auto opt_log_level = env_parser.getValue("LOG_LEVEL");
    if (opt_log_level) {
        std::string ll = *opt_log_level;
        std::transform(ll.begin(), ll.end(), ll.begin(), ::tolower);
        if (ll == "trace") log_level = spdlog::level::trace;
        else if (ll == "debug") log_level = spdlog::level::debug;
        else if (ll == "info") log_level = spdlog::level::info;
        else if (ll == "warn" || ll == "warning") log_level = spdlog::level::warn;
        else if (ll == "err" || ll == "error") log_level = spdlog::level::err;
        else if (ll == "critical") log_level = spdlog::level::critical;
    }

    // Initialize Logger
    Logger::Init(log_path, log_level);

    spdlog::info("Starting SSH Sync Application");
    spdlog::info("Username: {}", username);
    spdlog::info("Target Host: {}", target_host);
    spdlog::info("Source Folder: {}", source_folder);
    if (opt_log_level) {
        spdlog::info("Custom LOG_LEVEL active: {}", *opt_log_level);
    }

    if (!std::filesystem::exists(source_folder) || !std::filesystem::is_directory(source_folder)) {
        spdlog::error("Source folder does not exist or is not a directory: {}", source_folder);
        return 1;
    }

    auto opt_password = env_parser.getPassword(username);
    if (!opt_password) {
        spdlog::error("No password found for username '{}' in .env file (looked for USER_{}_PASSWORD).", username, username);
        return 1;
    }

    auto opt_target_dir = env_parser.getTargetDir(target_host);
    if (!opt_target_dir) {
        spdlog::error("No target directory found for host '{}' in .env file (looked for HOST_{}_TARGETDIR).", target_host, target_host);
        return 1;
    }

    std::string password = *opt_password;
    std::string remote_target_dir = *opt_target_dir;

    spdlog::info("Resolved target directory from .env: {}", remote_target_dir);

    // Initialize and Execute SSH Client
    SshClient client(target_host, username, password);
    if (!client.connect()) {
        spdlog::error("Failed to establish SSH/SFTP connection.");
        return 1;
    }

    if (!client.copyFolderRecursively(source_folder, remote_target_dir, srcbase)) {
        spdlog::error("Folder copy failed.");
        client.disconnect();
        return 1;
    }

    client.disconnect();
    spdlog::info("Application finished successfully.");
    
    return 0;
}
