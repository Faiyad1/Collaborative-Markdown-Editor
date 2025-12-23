# Collaborative Markdown Editor

**Systems Programming**  
The University of Sydney  
20 May 2025

<mark>**Academic Integrity:** The use of AI tools (e.g., ChatGPT, GitHub Copilot) is not permitted.</mark>

A collaborative markdown document editing system implementing a client-server architecture.

## Overview

A real-time, multi-threaded collaborative editing system built in C for Linux/Unix environments. The system utilizes a Client-Server architecture where a central server manages an authoritative markdown document. By leveraging POSIX threads, named pipes (FIFOs), and real-time signals, multiple users can concurrently edit, format, and synchronize document state with version control to prevent race conditions.

## Concepts Covered

This project demonstrates key systems programming topics:

- **Advanced C** - Dynamic memory allocation, pointers, linked data structures
- **Processes** - Process creation (fork()), process IDs (getpid())
- **Signals** - Real-time signal handling (`SIGRTMIN`, `SIGRTMIN+1`)
- **IPC** - Inter-process communication via named pipes (FIFOs)
- **Synchronisation** - Mutexes for thread-safe document access
- **POSIX Threads** - Multi-threaded server with one thread per client
- **Parallelism** - Concurrent client handling with epoll for async I/O

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
- [Configuration](#configuration)
- [Commands](#commands)
- [Response Codes](#response-codes)
- [Output](#output)
- [Testing & Memory Safety](#testing--memory-safety)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Markdown Syntax Reference](#markdown-syntax-reference)
- [Notes](#notes)

## Features

- Multi-client support with concurrent connections (one POSIX thread per client)
- Role-based access control (`read` / `write` permissions)
- Real-time document synchronization via versioning
- Full markdown formatting support
- IPC using named pipes (FIFOs) and POSIX signals
- Automatic ordered list renumbering

## Requirements

- Linux/Unix system (uses POSIX APIs: signals, pthreads, epoll, FIFOs)
- GCC compiler
- pthread library

## Building

```bash
make
```

This compiles both `server` and `client` executables.

To clean build artifacts:
```bash
make clean
```

## Usage

### Starting the Server

```bash
./server <TIME_INTERVAL>
```

- `TIME_INTERVAL`: Version update interval in milliseconds

The server prints its PID on startup:
```
Server PID: <pid>
```

### Connecting a Client

```bash
./client <server_pid> <username>
```

- `server_pid`: The PID printed by the server
- `username`: Must match an entry in `roles.txt`

### Server Shutdown

Type `QUIT` in the server terminal when no clients are connected. The document is saved to `doc.md` on shutdown.

## Configuration

### roles.txt

Define user permissions in `roles.txt` (same directory as server):

```
username role
```

Example:
```
bob write
eve read
ryan write
```

Roles:
- `write` - Can view and modify the document
- `read` - Can only view the document

## Commands

### Editing Commands

| Command                  | Description                             |
|--------------------------|-----------------------------------------|
| `INSERT <pos> <content>` | Insert text at position                 |
| `DEL <pos> <count>`      | Delete `count` characters from position |

### Formatting Commands

| Command | Description |
|---------|-------------|
| `NEWLINE <pos>`            | Insert newline at position                   |
| `HEADING <level> <pos>`    | Insert heading (level 1-3) at position       |
| `BOLD <start> <end>`       | Apply **bold** formatting to range           |
| `ITALIC <start> <end>`     | Apply *italic* formatting to range           |
| `CODE <start> <end>`       | Apply `code` formatting to range             |
| `LINK <start> <end> <url>` | Create [link](https://github.com/Faiyad1/Collaborative-Markdown-Editor?tab=readme-ov-file#commands) from text in range |
| `BLOCKQUOTE <pos>`         | Insert blockquote (`>`) at position          |
| `ORDERED_LIST <pos>`       | Insert ordered list item (`1.`) at position  |
| `UNORDERED_LIST <pos>`     | Insert unordered list item (`-`) at position |
| `HORIZONTAL_RULE <pos>`    | Insert horizontal rule (`---`) at position   |

### Other Commands

| Command      | Description                    |
|--------------|--------------------------------|
| `DISCONNECT` | Disconnect client from server  |
| `DOC?`       | Print current document content |

## Response Codes

| Response                    | Description                             |
|-----------------------------|-----------------------------------------|
| `SUCCESS`                   | Command executed successfully           |
| `Reject UNAUTHORISED`       | Insufficient permissions                |
| `Reject INVALID_CURSOR_POS` | Cursor position out of bounds           |
| `Reject DELETED_POSITION`   | Position was deleted by earlier command |
| `Reject OUTDATED_VERSION`   | Command targets old document version    |

## Output

- **Persistent Storage:** Upon `QUIT`, the server flushes the current state to `doc.md`
- **Named Pipes:** FIFOs are created in the working directory using the format `FIFO_C2S_<pid>` and `FIFO_S2C_<pid>`

## Testing & Memory Safety

- Compile with `-Wall -Wextra` to ensure no warnings

### Memory Leak Detection

- Use Valgrind to check for memory leaks:
  ```bash
  valgrind --leak-check=full ./server 1000
  ```
- AddressSanitizer (ASAN) can be enabled for runtime checks:
  ```bash
  gcc -fsanitize=address -g source/server.c -o server
  ```

### Race Condition Testing

- Use ThreadSanitizer (TSAN) to detect data races:
  ```bash
  gcc -fsanitize=thread -g source/server.c -o server -lpthread
  ```
- Test with multiple concurrent clients to stress mutex synchronisation
- Use Helgrind for thread error detection:
  ```bash
  valgrind --tool=helgrind ./server 1000
  ```


## Architecture

### Communication Protocol

1. Client sends `SIGRTMIN` to server PID
2. Server spawns thread, creates FIFOs (`FIFO_C2S_<pid>`, `FIFO_S2C_<pid>`)
3. Server sends `SIGRTMIN+1` to client
4. Client opens FIFOs and sends username
5. Server validates user against `roles.txt`
6. Server sends role and initial document state
7. Bidirectional command/response communication begins

### Document Transmission Format

```
<role>\n
<version>\n
<length>\n
<document_content>
```

### Versioning System

- Version increments at each `TIME_INTERVAL`
- All edits target the current document version
- Commands referencing outdated versions are rejected
- Changes are broadcast to all clients with version updates

## Project Structure

```
.
├── source/
│   ├── server.c      # Server implementation
│   ├── client.c      # Client implementation
│   └── markdown.c    # Markdown document operations
├── libs/
│   ├── markdown.h    # Markdown function declarations
│   └── document.h    # Document structure definitions
├── roles.txt         # User permissions configuration
├── Makefile          # Build configuration
├── doc.md            # Document output (created on QUIT)
└── README.md
```

## Markdown Syntax Reference

| Element         | Syntax         |
|-----------------|----------------|
| Heading 1       | `# H1`         |
| Heading 2       | `## H2`        |
| Heading 3       | `### H3`       |
| Bold            | `**bold**`     |
| Italic          | `*italic*`     |
| Code            | `` `code` ``   |
| Blockquote      | `> quote`      |
| Ordered List    | `1. item`      |
| Unordered List  | `- item`       |
| Horizontal Rule | `---`          |
| Link            | `[title](url)` |

## Notes

- Block-level elements (headings, lists, blockquotes, horizontal rules) automatically insert preceding newlines if needed
- Horizontal rules also require a trailing newline
- Ordered lists automatically renumber when items are added or removed
- Cursor positions are 0-indexed (position 0 is before the first character)
