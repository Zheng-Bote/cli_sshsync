#ifndef SSH_CLIENT_HPP
#define SSH_CLIENT_HPP

#include <string>
#include <filesystem>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

class SshClient {
public:
    SshClient(const std::string& host, const std::string& username, const std::string& password);
    ~SshClient();

    // Establishes the SSH connection, authenticates, and initializes the SFTP session.
    bool connect();

    // Disconnects and cleans up resources.
    void disconnect();

    // Recursively copies a local directory to a target remote directory.
    // If srcbase is provided, it reconstructs the remote path starting from srcbase.
    bool copyFolderRecursively(const std::filesystem::path& local_source_dir, const std::string& remote_target_dir, const std::string& srcbase);

private:
    // Helper to recursively create a directory on the remote SFTP server if it doesn't exist.
    bool createRemoteDir(const std::string& remote_dir);

    // Helper to upload a single file in chunks.
    bool uploadFile(const std::filesystem::path& local_file, const std::string& remote_file);

    std::string host_;
    std::string username_;
    std::string password_;
    
    ssh_session session_;
    sftp_session sftp_;
};

#endif // SSH_CLIENT_HPP
