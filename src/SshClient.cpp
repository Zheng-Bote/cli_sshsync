#include "SshClient.hpp"
#include "Logger.hpp"
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>

SshClient::SshClient(const std::string& host, const std::string& username, const std::string& password)
    : host_(host), username_(username), password_(password), session_(nullptr), sftp_(nullptr) {
}

SshClient::~SshClient() {
    disconnect();
}

bool SshClient::connect() {
    spdlog::info("Connecting to {} as {}...", host_, username_);
    
    session_ = ssh_new();
    if (session_ == nullptr) {
        spdlog::error("Failed to create SSH session.");
        return false;
    }

    ssh_options_set(session_, SSH_OPTIONS_HOST, host_.c_str());
    ssh_options_set(session_, SSH_OPTIONS_USER, username_.c_str());

    int rc = ssh_connect(session_);
    if (rc != SSH_OK) {
        spdlog::error("Error connecting to localhost: {}", ssh_get_error(session_));
        return false;
    }

    // Authenticate with password
    rc = ssh_userauth_password(session_, nullptr, password_.c_str());
    if (rc != SSH_AUTH_SUCCESS) {
        spdlog::error("Authentication failed: {}", ssh_get_error(session_));
        return false;
    }

    spdlog::info("SSH authenticated successfully.");

    // Initialize SFTP
    sftp_ = sftp_new(session_);
    if (sftp_ == nullptr) {
        spdlog::error("Failed to allocate SFTP session: {}", ssh_get_error(session_));
        return false;
    }

    rc = sftp_init(sftp_);
    if (rc != SSH_OK) {
        spdlog::error("Failed to initialize SFTP session: {}", sftp_get_error(sftp_));
        sftp_free(sftp_);
        sftp_ = nullptr;
        return false;
    }

    spdlog::info("SFTP session initialized.");
    return true;
}

void SshClient::disconnect() {
    if (sftp_ != nullptr) {
        sftp_free(sftp_);
        sftp_ = nullptr;
    }
    if (session_ != nullptr) {
        ssh_disconnect(session_);
        ssh_free(session_);
        session_ = nullptr;
    }
    spdlog::info("SSH connection closed.");
}

bool SshClient::createRemoteDir(const std::string& remote_dir) {
    if (remote_dir.empty()) return true;

    std::string current_path = "";
    bool absolute = (remote_dir[0] == '/');
    if (absolute) {
        current_path = "/";
    }

    size_t pos = absolute ? 1 : 0;
    while (pos < remote_dir.length()) {
        size_t next_pos = remote_dir.find('/', pos);
        if (next_pos == std::string::npos) {
            next_pos = remote_dir.length();
        }

        std::string component = remote_dir.substr(pos, next_pos - pos);
        if (!component.empty()) {
            if (current_path != "/" && !current_path.empty()) {
                current_path += "/";
            }
            current_path += component;

            sftp_attributes attr = sftp_stat(sftp_, current_path.c_str());
            if (attr != nullptr) {
                bool is_dir = (attr->type == SSH_FILEXFER_TYPE_DIRECTORY);
                sftp_attributes_free(attr);
                if (!is_dir) {
                    spdlog::error("Path {} exists but is not a directory.", current_path);
                    return false;
                }
            } else {
                spdlog::debug("Creating remote directory: {}", current_path);
                int rc = sftp_mkdir(sftp_, current_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                if (rc != SSH_OK) {
                    spdlog::error("Failed to create remote directory {}: SFTP error {}", current_path, sftp_get_error(sftp_));
                    return false;
                }
            }
        }
        pos = next_pos + 1;
    }
    return true;
}

bool SshClient::uploadFile(const std::filesystem::path& local_file, const std::string& remote_file) {
    spdlog::debug("Uploading {} to {}", local_file.string(), remote_file);

    sftp_file sftpFile = sftp_open(sftp_, remote_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (sftpFile == nullptr) {
        spdlog::error("Failed to open remote file {}: SFTP error {}", remote_file, sftp_get_error(sftp_));
        return false;
    }

    std::ifstream file(local_file, std::ios::binary);
    if (!file.is_open()) {
        spdlog::error("Failed to open local file {}", local_file.string());
        sftp_close(sftpFile);
        return false;
    }

    // Chunk size 32KB
    const size_t chunk_size = 32768;
    std::vector<char> buffer(chunk_size);
    
    size_t total_written = 0;
    while (!file.eof()) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
            ssize_t bytes_written = sftp_write(sftpFile, buffer.data(), bytes_read);
            if (bytes_written != bytes_read) {
                spdlog::error("Failed to write to remote file {}: SFTP error {}", remote_file, sftp_get_error(sftp_));
                sftp_close(sftpFile);
                return false;
            }
            total_written += bytes_written;
        }
    }

    sftp_close(sftpFile);
    spdlog::trace("Successfully uploaded {} ({} bytes)", local_file.string(), total_written);
    return true;
}

bool SshClient::copyFolderRecursively(const std::filesystem::path& local_source_dir, const std::string& remote_target_dir, const std::string& srcbase) {
    if (!std::filesystem::exists(local_source_dir) || !std::filesystem::is_directory(local_source_dir)) {
        spdlog::error("Local source directory does not exist or is not a directory: {}", local_source_dir.string());
        return false;
    }

    // Resolve the actual base path to deploy to
    std::filesystem::path resolved_remote_base = remote_target_dir;
    
    // Resolve Absolute local path (handles relative paths like "./folder")
    std::filesystem::path abs_local_source = std::filesystem::absolute(local_source_dir).lexically_normal();
    std::string abs_local_str = abs_local_source.string();
    
    // Calculate the 'appended' path portion based on srcbase logic
    std::string path_append;
    if (!srcbase.empty()) {
        // Resolve srcbase to an absolute path for reliable prefix matching
        std::filesystem::path abs_srcbase = std::filesystem::absolute(srcbase).lexically_normal();
        std::string srcbase_str = abs_srcbase.string();

        auto pos = abs_local_str.find(srcbase_str);
        if (pos == 0) {
            // Found srcbase at the start. Exclude it by taking the substring after it.
            path_append = abs_local_str.substr(srcbase_str.length());
            spdlog::debug("Applying srcbase filter '{}'. Remaining path to append: '{}'.", srcbase_str, path_append);
        } else {
            spdlog::warn("Provided --srcbase '{}' was not found at the start of source path '{}'. Ignoring srcbase filter.", srcbase_str, abs_local_str);
            // Default fallback: just append the deepest folder name
            path_append = abs_local_source.filename().string();
        }
    } else {
        // Standard scp behavior: append just the final local folder name (e.g. "2026")
        path_append = abs_local_source.filename().string();
    }

    // Clean up leading slashes in path_append
    std::replace(path_append.begin(), path_append.end(), '\\', '/');
    while (!path_append.empty() && path_append.front() == '/') {
        path_append.erase(path_append.begin());
    }

    // Clean up trailing slashes in remote_target_dir
    std::string remote_target_resolved_str = remote_target_dir;
    std::replace(remote_target_resolved_str.begin(), remote_target_resolved_str.end(), '\\', '/');
    while (!remote_target_resolved_str.empty() && remote_target_resolved_str.back() == '/') {
        remote_target_resolved_str.pop_back();
    }
    
    // Combine base and append
    if (!path_append.empty()) {
        remote_target_resolved_str += "/" + path_append;
    }

    // Use this as our base for relative file placements
    resolved_remote_base = remote_target_resolved_str;
    std::replace(remote_target_resolved_str.begin(), remote_target_resolved_str.end(), '\\', '/');

    spdlog::info("Starting recursive copy of {} to remote {}", abs_local_str, remote_target_resolved_str);

    // Initial remote target directory creation
    if (!createRemoteDir(remote_target_resolved_str)) {
        return false;
    }

    bool success = true;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(abs_local_source)) {
        auto rel_path = std::filesystem::relative(entry.path(), abs_local_source);
        
        std::filesystem::path target_file_path = resolved_remote_base / rel_path;
        std::string remote_path = target_file_path.string();
        std::replace(remote_path.begin(), remote_path.end(), '\\', '/');

        if (std::filesystem::is_directory(entry.status())) {
            if (!createRemoteDir(remote_path)) {
                success = false;
            }
        } else if (std::filesystem::is_regular_file(entry.status())) {
            if (!uploadFile(entry.path(), remote_path)) {
                success = false;
            }
        }
    }

    if (success) {
        spdlog::info("Folder synchronization completed successfully.");
    } else {
        spdlog::warn("Folder synchronization completed with errors.");
    }
    
    return success;
}
