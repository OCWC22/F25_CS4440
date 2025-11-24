# CS4440 Project 3 – Secondary Storage and Filesystem Server

This directory contains a complete implementation of **all parts** of Project 3:

- **Reverse string client/server** (basic socket + threads).
- **Directory listing server** using `ls` via `fork` + `exec`.
- **Disk storage server** that simulates a physical disk over sockets.
- **Disk test clients** (interactive and random).
- **Filesystem server** implementing a simple FAT-based filesystem with a
  single-root hierarchy and subdirectories.
- **Filesystem client** for interactive testing of all filesystem commands.

The design is intentionally simple and heavily commented so it can be
understood by a grader, an onboarding engineer, or by you later.

---

## Executables and Source Files

- **reverse_server.c / reverse_server**  
  Multi-threaded TCP server for the *reverse string* part. Spawns one
  thread per client connection.

- **reverse_client.c / reverse_client**  
  Connects to `reverse_server`, sends a line, prints the reversed result.

- **ls_server.c / ls_server**  
  TCP server for the *directory listing* part. For each client request it
  `fork`s and uses `execvp("ls", ...)` to run `ls` with the exact
  arguments sent by the client. The child's stdout/stderr are redirected
  to the socket.

- **ls_client.c / ls_client**  
  Sends `ls` arguments to `ls_server` and streams back the results.

- **disk_server.c / disk_server**  
  Simulated disk server. Speaks the **disk protocol** from the spec:

  - `I` – returns `<cylinders> <sectors_per_cylinder>`  
  - `R c s` – returns `"1"` + 128 bytes for a valid block, or `"0"` on
    invalid `c`/`s`  
  - `W c s l data` – writes `l` bytes (0–128) to cylinder `c`, sector `s`.
    Returns `"1"` on success, `"0"` on invalid arguments.

  The disk is backed by a regular file (e.g., `disk.img`) using `mmap()`.
  Track-to-track delay is simulated with `usleep()` based on the
  difference in cylinder numbers.

- **disk_client_cli.c / disk_client_cli**  
  Command-line client for manual disk testing. Accepts commands in the
  same format as the disk protocol and prints responses.

- **disk_client_random.c / disk_client_random**  
  Random stress tester. Uses `I` to discover geometry, then issues `N`
  random `R`/`W` requests with 128-byte writes, showing progress with
  `R`/`W` characters printed to stdout.

- **fs_defs.h**  
  Shared **filesystem metadata definitions**:

  - `BLOCK_SIZE` (128 bytes).  
  - `DirEntry` structure (fixed 32 bytes, so exactly 4 entries per block).  
  - FAT markers: `FAT_FREE` and `FAT_EOF`.  
  - File types: `TYPE_FILE`, `TYPE_DIR`.

- **fs_server.c / fs_server**  
  Filesystem server that connects to `disk_server` over TCP and
  implements:

  - **F** – format filesystem (initializes FAT and root directory).
  - **C f** – create file `f`.
  - **D f** – delete file `f`.
  - **L b** – directory listing (`b = 0` names only, `b = 1` with sizes).
  - **R f** – read the entire file `f`.
  - **W f l data** – overwrite file `f` with `l` bytes of `data`.
  - **mkdir dirname** – create subdirectory under current directory.
  - **cd dirname** – change current directory (`/` resets to root).
  - **pwd** – print current working directory path.
  - **rmdir dirname** – remove an *empty* subdirectory.

  The implementation uses an in-memory FAT (loaded from disk) and
  directory blocks stored as arrays of `DirEntry` in disk blocks. All
  disk access goes through the disk protocol (`I/R/W`) using a single
  TCP connection protected by a `pthread_mutex_t`.

- **fs_client.c / fs_client**  
  Command-line client for the filesystem server. Lets you interactively
  issue all filesystem protocol commands and see the responses.

- **Makefile**  
  Builds all of the above programs when you run `make` inside
  `project_3/`.

---

## End-to-End System Overview

### 1. Disk Layer (`disk_server`)

- Organizes the simulated disk as **cylinders** × **sectors** with a
  fixed `BLOCK_SIZE` of 128 bytes.
- Stores data in a regular file via `mmap()` so that the in-memory view
  and on-disk file stay consistent.
- Exposes the **disk protocol** (I/R/W) over TCP.
- Simulates mechanical seek time using a simple track-to-track delay in
  microseconds.

### 2. Filesystem Layer (`fs_server`)

- On startup, `fs_server`:

  - Connects to `disk_server` (IP + port provided on the command line).
  - Issues `I` to learn the geometry (cylinders × sectors).
  - Computes the total number of blocks.
  - Calculates how many blocks are needed for the FAT and which block is
    used for the **root directory**.
  - Reads the FAT from disk into memory.

- The FAT is a simple array of `uint16_t` entries, one per block:

  - `FAT_FREE` means the block is not in use by any file or directory.
  - Any other value is the index of the **next block** in the file.
  - `FAT_EOF` marks the **last block** in a chain.

- Each directory is represented as a single block containing an array of
  up to 4 `DirEntry` records:

  - `name` – fixed-length filename or directory name.
  - `size` – file size in bytes (0 for empty files and directories).
  - `head_block` – starting block index for this file or subdirectory.  
  - `type` – `TYPE_FILE` or `TYPE_DIR`.  
  - `valid` – whether this slot is in use.

- The server uses `pthread_mutex_t` to protect:

  - **Disk I/O** (`disk_lock`) so the single disk connection is used
    safely from multiple threads.
  - **Filesystem metadata** (`fs_lock`) around FAT and directory
    modifications.

### 3. Directory Structure and Sessions

- Each client connection gets its own `Session`:

  - `sock` – the connected socket.
  - `current_dir` – block index of the current directory.
  - `pwd` – printable path string (e.g., `/`, `/sub`, `/sub/child`).

- On connection, a session starts at the **root directory** (`/`).

- `mkdir dirname`:

  - Validates name length and finds a free directory entry in the current
    directory.
  - Allocates a new free block from the FAT and marks it as `FAT_EOF`.
  - Zero-fills that block (empty directory).
  - Writes a `DirEntry` of type `TYPE_DIR` in the parent directory.

- `cd dirname`:

  - Special case `cd /` resets to the root directory.
  - Otherwise, scans the current directory block for a `TYPE_DIR` entry
    matching `dirname`.
  - If found, updates `current_dir` to that directory's `head_block` and
    appends the name onto `pwd`.

- `pwd` simply sends back the current `pwd` string.

- `rmdir dirname` only succeeds if the target is a directory and its
  own directory block has no valid entries.

### 4. File Operations

- **F (format)**:

  - Resets the entire FAT to `FAT_FREE` and then marks the FAT region
    plus the root directory block as `FAT_EOF` (reserved).
  - Zeroes the root directory block.

- **C f (create)**:

  - Fails if `f` already exists in the current directory.
  - Finds a free directory slot and a free data block.
  - Initializes the new `DirEntry` with size 0 and `head_block` pointing
    to the allocated block.

- **W f l data**:

  - Validates that `f` exists and is a **file**, not a directory.
  - Walks the current block chain for the file and frees all blocks after
    the head back to `FAT_FREE`.
  - Rewrites from the head block onward, allocating new blocks if
    necessary and chaining them through FAT.
  - Updates the stored file size and persists the updated FAT and
    directory block to disk.

- **R f**:

  - Validates that `f` exists and is a file.
  - Computes the total size from the directory entry.
  - Streams the file contents by walking the FAT chain, block by block.

- **D f**:

  - For files, frees the entire FAT chain back to `FAT_FREE` and marks
    the directory slot as invalid.

All these operations are serialized by `fs_lock` so that the FAT and
directories remain consistent even with multiple concurrent clients.

---

## How to Build

From the repository root:

```bash
cd project_3
make
```

This builds all executables listed in the `Makefile`.

---

## How to Run Each Part

### 1. Reverse String Server

```bash
cd project_3
./reverse_server 5000 &
./reverse_client 127.0.0.1 5000
```

Type a line in the client and you should see the reversed string.

### 2. Directory Listing Server

```bash
cd project_3
./ls_server 5001 &
./ls_client 127.0.0.1 5001 -l .
```

The client prints the output of `ls -l .` executed on the server host.

### 3. Disk Server + Clients

Start the disk server with desired geometry and track delay:

```bash
cd project_3
./disk_server 6000 10 8 1000 disk.img
# 10 cylinders, 8 sectors, 1000 microseconds per cylinder move
```

Interactive disk client:

```bash
./disk_client_cli 127.0.0.1 6000
# Examples inside the client:
#   I
#   W 0 0 hello-disk
#   R 0 0
```

Random tester client:

```bash
./disk_client_random 127.0.0.1 6000 1000 42
```

### 4. Filesystem Server + Client

First ensure the disk server is running (as shown above). Then run the
filesystem server, pointing it at the disk server:

```bash
cd project_3
./fs_server 7000 127.0.0.1 6000
# fs_server listens on port 7000 and talks to disk_server on 127.0.0.1:6000
```

In another terminal, start the filesystem client:

```bash
cd project_3
./fs_client 127.0.0.1 7000
```

Example session in `fs_client`:

```text
fs> F
0 Format Complete
fs> L 0
(Empty)
fs> mkdir docs
0
fs> cd docs
0
fs> pwd
/docs
fs> C notes
0 Created
fs> W notes 11 hello world
0 Written
fs> R notes
0 11 hello world
fs> L 1
notes	11 bytes	
fs> cd /
0
fs> L 1
docs	0 bytes	<DIR>
fs> rmdir docs
2 Not Empty
```

---

## Mapping to Assignment Requirements

- **Basic client-server (reverse string)**  
  Implemented by `reverse_server` (multi-threaded) and `reverse_client`.
  Each incoming connection is handled by a dedicated thread.

- **Directory listing server**  
  Implemented by `ls_server` and `ls_client`. Uses `fork` + `execvp` to
  run `ls` with client-specified arguments and streams output back.

- **Basic disk-storage system**  
  Implemented by `disk_server`, `disk_client_cli`, and
  `disk_client_random`:

  - Supports `I`, `R c s`, `W c s l data` exactly as specified.  
  - Enforces bounds on cylinder/sector and maximum length `l <= 128`.  
  - Simulates track-to-track delay with `usleep()`.

- **Filesystem server (flat filesystem + directory structure)**  
  Implemented by `fs_server` + `fs_client` + `fs_defs.h`:

  - Flat filesystem operations: `F`, `C`, `D`, `L`, `R`, `W`.  
  - Directory structure commands: `mkdir`, `cd`, `pwd`, `rmdir`.  
  - Uses a FAT and directory blocks stored on top of `disk_server`.

- **Robustness and Error Handling**  
  All system calls are checked for errors. On fatal errors, programs
  print a clear message with `perror()` and exit with a non-zero status.

---

## Notes for Graders and Future Maintainers

- The implementation is deliberately straightforward: no extra
  abstractions beyond what is needed to clearly map to the spec.
- All key data structures and flows are documented in `fs_defs.h` and in
  the comments in `fs_server.c` and `disk_server.c`.
- If you need to extend this project (e.g., multiple directory blocks,
  more metadata, or integrity checks), the current layout should make it
  easy to add new features without changing the basic protocol.
