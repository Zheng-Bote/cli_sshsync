# SSH Folder Sync 

A C++23 command-line application that recursively copies a local folder to a remote server using SFTP via SSH.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)]()
[![CMake](https://img.shields.io/badge/CMake-3.23+-blue.svg)]()

[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Zheng-Bote/cli_sshsync?logo=GitHub)](https://github.com/Zheng-Bote/cli_sshsync/releases)

[Report Issue](https://github.com/Zheng-Bote/cli_sshsync/issues) · [Request Feature](https://github.com/Zheng-Bote/cli_sshsync/pulls)

---

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [Prerequisites](#prerequisites)
  - [1. C++ Compiler](#1-c-compiler)
  - [2. Install `libssh`](#2-install-libssh)
- [Building the Project](#building-the-project)
- [Usage](#usage)
  - [Required Arguments:](#required-arguments)
  - [Optional Arguments:](#optional-arguments)
- [Notes](#notes)
- [The `.env` File Format](#the-env-file-format)
- [Example](#example)
- [📜 License](#-license)
- [📄 Changelog](#-changelog)
- [Author](#author)
- [Code Contributors](#code-contributors)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

---

## Prerequisites

### 1. C++ Compiler
You need a C++ compiler that supports C++23 (e.g., modern GCC, Clang, or MSVC) and CMake 3.20+.

### 2. Install `libssh`
The application relies on `libssh` for secure SSH and SFTP connections. Since it requires system-level cryptography libraries like OpenSSL, it is not bundled via CMake FetchContent and must be installed manually.

**macOS (Homebrew):**
```bash
brew install libssh
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libssh-dev
```

*Other C++ dependencies (`spdlog` and `cxxopts`) are automatically downloaded during the CMake build phase.*

## Building the Project

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./ssh_sync --user <username> --target <target_host> --src <source_folder> [OPTIONS] 
```

### Required Arguments:
* `--user <username>`: The SSH username for the remote host.
* `--target <target_host>`: The hostname or IP address of the remote server.
* `--src <source_folder>`: The local directory you want to copy recursively.

### Optional Arguments:
* `--srcbase <string>`: An optional path segment filter. The app will sync the entire folder structure *starting from* this segment and append it to the remote target directory. (e.g. `--srcbase Documents`).
* `--env <path>`: Path to the `.env` file containing passwords and target paths. (Default: `./.env` in current working directory).
* `--log <path>`: Path to the log file. (Default: `./<YYYY-MM-DD>.log` in the current working directory).

### Notes
* The app will create the target directory on the remote server if it doesn't exist.
* The app will create the log file on the local machine if it doesn't exist.

## The `.env` File Format

The application expects an environment file to map usernames to passwords and hosts to target directories on the remote server.

See `.env.example` for details:
```env
# Optional: Override the default log level (trace, debug, info, warn, err, critical)
# Default is 'info'.
LOG_LEVEL=info

# User Passwords
# Format: USER_<username>_PASSWORD=...
USER_admin_PASSWORD=my_secure_password
USER_deploy_PASSWORD=another_password

# Host Target Directories
# Format: HOST_<hostname>_TARGETDIR=...
HOST_server1.example.com_TARGETDIR=/var/www/html
HOST_192.168.1.10_TARGETDIR=/opt/backup/folder
```

If you run the command `./ssh_sync --user admin --target server1.example.com --src ./local_data`, the app will look up `USER_admin_PASSWORD` and `HOST_server1.example.com_TARGETDIR` in the `.env` file.

## Example
```bash
./ssh_sync --user zb_bamboo --target 192.168.1.55 --src /Users/zb_bamboo/Documents/DEV/Rust/Geocoder --env .localserver_env --srcbase ~/Documents/DEV
```

```text
[info] Starting SSH Sync Application
[info] Username: zb_bamboo
[info] Target Host: 192.168.1.55
[info] Source Folder: /Users/zb_bamboo/Documents/DEV/Rust/Geocoder
[info] Custom LOG_LEVEL active: info
[info] Resolved target directory from .env: /home/zb_bamboo/target
[info] Connecting to 192.168.1.55 as zb_bamboo...
[info] SSH authenticated successfully.
[info] SFTP session initialized.
[info] Starting recursive copy of /Users/zb_bamboo/Documents/DEV/Rust/Geocoder to remote /home/zb_bamboo/target/Rust/Geocoder
[info] Folder synchronization completed successfully.
[info] SSH connection closed.
[info] Application finished successfully.
[info] SSH connection closed.
```

---

## 📜 License

This project is licensed under the MIT License - see the LICENSE file for details.

Copyright (c) 2026 ZHENG Robert

## 📄 Changelog

For a detailed history of changes, see the [CHANGELOG.md](CHANGELOG.md).

## Author

[![Zheng Robert - Core Development](https://img.shields.io/badge/Github-Zheng_Robert-black?logo=github)](https://www.github.com/Zheng-Bote)

## Code Contributors

![Contributors](https://img.shields.io/github/contributors/Zheng-Bote/cli_sshsync?color=dark-green)

---

**Happy syncing! 🚀** :vulcan_salute:

