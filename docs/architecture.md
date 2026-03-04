# SSH Folder Sync Architecture

This document provides a comprehensive structural and behavioral overview of the SSH Folder Sync application (`v1.1.0`) utilizing Mermaid diagrams required by `GEMINI.md`.

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

## 1. Bounded Contexts
```mermaid
C4Context
  title Bounded Contexts - SSH Folder Sync

  Person(user, "User", "CLI User")
  System(ssh_sync, "SSH Folder Sync", "CLI App to sync folders")
  System_Ext(remote_server, "Remote Server", "SSH/SFTP Server")
  System_Ext(env_file, ".env Configuration", "Stores credentials")

  Rel(user, ssh_sync, "Executes with args")
  Rel(ssh_sync, env_file, "Reads credentials")
  Rel(ssh_sync, remote_server, "Uploads files via SFTP")
```

## 2. Class Diagram
```mermaid
classDiagram
  class EnvParser {
    -map env_map_
    +getPassword(username) string
    +getPrivateKeyFile(username) string
    +getTargetDir(host) string
  }
  class SshClient {
    -string host_
    -string username_
    -string password_
    -ssh_session session_
    -sftp_session sftp_
    +connect() bool
    +disconnect() void
    +copyFolderRecursively(src, tgt, srcbase, dry_run, exclude, threads) bool
    -checkRemoteFileNeedsUpdate(local, remote) bool
    -uploadFile(local, remote) bool
    -createRemoteDir(remote) bool
  }
  class Logger {
    +Init(level, log_path)
  }
  EnvParser <-- main : Uses
  SshClient <-- main : Uses
  Logger <-- main : Initializes
```

## 3. Sequence Diagram
```mermaid
sequenceDiagram
  participant User
  participant main
  participant SshClient
  participant RemoteServer

  User->>main: Run `./ssh_sync`
  main->>SshClient: Initialize(user, host, auth)
  main->>SshClient: connect()
  
  SshClient->>RemoteServer: SSH Handshake
  alt Keyfile Auth
    SshClient->>RemoteServer: PubKey Auth
  else Password Auth
    SshClient->>RemoteServer: Password Auth
  end
  
  SshClient-->>main: Connected & SFTP Ready
  main->>SshClient: copyFolderRecursively(num_threads)
  
  par Thread Pool Workers
    SshClient->>RemoteServer: sftp_stat (Delta Check)
    RemoteServer-->>SshClient: File mtime & size
    alt Modified
      SshClient->>RemoteServer: sftp_write (Chunks)
    end
  end
  
  SshClient->>RemoteServer: Disconnect Session
```

## 4. Activity Diagram
```mermaid
stateDiagram-v2
  [*] --> ParseCLI
  ParseCLI --> ReadEnv: Load credentials
  ReadEnv --> ConnectSSH: Authenticate
  ConnectSSH --> WalkLocalDir: Iterate local files
  WalkLocalDir --> CheckExclude
  CheckExclude --> Filtered: Matches exclude
  CheckExclude --> EvaluateDelta: No match
  EvaluateDelta --> EnqueueUpload: File modified/new
  EvaluateDelta --> Skip: Up-to-date
  Filtered --> WalkLocalDir
  Skip --> WalkLocalDir
  
  state ThreadPool {
    EnqueueUpload --> Uploading
    Uploading --> FinishedUpload
  }
  FinishedUpload --> WalkLocalDir
  
  WalkLocalDir --> Disconnect: Traversal complete
  Disconnect --> [*]
```

## 5. State Diagram
```mermaid
stateDiagram-v2
  [*] --> Disconnected
  Disconnected --> Authenticating : connect()
  Authenticating --> Connected : Success
  Authenticating --> Disconnected : Failure (Password/Key invalid)
  Connected --> Transferring : copyFolderRecursively()
  Transferring --> Connected : queue complete
  Connected --> Disconnected : disconnect()
```

## 6. Component Diagram
```mermaid
C4Component
  title Component Diagram - SSH Folder Sync
  
  Container_Boundary(app, "SSH Sync App") {
    Component(cli, "CLI Parser", "cxxopts", "Parses arguments")
    Component(env, "EnvManager", "EnvParser", "Loads .env variables")
    Component(ssh, "SSH Engine", "SshClient", "Handles libssh/SFTP logic")
    Component(log, "Logger", "spdlog", "Writes logs")
  }
  
  Rel(cli, env, "Requests credentials")
  Rel(cli, ssh, "Invokes threaded sync")
  Rel(ssh, log, "Logs events/errors")
```

## 7. Deployment Diagram
```mermaid
flowchart TD
    subgraph Local_Environment
        A[ssh_sync Binary]
        B[.env File]
        A -->|Reads| B
    end
    
    subgraph Network
        C((SSH/SFTP Tunneled TCP Connection))
    end
    
    subgraph Remote_Environment
        D[OpenSSH Daemon]
        E[(Remote Filesystem)]
        D -->|Writes Chunked Data| E
    end
    
    A <-->|Port 22| C
    C <--> D
```

## 8. Use Case Diagram
```mermaid
flowchart LR
    User([CLI User])
    
    subgraph SSH Folder Sync Application
        UC1(Sync Local Folder to Remote)
        UC2(Simulate Sync / Dry-Run)
        UC3(Exclude Specific Paths)
        UC4(Upload via Multiple Threads)
    end
    
    User --> UC1
    User --> UC2
    User --> UC3
    User --> UC4
```

## 9. Entity Relationship Diagram
```mermaid
erDiagram
    ENVIRONMENT_FILE ||--o{ AUTH_CREDENTIAL : contains
    ENVIRONMENT_FILE ||--o{ HOST_MAPPING : defines
    
    AUTH_CREDENTIAL {
        string username
        string password
        string private_key_path
    }
    HOST_MAPPING {
        string target_hostname
        string target_base_path
    }
```

## 10. Flowchart
```mermaid
flowchart TD
    Start([Start]) --> Parse[Parse CLI Arguments]
    Parse --> IsDryRun{Is Dry Run?}
    IsDryRun -->|Yes| SetDry[Set Dry-Run Flags]
    IsDryRun -->|No| Init[Init SshClient]
    SetDry --> Init
    Init --> Threads{Threads > 1?}
    Threads -->|Yes| SpawnWorkers[Spawn N Worker Threads]
    Threads -->|No| SingleWorker[Sequential Processing]
    SpawnWorkers --> Traverse[Walk std::filesystem]
    SingleWorker --> Traverse
    Traverse --> End([Disconnect && Exit])
```

## 11. Functional Decomposition Diagram
```mermaid
mindmap
  root((SSH Folder Sync))
    CLI Interface Layer
      Argument Parsing
      Help Documentation
    Configuration Layer
      Key-Value Parsing
      Fallback Mechanisms
    Operational Layer
      Thread Pool / Async
      Differential Delta Logic
      Exclusion Filtering
    Network Layer
      SSH Handshakes
      Public-Key cryptography
      SFTP byte streams
```

## 12. Information Flow Diagram
```mermaid
flowchart LR
    LocalFS[(Local Disk)] -->|I/O Read| FileTraverser
    FileTraverser -->|File Paths| JobQueue
    JobQueue -->|Pop Task| ThreadWorkers
    EnvParser -->|Auth Tokens| ThreadWorkers
    ThreadWorkers -->|SFTP Stat Data| DeltaComparison
    DeltaComparison -->|Binary Bytes| SSHSocket
    SSHSocket -->|Network| RemoteFS[(Remote Target Disk)]
```

## 13. Logical Decomposition Diagram
```mermaid
graph TD
    APP[Application Controller] --> AUTH[Authentication Service]
    APP --> SYNC[Synchronization Engine]
    SYNC --> DELTA[Delta Analysis / Stat]
    SYNC --> THREADS[Concurrency Manager]
    SYNC --> FILTER[Path Exclusion Filter]
    AUTH --> LIBCRYPTO[Underlying Cryptography]
```

## 14. Physical Decomposition Diagram
```mermaid
graph TD
    subgraph Compile-Time Dependencies
        A[ssh_sync executable] 
        B[spdlog: static/inline]
        C[cxxopts: header-only]
    end
    
    subgraph Runtime Dependencies
        A -.->|Dynamic Link| D[libssh.dylib / .so]
        A -.->|Dynamic Link| E[libc++]
    end
```

## 15. Development View
```mermaid
flowchart TB
    subgraph Source Code
        M(src/main.cpp)
        E(src/EnvParser.cpp)
        S(src/SshClient.cpp)
        L(src/Logger.cpp)
    end
    subgraph Headers
        Eh(include/EnvParser.hpp)
        Sh(include/SshClient.hpp)
        Lh(include/Logger.hpp)
    end
    M --> Eh & Sh & Lh
    E --> Eh
    S --> Sh & Lh
    L --> Lh
```

## 16. Deployment View
```mermaid
flowchart TD
    subgraph Deployment Node A [User Workstation]
        CLI[Terminal Executable: ssh_sync]
        CONFIG[.env file]
        CLI -.->|Reads| CONFIG
    end
    
    subgraph Deployment Node B [CI/CD or Staging Server]
        SSHD[SSH Daemon]
        ROOTDIR[/target_directory/]
        SSHD -->|Applies changes| ROOTDIR
    end
    
    CLI <==>|Encrypted Tunnel| SSHD
```
