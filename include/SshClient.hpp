/**
 * SPDX-FileComment: Header for SSH and SFTP logic encapsulation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: MIT
 *
 * @file SshClient.hpp
 * @brief Header for SSH and SFTP logic encapsulation via libssh.
 * @version 1.1.0
 * @date 2026-03-04
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license MIT License
 */
#ifndef SSH_CLIENT_HPP
#define SSH_CLIENT_HPP

#include <string>
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

/**
 * @class SshClient
 * @brief Handles establishing SSH connections and securely copying files/folders over SFTP.
 */
class SshClient {
public:
    /**
     * @brief Constructs the SshClient with target credentials.
     * @param host Target Hostname or IP.
     * @param username SSH authentication username.
     * @param password SSH authentication password.
     * @param private_key_file Optional path to a private SSH key file.
     */
    SshClient(const std::string& host, const std::string& username, const std::string& password, const std::string& private_key_file = "");
    ~SshClient();

    /**
     * @brief Establishes the SSH connection, authenticates, and initializes the SFTP session.
     * @return True if connection and SFTP initialization succeed, false otherwise.
     */
    bool connect();

    /**
     * @brief Disconnects the SSH and SFTP sessions cleanly.
     */
    void disconnect();

    /**
     * @brief Recursively copies a local directory to a target remote directory.
     * If srcbase is provided, it reconstructs the remote path starting from srcbase.
     * 
     * @param local_source_dir Local path directory to scan recursively.
     * @param remote_target_dir Target path base on the remote host.
     * @param srcbase An optional structural filter string.
     * @param dry_run If true, logs actions without performing SFTP write operations.
     * @param exclude_filters A list of strings. If a local path contains any of these, it will be skipped.
     * @param num_threads The number of worker threads to spawn for parallel uploads.
     * @return True if folder and its contents were synchronized successfully.
     */
    bool copyFolderRecursively(const std::filesystem::path& local_source_dir, 
                               const std::string& remote_target_dir, 
                               const std::string& srcbase = "",
                               bool dry_run = false,
                               const std::vector<std::string>& exclude_filters = {},
                               int num_threads = 1);

private:
    /**
     * @brief Helper to recursively create a path of directories on the remote SFTP server.
     * Works similarly to 'mkdir -p'.
     * @param remote_dir Absolute string path indicating the folder chain to create.
     * @return True on success or if directory already exists.
     */
    bool createRemoteDir(const std::string& remote_dir);

    /**
     * @brief Checks if a remote file needs to be updated based on size and mtime.
     * @param local_file The local file path.
     * @param remote_file The remote file path.
     * @return True if the file should be uploaded, false if it's identical or newer locally.
     */
    bool checkRemoteFileNeedsUpdate(const std::filesystem::path& local_file, const std::string& remote_file);

    /**
     * @brief Helper to upload a single local file to a specified remote path utilizing chunk streaming.
     * @param local_file Absolute path to local file.
     * @param remote_file Absolute destination path on remote system.
     * @return True if securely transmitted.
     */
    bool uploadFile(const std::filesystem::path& local_file, const std::string& remote_file);

    std::string host_;
    std::string username_;
    std::string password_;
    std::string private_key_file_;
    
    ssh_session session_;
    sftp_session sftp_;
};

#endif // SSH_CLIENT_HPP
