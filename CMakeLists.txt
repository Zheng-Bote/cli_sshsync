cmake_minimum_required(VERSION 3.23)
project(ssh_copy_app VERSION 1.0.0 LANGUAGES CXX)

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
target_include_directories(ssh_sync PRIVATE src)

# Link Libraries
target_link_libraries(ssh_sync PRIVATE 
    spdlog::spdlog
    cxxopts::cxxopts
    ssh
)
