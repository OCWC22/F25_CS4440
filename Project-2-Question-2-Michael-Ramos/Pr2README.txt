Pr2README — Project 2 (Problem 2 only)

Contents of ZIP/TAR
-------------------
- Makefile          : Build script for the project (targets: all, clean).
- mh.c              : Source code for "Mother Hubbard" (Problem 2) with internal documentation.
- run_tests.sh      : Test script shows correct operation and error handling.

Build & Run
-----------
Linux / POSIX with pthreads:
    make
    ./mh 1        # run for 1 day
    ./mh 2        # run for 2 days
    ./mh n 	  # run for n days

macOS note:
  Native macOS lacks unnamed POSIX semaphores (sem_init). Options:
    (a) Use Docker:
        docker run --rm -it -v "$PWD":/work -w /work gcc:14 bash -lc 'make && ./mh 1'
    (b) Use ./mh_macos.c

What the Program shows
-------------------------
- Simulates the Mother/Father two-thread scenario for N cycles (days).
- Mother performs, per child (1..12), in order: breakfast → school → dinner → bath.
- After each bath, the child is queued for Father; Father reads a book and tucks the child in bed.
- Proper blocking synchronization (no busy-wait); Mother does usleep(100) per action.
- Printed output includes day begin/end, wake/sleep handoffs, and per-child actions.
- Program exits cleanly after N cycles.

Code Meets Requirements
---------------------------------
- Two threads (Mother and Father) created via pthreads.
- Semaphore-guarded ring buffer carries per-child readiness from Mother to Father.
- Binary semaphore coordinates day boundaries (Mother starts awake; Father signals day complete).
- No busy-wait: threads block on semaphores.
- Internal documentation present throughout mh.c.

Observation Section (Problem 2)
-------------------------------
- Correctness: Father actions never occur before a child's bath, due to queue + semaphores.
- Concurrency: Output from both threads interleaves, but ordering constraints hold.
- Performance: usleep(100) intentionally slows prints; removing it speeds up runs.
- Robustness: Bad inputs (missing argument, non-positive N) are detected; program exits with usage message.
- Deadlock Avoidance: Separation of "work queue" (bathed children) from "day handoff" semaphore avoids circular waits.
- Starvation Avoidance: Queue is FIFO and size-limited to 12; every bathed child is eventually served by Father.

Test Runs
---------
Use the script:
    ./run_tests.sh

It shows:
  1) Correct run with N=1 and N=2 (first lines displayed for brevity).
  2) Error cases: missing argument and non-positive N.


Additional Notes (macOS & Expected Output)
-----------------------------------------
- If you see repeated "Child #0" lines or Father actions before the first bath, you are likely
  running the Linux variant on macOS where sem_init is not implemented. Build the macOS target:
    make macos
    ./mh_macos 1
- Valid child IDs are 1..12 only. Interleaving is expected, but ordering constraints hold:
  Father reads/tucks only after a child's bath, and "Father is waking up..." prints once per day.
