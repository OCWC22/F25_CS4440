# Task 011 – Project 3 Disk + Filesystem Implementation

- **Date:** 2025-11-24
- **Status:** Completed

## Files Updated / Created

- **CREATED** `project_3/Makefile`  
  - Single entry point to build all Project 3 binaries (reverse, ls,
    disk, and filesystem components).

- **UPDATED** `project_3/fs_server.c`  
  - Completed `handle_client` command dispatch, especially `cd` handling
    and per-session directory tracking.  
  - Added a proper `main()` that connects to `disk_server`, initializes
    FAT metadata, and runs a threaded TCP server for filesystem clients.  
  - Included `<netinet/in.h>` to ensure portable `sockaddr_in` usage.

- **CREATED** `project_3/fs_client.c`  
  - Interactive filesystem client supporting all required commands
    (F/C/D/L/R/W, mkdir, cd, pwd, rmdir).  
  - Special handling for `R` responses to correctly read a full file
    payload from the server.

- **CREATED** `project_3/README.md`  
  - High-level overview of all Project 3 components.  
  - End-to-end explanation of disk server, filesystem server, and
    clients.  
  - Concrete run instructions for every executable and explicit mapping
    to the assignment requirements.

## Summary

Project 3 is now **end-to-end complete**:

- Reverse string client/server
- Directory listing server/client
- Disk server with interactive and random clients
- Filesystem server with a simple FAT-based layout and directory support
- Filesystem client for interactive testing
- A dedicated `Makefile` and README for Project 3

All components compile together and are wired up through the documented
socket protocols.

## Reasoning & Trade-offs

- **Filesystem design:**  
  Chose a classic FAT-style layout with one `uint16_t` entry per block
  and fixed-size directory entries. This matches the spec suggestions and
  keeps on-disk structures simple enough to reason about without extra
  tooling.

- **Concurrency model:**  
  - Disk server: single-threaded, one connection at a time, but supports
    multiple requests per connection.  
  - Filesystem server: multi-threaded (one thread per client) with
    coarse-grained `pthread_mutex_t` guards for disk I/O and filesystem
    metadata. This is simpler and more robust than fine-grained locking
    for a student project.

- **Protocol handling:**  
  For disk and filesystem operations, the code favors simple, readable
  protocol parsing over extreme optimization. Where results can be
  arbitrarily large (filesystem `R`), the client explicitly parses the
  header and then reads exactly the advertised payload.

- **Scope discipline:**  
  Only functionality explicitly required in the assignment is
  implemented. No extra features (permissions, integrity checks, or
  multi-block directories) were added to avoid overcomplicating grading
  and maintenance.

## Issues Encountered

- The original `fs_server.c` ended mid-line in the `cd` handler and was
  missing a `main()` function, so the server would not link or run.
  Fixed by reconstructing the intended session semantics and adding a
  standard TCP accept loop.

- Needed to ensure that filesystem client/server handling of `R` was
  robust against partial network reads. Implemented a small header parser
  and `recv_all()` helper in `fs_client.c`.

- There was no `Makefile` in `project_3`, which would have broken the
  grading requirement that `make` compile all artifacts. Added a
  minimal, conventional Makefile consistent with Project 1.

## Future Work (Optional)

- Extend directory support to multiple blocks per directory and add a
  notion of `..` (parent) for `cd` semantics.  
- Add a small scripted test harness per program to capture example runs
  (as `.sh` or `.txt` transcripts) mirroring the assignment submission
  expectations.  
- Add basic filesystem statistics commands (free space, number of files,
  fragmentation estimate) for additional insight during testing.
