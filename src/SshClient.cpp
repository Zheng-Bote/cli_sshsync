/**
 * SPDX-FileComment: Implementation of SSH and SFTP logic encapsulation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: MIT
 *
 * @file SshClient.cpp
 * @brief Implementation for SshClient.
 * @version 1.1.0
 * @date 2026-03-04
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license MIT License
 */
#include "SshClient.hpp"
#include "Logger.hpp"
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>

SshClient::SshClient(const std::string& host, const std::string& username, const std::string& password, const std::string& private_key_file)
    : host_(host), username_(username), password_(password), private_key_file_(private_key_file), session_(nullptr), sftp_(nullptr) {
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
        spdlog::error("Error connecting to host: {}", ssh_get_error(session_));
        return false;
    }

    // Try Public Key Auth first if keyfile provided
    bool authenticated = false;
    if (!private_key_file_.empty()) {
        spdlog::debug("Attempting public key authentication with keyfile: {}", private_key_file_);
        ssh_key privkey = nullptr;
        rc = ssh_pki_import_privkey_file(private_key_file_.c_str(), nullptr, nullptr, nullptr, &privkey);
        if (rc == SSH_OK) {
            rc = ssh_userauth_publickey(session_, nullptr, privkey);
            if (rc == SSH_AUTH_SUCCESS) {
                spdlog::info("SSH authenticated successfully using public key.");
                authenticated = true;
            } else {
                spdlog::warn("Public key authentication failed, falling back to password...");
            }
            ssh_key_free(privkey);
        } else {
            spdlog::warn("Failed to load private key file {}, falling back to password...", private_key_file_);
        }
    } else {
        // Try automatic SSH agent public key auth as fallback before password
        rc = ssh_userauth_publickey_auto(session_, nullptr, nullptr);
        if (rc == SSH_AUTH_SUCCESS) {
            spdlog::info("SSH authenticated successfully using SSH agent.");
            authenticated = true;
        }
    }

    // Authenticate with password as final fallback
    if (!authenticated) {
        if (!password_.empty()) {
            rc = ssh_userauth_password(session_, nullptr, password_.c_str());
            if (rc != SSH_AUTH_SUCCESS) {
                spdlog::error("Password authentication failed: {}", ssh_get_error(session_));
                return false;
            }
            spdlog::info("SSH authenticated successfully using password.");
        } else {
            spdlog::error("No valid authentication methods succeeded.");
            return false;
        }
    }

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

bool SshClient::checkRemoteFileNeedsUpdate(const std::filesystem::path& local_file, const std::string& remote_file) {
    sftp_attributes attr = sftp_stat(sftp_, remote_file.c_str());
    if (attr == nullptr) {
        // File doesn't exist on remote, needs upload
        return true;
    }

    struct stat local_stat;
    if (stat(local_file.string().c_str(), &local_stat) != 0) {
        // Can't stat local file, assume needs upload to be safe
        sftp_attributes_free(attr);
        return true;
    }

    bool needs_update = false;
    
    // Check size first
    if (attr->size != local_stat.st_size) {
        needs_update = true;
        spdlog::trace("File sizes differ for {} (Local: {}, Remote: {})", local_file.string(), local_stat.st_size, attr->size);
    } 
    // Check modification time (if remote is older than local)
    else if (attr->mtime < local_stat.st_mtime) {
        needs_update = true;
        spdlog::trace("Local file {} is newer than remote (Local mtime: {}, Remote mtime: {})", local_file.string(), local_stat.st_mtime, attr->mtime);
    }

    sftp_attributes_free(attr);
    return needs_update;
}

bool SshClient::copyFolderRecursively(const std::filesystem::path& local_source_dir, 
                                      const std::string& remote_target_dir, 
                                      const std::string& srcbase,
                                      bool dry_run,
                                      const std::vector<std::string>& exclude_filters,
                                      int num_threads) {
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

    if (num_threads < 1) num_threads = 1;

    std::mutex queue_mutex;
    std::condition_variable cv;
    std::queue<std::pair<std::filesystem::path, std::string>> file_queue;
    bool done_traversing = false;
    std::atomic<bool> success{true};

    std::vector<std::thread> workers;
    
    int actual_workers = (num_threads > 1) ? num_threads : 1;
    
    if (num_threads > 1) {
        spdlog::info("Spawning {} worker threads for parallel sync...", actual_workers);
        for (int i = 0; i < actual_workers; ++i) {
            workers.emplace_back([this, &queue_mutex, &cv, &file_queue, &done_traversing, &success, dry_run]() {
                SshClient worker_client(this->host_, this->username_, this->password_, this->private_key_file_);
                if (!worker_client.connect()) {
                    success = false;
                    return;
                }
                while (true) {
                    std::pair<std::filesystem::path, std::string> job;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        cv.wait(lock, [&]() { return done_traversing || !file_queue.empty(); });
                        if (done_traversing && file_queue.empty()) {
                            break;
                        }
                        job = file_queue.front();
                        file_queue.pop();
                    }
                    
                    std::string entry_str = job.first.string();
                    if (worker_client.checkRemoteFileNeedsUpdate(job.first, job.second)) {
                        if (dry_run) {
                            spdlog::info("[DRY-RUN] Would upload file (modified): {} -> {}", entry_str, job.second);
                        } else {
                            if (!worker_client.uploadFile(job.first, job.second)) {
                                success = false;
                            }
                        }
                    } else {
                        spdlog::debug("File up-to-date, skipping: {}", entry_str);
                    }
                }
                worker_client.disconnect();
            });
        }
    }

    // Initial remote target directory creation
    if (dry_run) {
        spdlog::info("[DRY-RUN] Would create base remote directory: {}", remote_target_resolved_str);
    } else {
        if (!createRemoteDir(remote_target_resolved_str)) {
            success = false;
            done_traversing = true;
            cv.notify_all();
            for (auto& t : workers) { if (t.joinable()) t.join(); }
            return false;
        }
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(abs_local_source)) {
        std::string entry_str = std::filesystem::absolute(entry.path()).string();
        
        // Exclude Filter Logic
        bool skip = false;
        for (const auto& filter : exclude_filters) {
            if (entry_str.find(filter) != std::string::npos) {
                spdlog::debug("Skipping {} (matches exclude filter: {})", entry_str, filter);
                skip = true;
                break;
            }
        }
        
        if (skip) continue;

        auto rel_path = std::filesystem::relative(entry.path(), abs_local_source);
        std::filesystem::path target_file_path = resolved_remote_base / rel_path;
        std::string remote_path = target_file_path.string();
        std::replace(remote_path.begin(), remote_path.end(), '\\', '/');

        if (std::filesystem::is_directory(entry.status())) {
            if (dry_run) {
                spdlog::info("[DRY-RUN] Would create directory: {}", remote_path);
            } else {
                if (!createRemoteDir(remote_path)) {
                    success = false;
                }
            }
        } else if (std::filesystem::is_regular_file(entry.status())) {
            if (num_threads > 1) {
                std::lock_guard<std::mutex> lock(queue_mutex);
                file_queue.push({entry.path(), remote_path});
                cv.notify_one();
            } else {
                if (checkRemoteFileNeedsUpdate(entry.path(), remote_path)) {
                    if (dry_run) {
                        spdlog::info("[DRY-RUN] Would upload file (modified): {} -> {}", entry_str, remote_path);
                    } else {
                        if (!uploadFile(entry.path(), remote_path)) {
                            success = false;
                        }
                    }
                } else {
                    spdlog::debug("File up-to-date, skipping: {}", entry_str);
                }
            }
        }
    }

    if (num_threads > 1) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            done_traversing = true;
        }
        cv.notify_all();
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
    }

    if (success) {
        spdlog::info("Folder synchronization completed successfully.");
    } else {
        spdlog::warn("Folder synchronization completed with errors.");
    }
    
    return success;
}
