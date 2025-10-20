# Complete CPU & Operating Systems Visualization Guide

## Table of Contents
1. [CPU Architecture Overview](#cpu-architecture-overview)
2. [Process Management & Scheduling](#process-management--scheduling)
3. [Memory Management & Virtualization](#memory-management--virtualization)
4. [Synchronization Primitives](#synchronization-primitives)
5. [Banker's Algorithm CPU Execution](#bankers-algorithm-cpu-execution)
6. [Deadlock Prevention Hardware View](#deadlock-prevention-hardware-view)
7. [Complete System Interaction Diagram](#complete-system-interaction-diagram)

## 1. CPU Architecture Overview

### Modern Multi-Core CPU Layout
```
┌─────────────────────────────────────────────────────────────────┐
│                    INTEL XEON SAPPHIRE RAPIDS                    │
│                     (Up to 60 Cores, 120 Threads)                │
├─────────────────────────────────────────────────────────────────┤
│                         L3 CACHE (Shared)                        │
│                     60-105 MB per Socket                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐  │
│  │    CORE 0   │ │    CORE 1   │ │    CORE 2   │ │    CORE N   │  │
│  │  (2 Threads)│ │  (2 Threads)│ │  (2 Threads)│ │  (2 Threads)│  │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘  │
│   │ L1I │ L1D │   │ L1I │ L1D │   │ L1I │ L1D │   │ L1I │ L1D │   │
│   │48KB │32KB │   │48KB │32KB │   │48KB │32KB │   │48KB │32KB │   │
│   └─────┴─────┘   └─────┴─────┘   └─────┴─────┘   └─────┴─────┘   │
│        └─────────┴─ L2 CACHE (2MB per core) ─────────┘        │
└─────────────────────────────────────────────────────────────────┘
```

### CPU Core Execution Pipeline
```
┌─────────────────────────────────────────────────────────────────┐
│                    SINGLE CORE EXECUTION UNIT                    │
├─────────────────────────────────────────────────────────────────┤
│  Frontend: Instruction Fetch & Decode                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐  │
│  │  I-CACHE    │ │Instruction  │ │  Branch     │ │Instruction  │  │
│  │  (L1I)      │ │   Queue     │ │ Predictor   │ │  Decoder    │  │
│  │   48KB      │ │             │ │             │ │             │  │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘  │
│        │              │              │              │           │
│        ▼              ▼              ▼              ▼           │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │               OUT-OF-ORDER EXECUTION ENGINE                │ │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌────────┐ │ │
│  │  │  Reorder │ │ Register│ │  Issue  │ │Execution│ │  ROB   │ │ │
│  │  │  Buffer  │ │ Rename  │ │ Queue   │ │ Units   │ │ (192)  │ │ │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └────────┘ │ │
│  │     │           │           │           │          │        │ │
│  │     ▼           ▼           ▼           ▼          ▼        │ │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌────────┐ │ │
│  │  │   ALU   │ │  FPU    │ │  Load   │ │  Store  │ │  Branch│ │ │
│  │  │ (4 units)│ │ (2 units)│ │ (2 units)│ │ (2 units)│ │  Unit  │ │ │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └────────┘ │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                 │                               │
│                                 ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                  MEMORY SUBSYSTEM                          │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌────────┐ │ │
│  │  │   L1D       │ │     L2      │ │     L3      │ │Memory │ │ │
│  │  │   Cache     │ │    Cache    │ │    Cache    │ │Controller│ │ │
│  │  │   32KB      │ │    2MB      │ │   Shared    │ │        │ │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └────────┘ │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 2. Process Management & Scheduling

### Process State Transitions (CPU View)
```
┌─────────────────────────────────────────────────────────────────┐
│                    LINUX PROCESS STATES                         │
│                                                                 │
│  ┌─────────────┐        fork()        ┌─────────────┐          │
│  │             │ ───────────────────► │             │          │
│  │   READY     │                      │   RUNNING   │ ◄─────────┤
│  │  (Runnable) │ ◄─────────────────── │ (On CPU)    │ schedule()│
│  │             │    time_slice_expired│             │          │
│  └─────────────┘                      └─────────────┘          │
│        │                                      │               │
│        │ sleep()/wait()                       │               │
│        ▼                                      │               │
│  ┌─────────────┐                               │               │
│  │  SLEEPING   │                               │               │
│  │  (Blocked)  │ ◄─────────────────────────────┘               │
│  │             │    I/O complete or signal received              │
│  └─────────────┘                                               │
│        │                                                       │
│        │ process_exit()                                         │
│        ▼                                                       │
│  ┌─────────────┐                                               │
│  │   ZOMBIE    │                                               │
│  │ (Terminated)│                                               │
│  └─────────────┘                                               │
└─────────────────────────────────────────────────────────────────┘
```

### Linux CFS Scheduler (Completely Fair Scheduler)
```
┌─────────────────────────────────────────────────────────────────┐
│                      CFS RUNQUEUE                              │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │           RED-BLACK TREE (ordered by vruntime)             │ │
│  │                                                             │ │
│  │             ┌───── P1 (vruntime=10) ─────┐                 │ │
│  │             │                           │                 │ │
│  │      ┌───── P0 (vruntime=5) ──────┐    │    P2 (vruntime=15) │ │
│  │      │                           │    │    (next to run)    │ │
│  │   P3 (vruntime=3)              P4 (vruntime=8)           │ │
│  │   (leftmost = min vruntime)    │                        │ │
│  │                               │                        │ │
│  │                             P5 (vruntime=12)            │ │
│  │                                                             │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│  Current Process: P1 (on CPU Core 2)                           │
│  Time Slice Remaining: 3ms                                     │
│  next_timer_interrupt: 3ms                                     │
└─────────────────────────────────────────────────────────────────┘

```

### Context Switch Hardware Flow
```
┌─────────────────────────────────────────────────────────────────┐
│                    CONTEXT SWITCH SEQUENCE                      │
│                                                                 │
│  TIMER INTERRUPT → CPU SWITCHES TO KERNEL MODE                  │
│                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐  │
│  │   SAVE CURRENT  │    │  SELECT NEXT    │    │ RESTORE NEXT│  │
│  │   PROCESS STATE │    │   PROCESS       │    │ PROCESS STATE│  │
│  │                 │    │                 │    │             │  │
│  │ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────┐ │  │
│  │ │   CPU       │ │    │ │   Scheduler │ │    │ │   CPU   │ │  │
│  │ │ Registers   │ │    │ │   Logic     │ │    │ │Registers│ │  │
│  │ │             │ │    │ │             │ │    │ │         │ │  │
│  │ │ RAX: 0x1234 │ │    │ │ Pick P2 from│ │    │ │ RAX:...  │ │  │
│  │ │ RBX: 0xABCD │ │    │ │ runqueue    │ │    │ │ RBX:...  │ │  │
│  │ │ RSP: 0x7FFF  │ │    │ │             │ │    │ │ RSP:...  │ │  │
│  │ │ RIP: 0x4000  │ │    │ │ Load P2's   │ │    │ │ RIP:...  │ │  │
│  │ │             │ │    │ │ task_struct  │ │    │ │         │ │  │
│  │ └─────────────┘ │    │ └─────────────┘ │    │ └─────────┘ │  │
│  │ ┌─────────────┐ │    │                 │    │ ┌─────────┐ │  │
│  │ │   MMU/TLB   │ │    │                 │    │ │   MMU/  │ │  │
│  │ │   State     │ │    │                 │    │ │   TLB   │ │  │
│  │ │ CR3: 0x1000 │ │    │                 │    │ │ CR3:... │ │  │
│  │ │ Flush TLB   │ │    │                 │    │ │ Flush   │ │  │
│  │ └─────────────┘ │    │                 │    │ └─────────┘ │  │
│  └─────────────────┘    └─────────────────┘    └─────────────┘  │
│           │                       │                       │    │
│           └───────────────────────┼───────────────────────┘    │
│                                   ▼                           │
│                        SWITCH TO USER MODE                    │
│                        (IRET instruction)                    │
└─────────────────────────────────────────────────────────────────┘
```

## 3. Memory Management & Virtualization

### Virtual Memory Translation
```
┌─────────────────────────────────────────────────────────────────┐
│                    VIRTUAL MEMORY TRANSLATION                   │
│                                                                 │
│  VIRTUAL ADDRESS (48-bit)                                       │
│  ┌─────────────┬─────────────┬─────────────┬─────────────────────┐ │
│  │   PML4      │  PDPT (512) │  PD (512)   │   PAGE (4KB)        │ │
│  │   (9 bits)  │  (9 bits)   │  (9 bits)   │    (12 bits)        │ │
│  └─────────────┴─────────────┴─────────────┴─────────────────────┘ │
│           │              │              │              │         │
│           ▼              ▼              ▼              ▼         │
│  ┌─────────────┐ ┌─────────────────┐ ┌─────────────┐ ┌─────────┐ │
│  │  PML4 Table │ │  PDPT Table     │ │  PD Table   │ │ Physical│ │
│  │  (in CR3)   │ │                 │ │             │ │  Page  │ │
│  │             │ │                 │ │             │ │ Frame  │ │
│  │ Entry[0] ──┼─► Entry[3] ───────┼─► Entry[7] ───┼─► Frame  │ │
│  │ (points to  │ │ (points to PD   │ │ (points to  │ │ #42     │ │
│  │  PDPT)      │ │  table)         │ │  page)      │ │         │ │
│  └─────────────┘ └─────────────────┘ └─────────────┘ └─────────┘ │
│                                                                 │
│  TLB (Translation Lookaside Buffer) - CACHE                     │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │   VPN → PPN Mappings (Recently Used)                        │ │
│  │   0x1234000 → 0x0042000 (Hit!)                              │ │
│  │   0x5678000 → 0x00A5000 (Hit!)                              │ │
│  │   ... 64 entries total (L1 TLB)                             │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Memory Hierarchy & Cache Coherency (MESI)
```
┌─────────────────────────────────────────────────────────────────┐
│                    MULTI-CORE CACHE COHERENCY                   │
│                                                                 │
│  CORE 0                         CORE 1                         CORE 2│
│  ┌─────────────┐               ┌─────────────┐           ┌─────────┐│
│  │    L1D      │◄──────────────►│    L1D      │◄──────────►│  L1D    ││
│  │   Cache     │  MESI Protocol │   Cache     │  Shared Bus │  Cache  ││
│  │   (32KB)    │               │   (32KB)    │           │ (32KB)  ││
│  └─────────────┘               └─────────────┘           └─────────┘│
│         │                               │                        ││
│         ▼                               ▼                        ││
│  ┌─────────────┐               ┌─────────────┐                    ││
│  │     L2      │◄──────────────►│     L2      │                    ││
│  │   Cache     │               │   Cache     │                    ││
│  │   (2MB)     │               │   (2MB)     │                    ││
│  └─────────────┘               └─────────────┘                    ││
│         │                               │                        ││
│         └───────────────────────────────┼────────────────────────┘│
│                                         ▼                        ││
│  ┌─────────────────────────────────────────────────────────────┐ ││
│  │                     L3 CACHE (SHARED)                      │ ││
│  │                   60-105 MB Total                          │ ││
│  │                                                             │ ││
│  │  MESI States:                                               │ ││
│  │  M (Modified): This core has exclusive, dirty copy          │ ││
│  │  E (Exclusive): This core has exclusive, clean copy        │ ││
│  │  S (Shared): Multiple cores have clean copies              │ ││
│  │  I (Invalid): Cache line is invalid                        │ ││
│  └─────────────────────────────────────────────────────────────┘ ││
└─────────────────────────────────────────────────────────────────┘
```

## 4. Synchronization Primitives

### Mutex Hardware Implementation
```
┌─────────────────────────────────────────────────────────────────┐
│              PTHREAD_MUTEX HARDWARE EXECUTION                   │
│                                                                 │
│  THREAD A (trying to lock)                 THREAD B (unlocking) │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  UNLOCKED MUTEX (state = 0)                                 │ │
│  │  Cache Line State: SHARED (all cores have same value)      │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  THREAD A: LOCK CMPXCHG [mutex_addr], thread_id             │ │
│  │                                                             │ │
│  │  CPU EXECUTION:                                            │ │
│  │  1. Load mutex value (0)                                    │ │
│  │  2. Compare with expected (0)                               │ │
│  │  3. If equal: store thread_id atomically                    │ │
│  │  4. LOCK prefix:                                           │ │
│  │     - Asserts bus lock                                      │ │
│  │     - Cache line transitions to MODIFIED state              │ │
│  │     - Other cores' copies invalidated (MESI I state)       │ │
│  │  5. Return result (success)                                 │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  LOCKED MUTEX (state = thread_id_A)                         │ │
│  │  Cache Line State: MODIFIED (only Core A has current value) │ │
│  │  Other cores: INVALID (must reload to access)              │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  THREAD B: TRY TO LOCK (contended case)                     │ │
│  │                                                             │ │
│  │  1. CMPXCHG fails (value != 0)                              │ │
│  │  2. Cache line in Core B is INVALID → reload from Core A    │ │
│  │  3. Still fails → spin briefly (adaptive mutex)             │ │
│  │  4. Call futex(FUTEX_LOCK_PI) system call                  │ │
│  │     - Switch to kernel mode                                  │ │
│  │     - Add Thread B to mutex wait queue                       │ │
│  │     - Put Thread B to sleep (TASK_INTERRUPTIBLE)            │ │
│  │     - Context switch away                                   │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  THREAD A: UNLOCK                                           │ │
│  │                                                             │ │
│  │  1. Store 0 to mutex address (atomic XCHG)                  │ │
│  │  2. Memory barrier ensures all prior stores visible          │ │
│  │  3. Cache line transitions to SHARED state                  │ │
│  │  4. Check futex wait queue → Thread B is waiting           │ │
│  │  5. Call futex(FUTEX_UNLOCK_PI)                             │ │
│  │     - Wake Thread B                                         │ │
│  │     - Move Thread B to runnable queue                        │ │
│  │     - Scheduler may immediately schedule Thread B           │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Semaphore Hardware Implementation
```
┌─────────────────────────────────────────────────────────────────┐
│                SEMAPHORE ATOMIC OPERATIONS                      │
│                                                                 │
│  SEMAPHORE COUNT (stored in shared memory)                      │
│  Initial Value: BUFFER_SIZE = 10                                │
│                                                                 │
│  PRODUCER THREAD (sem_wait):               CONSUMER THREAD (sem_wait)│
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  LOCK DEC [semaphore_addr]  ← ATOMIC DECREMENT              │ │
│  │                                                             │ │
│  │  CPU HARDWARE FLOW:                                        │ │
│  │  1. Load semaphore value into register                      │ │
│  │  2. Decrement register value                                │ │
│  │  3. LOCK prefix:                                           │ │
│  │     - Locks memory bus                                      │ │
│  │     - Prevents other cores from accessing                   │ │
│  │     - Ensures atomic operation                              │ │
│  │  4. Store result back to memory                             │ │
│  │  5. Return old value                                        │ │
│  │                                                             │ │
│  │  RESULT: old_value = 5, new_value = 4                       │ │
│  │  Since old_value > 0: continue (fast path)                  │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  CONSUMER THREAD (sem_wait on full semaphore)               │ │
│  │                                                             │ │
│  │  LOCK DEC [full_semaphore_addr]                            │ │
│  │  RESULT: old_value = 0, new_value = -1                      │ │
│  │                                                             │ │
│  │  Since old_value <= 0: BLOCK (slow path)                    │ │
│  │  1. Call futex(FUTEX_WAIT) system call                     │ │
│  │  2. Kernel puts thread to sleep on semaphore                │ │
│  │  3. Context switch away                                      │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  PRODUCER THREAD (sem_post on full semaphore)               │ │
│  │                                                             │ │
│  │  LOCK INC [full_semaphore_addr]                            │ │
│  │  RESULT: new_value = 1                                      │ │
│  │                                                             │ │
│  │  Check wait queue → Consumer thread is waiting              │ │
│  │  1. Call futex(FUTEX_WAKE) system call                      │ │
│  │  2. Kernel wakes consumer thread                             │ │
│  │  3. Consumer becomes runnable                               │ │
│  │  4. Scheduler may immediately run consumer                  │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 5. Banker's Algorithm CPU Execution

### Banker's Algorithm System State Visualization
```
┌─────────────────────────────────────────────────────────────────┐
│                BANKER'S ALGORITHM SYSTEM STATE                  │
│                                                                 │
│  SYSTEM RESOURCES: R1=7, R2=5, R3=3                             │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                      ALLOCATION MATRIX                      │ │
│  │  Process │  R1  │  R2  │  R3  │    Current Usage            │ │
│  │  ────────┼──────┼──────┼──────┼────────────────────────────  │ │
│  │    P0    │  0   │  1   │  0   │    ████░░░░░░░░░░░░░░░        │ │
│  │    P1    │  2   │  0   │  0   │    ████████░░░░░░░░░░        │ │
│  │    P2    │  3   │  0   │  2   │    █████████████░░░░        │ │
│  │    P3    │  2   │  1   │  1   │    ████████████░░░░░        │ │
│  │    P4    │  0   │  0   │  2   │    ████░░░░░░░░░░░░░░░        │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                        MAX MATRIX                           │ │
│  │  Process │  R1  │  R2  │  R3  │    Maximum Claims           │ │
│  │  ────────┼──────┼──────┼──────┼────────────────────────────  │ │
│  │    P0    │  7   │  5   │  3   │    ████████████████░░        │ │
│  │    P1    │  3   │  2   │  2   │    ████████░░░░░░░░░░        │ │
│  │    P2    │  9   │  0   │  2   │    █████████████░░░░        │ │
│  │    P3    │  2   │  2   │  2   │    ████████░░░░░░░░░░        │ │
│  │    P4    │  4   │  3   │  3   │    ██████████░░░░░░░        │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                        NEED MATRIX                          │ │
│  │  Process │  R1  │  R2  │  R3  │    Still Needed             │ │
│  │  ────────┼──────┼──────┼──────┼────────────────────────────  │ │
│  │    P0    │  7   │  4   │  3   │    ████████████░░░░░        │ │
│  │    P1    │  1   │  2   │  2   │    ██████░░░░░░░░░░░        │ │
│  │    P2    │  6   │  0   │  0   │    ██████░░░░░░░░░░░        │ │
│  │    P3    │  0   │  1   │  1   │    ██░░░░░░░░░░░░░░░░        │ │
│  │    P4    │  4   │  3   │  1   │    ████████░░░░░░░░░        │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│  AVAILABLE: (R1=3, R2=3, R3=0)                                   │
│  [███████░░░] [███████░░░] [░░░░░░░░░░░]                            │
└─────────────────────────────────────────────────────────────────┘
```

### Safety Algorithm CPU Execution Flow
```
┌─────────────────────────────────────────────────────────────────┐
│                SAFETY ALGORITHM CPU EXECUTION                   │
│                                                                 │
│  CPU CORE 0 (Banker's Algorithm Execution)                      │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  INSTRUCTION FETCH & DECODE                                 │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │    RAX      │ │    RBX      │ │    RCX      │           │ │
│  │  │ Work[0]=3   │ │ Work[1]=3   │ │ Work[2]=0   │           │ │
│  │  │ (R1 avail)  │ │ (R2 avail)  │ │ (R3 avail)  │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  LOOP: FOR each process i = 0 to 4                         │ │
│  │                                                             │ │
│  │  PROCESS P0 CHECK:                                          │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │ Need[0][0]=7 │ │ Need[0][1]=4 │ │ Need[0][2]=3 │           │ │
│  │  │ Work[0]=3   │ │ Work[1]=3   │ │ Work[2]=0   │           │ │
│  │  │ 7 ≤ 3? NO   │ │ 4 ≤ 3? NO   │ │ 3 ≤ 0? NO   │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │  RESULT: P0 CANNOT RUN                                      │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  PROCESS P1 CHECK:                                          │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │ Need[1][0]=1 │ │ Need[1][1]=2 │ │ Need[1][2]=2 │           │ │
│  │  │ Work[0]=3   │ │ Work[1]=3   │ │ Work[2]=0   │           │ │
│  │  │ 1 ≤ 3? YES  │ │ 2 ≤ 3? YES  │ │ 2 ≤ 0? NO   │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │  RESULT: P1 CANNOT RUN (R3 needed but unavailable)          │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  PROCESS P3 CHECK:                                          │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │ Need[3][0]=0 │ │ Need[3][1]=1 │ │ Need[3][2]=1 │           │ │
│  │  │ Work[0]=3   │ │ Work[1]=3   │ │ Work[2]=0   │           │ │
│  │  │ 0 ≤ 3? YES  │ │ 1 ≤ 3? YES  │ │ 1 ≤ 0? NO   │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │  RESULT: P3 CANNOT RUN (R3 needed but unavailable)          │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  ALL PROCESSES CHECKED → NONE CAN RUN                       │ │
│  │  BRANCH PREDICTOR: TAKEN (unsafe_state_detected)           │ │
│  │                                                             │ │
│  │  CPU EXECUTION:                                            │ │
│  │  MOV RAX, 0          ; Return unsafe                        │ │
│  │  RET                  ; Return to caller                    │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Resource Request Algorithm Hardware Flow
```
┌─────────────────────────────────────────────────────────────────┐
│              RESOURCE REQUEST ALGORITHM FLOW                    │
│                                                                 │
│  SCENARIO: P1 requests (1,0,0)                                  │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  STEP 1: CHECK REQUEST ≤ NEED                               │ │
│  │                                                             │ │
│  │  CPU REGISTERS:                                            │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │    RAX      │ │    RBX      │ │    RCX      │           │ │
│  │  │Request[0]=1 │ │Request[1]=0 │ │Request[2]=0 │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  MEMORY ACCESS:                                             │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │ Need[1][0]=1 │ │ Need[1][1]=2 │ │ Need[1][2]=2 │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  COMPARISON:                                                │ │
│  │  CMP RAX, [Need+1*3+0]  ; 1 ≤ 1? YES                       │ │
│  │  CMP RBX, [Need+1*3+1]  ; 0 ≤ 2? YES                       │ │
│  │  CMP RCX, [Need+1*3+2]  ; 0 ≤ 2? YES                       │ │
│  │  RESULT: REQUEST VALID                                      │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  STEP 2: CHECK REQUEST ≤ AVAILABLE                           │ │
│  │                                                             │ │
│  │  MEMORY ACCESS (Available vector):                          │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │Available[0]=3│ │Available[1]=3│ │Available[2]=0│           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  COMPARISON:                                                │ │
│  │  CMP RAX, [Available+0]  ; 1 ≤ 3? YES                       │ │
│  │  CMP RBX, [Available+1]  ; 0 ≤ 3? YES                       │ │
│  │  CMP RCX, [Available+2]  ; 0 ≤ 0? YES                       │ │
│  │  RESULT: RESOURCES AVAILABLE                                │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  STEP 3: TENTATIVELY ALLOCATE                              │ │
│  │                                                             │ │
│  │  ATOMIC OPERATIONS (LOCK prefix):                          │ │
│  │  LOCK SUB [Available+0], RAX   ; Available[0] = 3-1 = 2    │ │
│  │  LOCK ADD [Alloc+1*3+0], RAX   ; Allocation[1][0] = 2+1 = 3 │ │
│  │  LOCK SUB [Need+1*3+0], RAX    ; Need[1][0] = 1-1 = 0      │ │
│  │                                                             │ │
│  │  CACHE COHERENCY:                                          │ │
│  │  - Cache lines modified (MESI M state)                     │ │
│  │  - Other cores invalidated                                 │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  STEP 4: RUN SAFETY ALGORITHM                              │ │
│  │                                                             │ │
│  │  CALL safety_algorithm                                      │ │
│  │  - Push return address to stack                             │ │
│  │  - Jump to safety algorithm                                │ │
│  │                                                             │ │
│  │  (Safety algorithm runs - returns UNSAFE)                  │ │
│  │                                                             │ │
│  │  RESULT: NEW STATE IS UNSAFE                               │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  STEP 5: ROLLBACK (UNSAFE)                                 │ │
│  │                                                             │ │
│  │  ATOMIC OPERATIONS (undo allocation):                      │ │
│  │  LOCK ADD [Available+0], RAX   ; Available[0] = 2+1 = 3    │ │
│  │  LOCK SUB [Alloc+1*3+0], RAX   ; Allocation[1][0] = 3-1 = 2 │ │
│  │  LOCK ADD [Need+1*3+0], RAX    ; Need[1][0] = 0+1 = 1      │ │
│  │                                                             │ │
│  │  FINAL RESULT: REQUEST DENIED                               │ │
│  │  PROCESS P1 MUST WAIT                                      │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 6. Deadlock Prevention Hardware View

### Deadlock Detection CPU Analysis
```
┌─────────────────────────────────────────────────────────────────┐
│                DEADLOCK DETECTION HARDWARE ANALYSIS              │
│                                                                 │
│  WAIT-FOR GRAPH (CPU Data Structure):                           │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Process → Resource Mapping:                                │ │
│  │  P0 → R1 (waiting for R1)                                  │ │
│  │  R1 → P2 (held by P2)                                      │ │
│  │  P2 → R3 (waiting for R3)                                  │ │
│  │  R3 → P0 (held by P0) ← CYCLE DETECTED!                    │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│  CPU ALGORITHM FOR CYCLE DETECTION:                            │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  DEPTH-FIRST SEARCH (DFS) IMPLEMENTATION:                   │ │
│  │                                                             │ │
│  │  REGISTERS:                                                │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │    RAX      │ │    RBX      │ │    RCX      │           │ │
│  │  │ current_node│ │   visited   │ │ recursion  │           │ │
│  │  │   = P0      │ │   bitmap    │ │   stack     │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  ALGORITHM EXECUTION:                                      │ │
│  │  1. MARK P0 as visited                                     │ │
│  │  2. FOR each edge from P0:                                 │ │
│  │     - Edge P0→R1: recurse on R1                            │ │
│  │     - MARK R1 as visited                                   │ │
│  │     - Edge R1→P2: recurse on P2                            │ │
│  │     - MARK P2 as visited                                   │ │
│  │     - Edge P2→R3: recurse on R3                            │ │
│  │     - MARK R3 as visited                                   │ │
│  │     - Edge R3→P0: P0 already visited → CYCLE!             │ │
│  │                                                             │ │
│  │  BRANCH PREDICTOR:                                         │ │
│  │  - Predicts no cycle (normal case)                         │ │
│  │  - Misprediction → pipeline flush (~15-20 cycles)          │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  DEADLOCK RECOVERY ACTIONS                                  │ │
│  │                                                             │ │
│  │  KERNEL SPACE EXECUTION:                                    │ │
│  │  1. Identify victim process (lowest priority)               │ │
│  │  2. Send SIGKILL to victim                                 │ │
│  │  3. Force release of held resources                         │ │
│  │  4. Wake up waiting processes                               │ │
│  │                                                             │ │
│  │  CPU ACTIONS:                                               │ │
│  │  - Context switch to victim process                         │ │
│  │  - Deliver signal, terminate process                        │ │
│  │  - Update resource allocation tables                        │ │
│  │  - Reschedule blocked processes                             │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Prevention vs. Avoidance vs. Detection
```
┌─────────────────────────────────────────────────────────────────┐
│              DEADLOCK HANDLING STRATEGIES COMPARISON            │
│                                                                 │
│  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐    │
│  │   PREVENTION    │ │    AVOIDANCE    │ │    DETECTION    │    │
│  │                 │ │                 │ │                 │    │
│  │ Break one of    │ │ Banker's        │ │ Allow deadlock  │    │
│  │ the four        │ │ Algorithm       │ │ to occur, then  │    │
│  │ conditions:     │ │ (runtime check) │ │ detect & recover│    │
│  │                 │ │                 │ │                 │    │
│  │ • No mutual     │ │ Requires:       │ │ Requires:       │    │
│  │   exclusion     │ │ • Max claims    │ │ • Detection     │    │
│  │ • Hold & wait   │ │ • Safety check  │ │   algorithm     │    │
│  │ • No preemption │ │ • Resource      │ │ • Recovery      │    │
│  │ • Circular wait │ │   allocation    │ │   strategy      │    │
│  │                 │ │                 │ │                 │    │
│  │ CPU OVERHEAD:   │ │ CPU OVERHEAD:   │ │ CPU OVERHEAD:   │    │
│  │ Low (compile    │ │ Medium (runtime │ │ High (detection │    │
│  │ time/design)   │ │ safety checks)  │ │ + recovery)     │    │
│  │                 │ │                 │ │                 │    │
│  │ RESOURCE UTIL:  │ │ RESOURCE UTIL:  │ │ RESOURCE UTIL:  │    │
│  │ Low (conservative│ │ Medium (better │ │ High (optimal   │    │
│  │ allocation)     │ │ utilization)   │ │ utilization)   │    │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘    │
│                                                                 │
│  MODERN SYSTEMS:                                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  DATABASE SYSTEMS → Detection & Recovery                   │ │
│  │  • Deadlock detection runs periodically                    │ │
│  │  • Victim selection based on cost/rollback complexity      │ │
│  │                                                             │ │
│  │  REAL-TIME SYSTEMS → Prevention                           │ │
│  │  • Static resource allocation                              │ │
│  │  • Priority inheritance protocols                           │ │
│  │                                                             │ │
│  │  GENERAL PURPOSE OS → Avoidance                           │ │
│  │  • Banker's algorithm for some resources                   │ │
│  │  • Combination of strategies                               │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 7. Complete System Interaction Diagram

### Full OS/Hardware Interaction Stack
```
┌─────────────────────────────────────────────────────────────────┐
│                    COMPLETE SYSTEM ARCHITECTURE                 │
│                                                                 │
│  USER SPACE                                                    │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  APPLICATION PROCESS (Banker's Algorithm)                  │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │   main()    │ │request_res()│ │safety_check()│           │ │
│  │  │             │ │             │ │             │           │ │
│  │  │ Variables:  │ │ Request[0]=1│ │ Work[0]=3   │           │ │
│  │  │ Need[3][3]  │ │ Request[1]=0│ │ Work[1]=3   │           │ │
│  │  │ Alloc[3][3] │ │ Request[2]=0│ │ Work[2]=0   │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  SYSTEM CALL INTERFACE (SYSCALL)                            │ │
│  │                                                             │ │
│  │  USER MODE → KERNEL MODE TRANSITION:                        │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │   SYSCALL   │ │   SYSCALL   │ │   SYSCALL   │           │ │
│  │  │    TABLE    │ │    ENTRY    │ │    HANDLER  │           │ │
│  │  │             │ │             │ │             │           │ │
│  │  │ malloc() →  │ │ sched_      │ │ mutex_lock()│           │ │
│  │ │ brk()       │ │ yield()     │ │ futex()     │           │ │
│  │ └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  CPU STATE CHANGE:                                          │ │
│  │  - Save user registers to kernel stack                      │ │
│  │  - Load kernel registers (GS, FS segments)                  │ │
│  │  - Switch to kernel page tables (CR3)                       │ │
│  │  - Privilege level: 3 → 0                                   │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  KERNEL SPACE                                                  │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  LINUX KERNEL SUBSYSTEMS                                    │ │
│  │                                                             │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │   MEMORY    │ │  PROCESS    │ │   I/O &     │           │ │
│  │  │  MANAGER    │ │ SCHEDULER   │ │ FILESYSTEM  │           │ │
│  │  │             │ │             │ │             │           │ │
│  │  │ • Page      │ │ • CFS       │ │ • VFS       │           │ │
│  │  │   alloc     │ │ • Runqueue  │ │ • Ext4      │           │ │
│  │  │ • SLAB      │ │ • Context   │ │ • Block     │           │ │
│  │  │ • VM        │ │   switch    │ │   layer     │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │ SYNCHRONI-  │ │ INTERRUPT   │ │   NETWORK   │           │ │
│  │  │ ZATION     │ │  HANDLER    │ │   STACK     │           │ │
│  │  │             │ │             │ │             │           │ │
│  │  │ • Futex     │ │ • Timer     │ │ • TCP/IP    │           │ │
│  │  │ • Mutex     │ │ • I/O       │ │ • Sockets   │           │ │
│  │  │ • Semaphore │ │ • IRQ       │ │ • Netfilter │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  HARDWARE ABSTRACTION LAYER (HAL)                          │ │
│  │                                                             │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │   DEVICE    │ │   MEMORY    │ │   TIMER     │           │ │
│  │  │   DRIVERS   │ │ MANAGEMENT  │ │ MANAGEMENT  │           │ │
│  │  │             │ │             │ │             │           │ │
│  │  │ • PCI/PCIe  │ │ • NUMA      │ │ • HPET      │           │ │
│  │  │ • USB       │ │ • DMA       │ │ • TSC       │           │ │
│  │  │ • SATA/NVMe │ │ • IOMMU     │ │ • LAPIC     │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                    CPU HARDWARE                             │ │
│  │                                                             │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │   CORE 0    │ │   CORE 1    │ │   CORE N    │           │ │
│  │  │             │ │             │ │             │           │ │
│  │  │ • Execution │ │ • Execution │ │ • Execution │           │ │
│  │  │   Units     │ │   Units     │ │   Units     │           │ │
│  │  │ • L1/L2     │ │ • L1/L2     │ │ • L1/L2     │           │ │
│  │  │   Cache     │ │   Cache     │ │   Cache     │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │                  SHARED L3 CACHE                       │ │ │
│  │  │             MEMORY CONTROLLER                         │ │ │
│  │  │             INTERCONNECT (QPI/UPI)                    │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                     MAIN MEMORY                             │ │
│  │                                                             │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │   DRAM      │ │   NUMA      │ │   PERSISTENT│           │ │
│  │  │   MODULES   │ │   DOMAINS   │ │   MEMORY    │           │ │
│  │  │             │ │             │ │             │           │ │
│  │  │ • DDR4/DDR5 │ │ • Node 0/1  │ │ • Intel    │           │ │
│  │  │ • ECC       │ │ • Local/    │ │   Optane   │           │ │
│  │  │ • Channels  │ │   Remote    │ │ • NVMe     │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Data Flow: Banker's Algorithm Execution
```
┌─────────────────────────────────────────────────────────────────┐
│              BANKER'S ALGORITHM EXECUTION FLOW                  │
│                                                                 │
│  STEP 1: USER SPACE REQUEST                                     │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Application: P1 calls request_resources(1,0,0)              │ │
│  │                                                             │ │
│  │  CPU STATE (User Mode):                                     │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │    RDI      │ │    RSI      │ │    RDX      │           │ │
│  │  │  arg1 = 1   │ │  arg2 = 0   │ │  arg3 = 0   │           │ │
│  │  │  (R1 req)   │ │  (R2 req)   │ │  (R3 req)   │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  STACK FRAME:                                               │ │
│  │  ┌─────────────┐                                           │ │
│  │  │ return addr │                                           │ │
│  │  │ old RBP     │                                           │ │
│  │  │ local vars  │                                           │ │
│  │  └─────────────┘                                           │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  STEP 2: SYSTEM CALL ENTRY                                      │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  SYSCALL Instruction (特权指令)                              │ │
│  │                                                             │ │
│  │  CPU MICROCODE EXECUTION:                                   │ │
│  │  1. Read MSR_LSTAR → syscall entry point                     │ │
│  │  2. Save user RIP to RCX                                    │ │
│  │  3. Save user RFLAGS to R11                                 │ │
│  │  4. Load kernel RIP from MSR_LSTAR                          │ │
│  │  5. Load kernel RFLAGS (clear IF flag)                       │ │
│  │  6. Load kernel GS segment                                  │ │
│  │  7. Switch to kernel stack (TSS.RSP0)                       │ │
│  │                                                             │ │
│  │  PRIVILEGE LEVEL CHANGE: CPL 3 → CPL 0                       │ │
│  │  BRANCH PREDICTOR: Misprediction → pipeline flush           │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  STEP 3: KERNEL SPACE PROCESSING                                │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Kernel Syscall Handler:                                    │ │
│  │                                                             │ │
│  │  1. Verify arguments (copy_from_user)                       │ │
│  │  2. Access kernel data structures                           │ │
│  │  3. Execute Banker's Algorithm                             │ │
│  │                                                             │ │
│  │  KERNEL DATA STRUCTURES (in kernel memory):                 │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │ │
│  │  │ resource_   │ │ process_    │ │ banker_     │           │ │
│  │  │ table[3]    │ │ table[N]    │ │ state       │           │ │
│  │  │             │ │             │ │             │           │ │
│  │  │ Available   │ │ Max[3]      │ │ Work[3]     │           │ │
│  │  │ Allocation  │ │ Need[3]     │ │ Finish[N]   │           │ │
│  │  └─────────────┘ └─────────────┘ └─────────────┘           │ │
│  │                                                             │ │
│  │  CPU CACHE BEHAVIOR:                                        │ │
│  │  - L1: Kernel code and hot data                             │ │
│  │  - L2: Resource tables, process descriptors                 │ │
│  │  - L3: Less frequently used data                            │ │
│  │  - TLB: Kernel page table mappings                          │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  STEP 4: ALGORITHM EXECUTION                                   │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  Safety Algorithm Assembly-like Pseudocode:                  │ │
│  │                                                             │ │
│  │  ; Initialize Work and Finish                               │ │
│  │  MOV R8, [available]     ; R8 = Available vector            │ │
│  │  XOR R9, R9              ; R9 = 0 (Finish bitmap)           │ │
│  │                                                             │ │
│  │  ; Main loop: find process that can finish                  │ │
│  │  find_loop:                                                 │ │
│  │    MOV R10, 0             ; i = 0                          │ │
│  │  process_loop:                                              │ │
│  │    ; Check if process i can finish                          │ │
│  │    TEST R9, (1<<R10)       ; if Finish[i] = true, skip     │ │
│  │    JNZ next_process                                          │ │
│  │                                                             │ │
│  │    ; Compare Need[i] ≤ Work                                │ │
│  │    LEA R11, [need + R10*3]  ; R11 = &Need[i]               │ │
│  │    MOV R12, 3              ; resource count                │ │
│  │  compare_loop:                                              │ │
│  │    MOV R13, [R11]           ; Need[i][j]                   │ │
│  │    MOV R14, [R8 + R12*4]    ; Work[j]                      │ │
│  │    CMP R13, R14            ; Need[i][j] ≤ Work[j]?         │ │
│  │    JA cant_finish          ; if >, can't finish            │ │
│  │    INC R11                 ; next resource                 │ │
│  │    DEC R12                 ; decrement counter             │ │
│  │    JNZ compare_loop                                          │ │
│  │                                                             │ │
│  │    ; Process can finish!                                   │ │
│  │    LEA R15, [alloc + R10*3] ; R15 = &Allocation[i]          │ │
│  │    MOV R12, 3              ; resource count                │ │
│  │  add_loop:                                                   │ │
│  │    MOV R13, [R15]           ; Allocation[i][j]             │ │
│  │    ADD [R8 + R12*4], R13    ; Work[j] += Allocation[i][j]   │ │
│  │    INC R15                 ; next resource                 │ │
│  │    DEC R12                 ; decrement counter             │ │
│  │    JNZ add_loop                                              │ │
│  │                                                             │ │
│  │    OR R9, (1<<R10)          ; Finish[i] = true             │ │
│  │    JMP find_loop                                            │ │
│  │                                                             │ │
│  │  cant_finish:                                               │ │
│  │  next_process:                                              │ │
│  │    INC R10                 ; i++                           │ │
│  │    CMP R10, process_count                                   │ │
│  │    JL process_loop                                           │ │
│  │                                                             │ │
│  │    ; Check if all finished                                  │ │
│  │    CMP R9, all_finished_mask                                │ │
│  │    JE system_safe                                           │ │
│  │    JMP system_unsafe                                         │ │
│  │                                                             │ │
│  │  CPU EXECUTION CHARACTERISTICS:                             │ │
│  │  - Branch predictor accuracy: ~95%                          │ │
│  │  - L1I cache hit rate: ~98%                                 │ │
│  │  - L1D cache hit rate: ~95%                                 │ │
│  │  - Instructions per cycle: ~2.5 (out-of-order execution)    │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                  │                               │
│                                  ▼                               │
│  STEP 5: RETURN TO USER SPACE                                   │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │  SYSRET Instruction (特权指令)                               │ │
│  │                                                             │ │
│  │  CPU MICROCODE EXECUTION:                                   │ │
│  │  1. Load user RIP from RCX                                   │ │
│  │  2. Load user RFLAGS from R11                               │ │
│  │  3. Restore user GS segment                                 │ │
│  │  4. Switch to user stack                                   │ │
│  │  5. Restore user page tables (CR3)                          │ │
│  │                                                             │ │
│  │  PRIVILEGE LEVEL CHANGE: CPL 0 → CPL 3                       │ │
│  │  RESULT: Request denied, P1 must wait                       │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## Summary

This comprehensive visualization guide shows how the Banker's Algorithm and operating system concepts execute on real CPU hardware:

### Key Hardware Interactions:
1. **CPU Pipelines** execute the algorithm with branch prediction and out-of-order execution
2. **Cache Coherency** (MESI protocol) ensures data consistency across cores
3. **Atomic Operations** (LOCK prefix) provide thread-safe resource management
4. **Memory Management** unit handles virtual-to-physical address translation
5. **System Calls** trigger user/kernel mode transitions with context switches

### Synchronization Hardware:
- **Mutex**: Atomic compare-and-swap with futex fallback for contention
- **Semaphores**: Atomic increment/decrement with kernel blocking
- **Memory Barriers**: Ensure proper ordering of memory operations

### Banker's Algorithm CPU Impact:
- **Safety checks** require multiple memory accesses and comparisons
- **Resource allocation** needs atomic operations to prevent races
- **Context switches** add overhead when processes block/wait
- **Cache effects** influence performance with different data access patterns

This visualization helps understand how theoretical OS concepts translate into actual CPU execution and hardware interactions!