# SPDX-FileComment: CMake Build configuration for SSH Folder Sync
# SPDX-FileType: SOURCE
# SPDX-FileContributor: ZHENG Robert
# SPDX-FileCopyrightText: 2026 ZHENG Robert
# SPDX-License-Identifier: MIT
#
# @file CMakeLists.txt
# @brief CMake build configuration for SSH Folder Sync project.
# @version 1.1.0
# @date 2026-03-04
#
# @author ZHENG Robert (robert@hase-zheng.net)
# @copyright Copyright (c) 2026 ZHENG Robert
#
# @license MIT License

cmake_minimum_required(VERSION 3.23)
project(ssh_copy_app
    VERSION 1.1.0
    DESCRIPTION "SSH Folder Sync"
    HOMEPAGE_URL "https://github.com/Zheng-Bote/cli_sshsync"
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Includes FetchContent
include(FetchContent)

# Fetch spdlog for logging
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# Fetch cxxopts for CLI argument parsing
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        v3.2.0
)
FetchContent_MakeAvailable(cxxopts)

# Find libssh locally
# Requires: brew install libssh
find_package(libssh REQUIRED)

# Executable and Sources
add_executable(ssh_sync
    src/main.cpp
    src/EnvParser.cpp
    src/Logger.cpp
    src/SshClient.cpp
)

# Include directories
target_include_directories(ssh_sync PRIVATE include)

# Link Libraries
target_link_libraries(ssh_sync PRIVATE 
    spdlog::spdlog
    cxxopts::cxxopts
    ssh
)

# --- Install Configuration ---
include(GNUInstallDirs)

install(TARGETS ssh_sync
    EXPORT ssh_syncTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Optional: Install the env.example
install(FILES env.example
    DESTINATION ${CMAKE_INSTALL_DATADIR}/ssh_sync
)

# --- Custom Generators (Original Code) ---

# CMake File API Query (Datei erzeugen)
set(QUERY_DIR "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/client-project")
file(MAKE_DIRECTORY "${QUERY_DIR}")
file(WRITE "${QUERY_DIR}/codemodel-v2" "") 

# libs.txt Generierung (Timing-sicher via GENERATE)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/libs.txt" 
     CONTENT "$<TARGET_PROPERTY:ssh_sync,LINK_LIBRARIES>")

# --- CPack Configuration ---
set(CPACK_GENERATOR "DragNDrop")
set(CPACK_PACKAGE_NAME "ssh_sync")
set(CPACK_PACKAGE_VERSION "1.1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SSH Folder Sync CLI Tool")
set(CPACK_PACKAGE_VENDOR "ZHENG Robert")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macOS")
set(CPACK_DMG_VOLUME_NAME "SSH Sync Installer")

include(CPack)