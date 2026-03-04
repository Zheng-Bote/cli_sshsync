<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [Changelog](#changelog)
  - [[1.0.0] - 2026-03-04](#100---2026-03-04)
    - [Added](#added)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-03-04

### Added
- **Initial Release:** Core SSH Folder Sync application built with C++23.
- **Recursive SFTP Copy:** Reliable transfer of entire directory trees using `libssh`.
- **Environment Management:** Parse authentication and target path mappings from `.env` files (e.g., `USER_<name>_PASSWORD`, `HOST_<ip>_TARGETDIR`).
- **CLI Arguments Parsing:** Support for named parameters (`--user`, `--target`, `--src`) using `cxxopts`.
- **Advanced Path Restructuring (`--srcbase`):** Intelligently map local source paths to remote targets by mirroring directory structures or explicitly truncating base path prefixes.
- **Robust Logging:** Built-in console and file logging (`YYYY-MM-DD.log`) via `spdlog`.
- **Configurable `LOG_LEVEL`:** Dynamically control verbosity (`trace`, `debug`, `info`, `warn`, `err`, `critical`) through the `.env` file.
- **Automated Directory Creation:** Seamless recursive `mkdir -p` equivalent implementation over SFTP to automatically scaffold missing remote parent directories.
- **CI/CD Pipelines:** Comprehensive GitHub Actions workflows for CodeQL, Cppcheck, Clang-Tidy, SBOM generation, and automated markdown TOC updates.
