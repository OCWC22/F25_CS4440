# Intel Xeon Sapphire Rapids: Hardware Architecture Guide for OS Programmers

## Executive Summary

This guide connects POSIX concurrency primitives (pthread, semaphores) to Intel Xeon Sapphire Rapids CPU hardware execution. It explains the complete stack from user code through the Linux kernel to physical CPU operations, providing actionable insights for writing high-performance concurrent systems.

**Target Audience:** CTOs, senior engineers, OS course students, systems programmers

**Key Takeaways:**
1. Synchronization primitives map to atomic CPU instructions + kernel syscalls
2. Cache coherency (MESI protocol) ensures consistency across cores
3. Context switching has both direct (~1-10 μs) and indirect (cache pollution) costs
4. Hardware-aware design can improve performance by 10-100x

---

## Table of Contents

1. [Intel Xeon Sapphire Rapids Overview](#intel-xeon-sapphire-rapids-overview)
2. [CPU Architecture Deep Dive](#cpu-architecture-deep-dive)
3. [Cache Hierarchy and Coherency](#cache-hierarchy-and-coherency)
4. [Memory Ordering Model](#memory-ordering-model)
5. [Atomic Operations](#atomic-operations)
6. [POSIX Primitives → Hardware Mapping](#posix-primitives--hardware-mapping)
7. [Thread Execution Flow](#thread-execution-flow)
8. [Context Switching Mechanics](#context-switching-mechanics)
9. [Performance Optimization Strategies](#performance-optimization-strategies)
10. [NUMA Architecture](#numa-architecture)

---

## Intel Xeon Sapphire Rapids Overview

### Key Specifications

| Feature | Specification |
|---------|--------------|
| **Process Technology** | Intel 7 (7nm) |
| **Cores per Socket** | Up to 60 cores |
| **Threads per Core** | 2 (SMT/Hyper-Threading) |
| **Max Threads** | 120 per socket |
| **L1 Cache** | 48KB I-cache + 32KB D-cache per core |
| **L2 Cache** | 2MB per core (private) |
| **L3 Cache** | Up to 112.5MB (shared) |
| **Cache Line Size** | 64 bytes |
| **Memory Channels** | 8 DDR5 channels per socket |
| **Max Memory** | 4TB per socket |
| **PCIe** | Gen 5.0, 80 lanes |
| **TDP** | 225-350W |

### Architecture Highlights

**Golden Cove Microarchitecture:**
- 14-stage pipeline (deeper than previous generations)
- 6-wide decode, 12-wide allocation
- Out-of-order execution with 512-entry ROB (reorder buffer)
- Advanced branch prediction (TAGE predictor)
- Hardware prefetchers (L1, L2, L3)

**Advanced Features:**
- **AMX (Advanced Matrix Extensions):** Accelerates AI/ML workloads
- **DSA (Data Streaming Accelerator):** Offloads memory operations
- **IAA (In-Memory Analytics Accelerator):** Compression/decompression
- **HLE/RTM:** Hardware lock elision and transactional memory
- **CXL (Compute Express Link):** High-bandwidth memory expansion

---

## CPU Architecture Deep Dive

### Core Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                    Golden Cove Pipeline                      │
├─────────────────────────────────────────────────────────────┤
│ Fetch (4 stages)                                            │
│   ├─ Instruction fetch from L1 I-cache                      │
│   ├─ Branch prediction (TAGE + neural predictor)            │
│   └─ Macro-op fusion                                        │
├─────────────────────────────────────────────────────────────┤
│ Decode (2 stages)                                           │
│   ├─ 6-wide decode (x86 → micro-ops)                        │
│   └─ Micro-op cache (4K entries)                            │
├─────────────────────────────────────────────────────────────┤
│ Allocate/Rename (2 stages)                                  │
│   ├─ 12-wide allocation                                     │
│   ├─ Register renaming (physical register file)             │
│   └─ ROB allocation (512 entries)                           │
├─────────────────────────────────────────────────────────────┤
│ Schedule/Execute (variable)                                 │
│   ├─ Unified reservation station (320 entries)              │
│   ├─ 12 execution ports                                     │
│   ├─ 3 ALU ports, 2 load ports, 2 store ports               │
│   └─ Out-of-order execution                                 │
├─────────────────────────────────────────────────────────────┤
│ Retire (variable)                                           │
│   ├─ In-order retirement                                    │
│   └─ Update architectural state                             │
└─────────────────────────────────────────────────────────────┘
```

### Execution Units

| Port | Execution Units | Operations |
|------|----------------|------------|
| 0 | ALU, Vector ALU, Branch | Integer, FP, Vector |
| 1 | ALU, Vector ALU, FMA | Integer, FP, Vector |
| 2 | Load, AGU | Memory load |
| 3 | Load, AGU | Memory load |
| 4 | Store Data | Memory store data |
| 5 | ALU, Vector ALU, FMA | Integer, FP, Vector |
| 6 | ALU, Branch | Integer, Branch |
| 7 | Store Address, AGU | Memory store address |
| 8 | Store Address, AGU | Memory store address |
| 9 | Load, AGU | Memory load |
| 10 | Load, AGU | Memory load |
| 11 | Store Data | Memory store data |

**Key Insight:** Multiple execution units enable instruction-level parallelism (ILP). The CPU can execute multiple independent instructions simultaneously.

---

## Cache Hierarchy and Coherency

### Cache Organization

```
┌──────────────────────────────────────────────────────────────┐
│                         CPU Core                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ L1 Instruction Cache: 48KB, 8-way, 64B lines          │  │
│  │ Latency: ~4 cycles                                     │  │
│  └────────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ L1 Data Cache: 32KB, 8-way, 64B lines                 │  │
│  │ Latency: ~4 cycles                                     │  │
│  │ Bandwidth: ~200 GB/s per core                          │  │
│  └────────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ L2 Cache: 2MB, 16-way, 64B lines (private)            │  │
│  │ Latency: ~12 cycles                                    │  │
│  │ Bandwidth: ~100 GB/s per core                          │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
                            ↓
┌──────────────────────────────────────────────────────────────┐
│ L3 Cache: Up to 112.5MB, 15-way, 64B lines (shared)         │
│ Latency: ~40 cycles                                          │
│ Bandwidth: ~500 GB/s aggregate                               │
│ Inclusive of L1/L2 (tracks all cache lines)                 │
└──────────────────────────────────────────────────────────────┘
                            ↓
┌──────────────────────────────────────────────────────────────┐
│ Main Memory: DDR5-4800, 8 channels                          │
│ Latency: ~100-200 cycles (~50-100 ns)                       │
│ Bandwidth: ~300 GB/s per socket                              │
└──────────────────────────────────────────────────────────────┘
```

### MESI Cache Coherency Protocol

**States:**
- **M (Modified):** Cache line is dirty, this core has exclusive copy
- **E (Exclusive):** Cache line is clean, this core has exclusive copy
- **S (Shared):** Cache line is clean, multiple cores may have copies
- **I (Invalid):** Cache line is invalid

**State Transitions:**

```
Thread A (Core 0)          Cache Line State          Thread B (Core 1)
─────────────────          ────────────────          ─────────────────
Read variable X            Core 0: E (Exclusive)     
                           Core 1: I (Invalid)       
                                                     Read variable X
                           Core 0: S (Shared)        
                           Core 1: S (Shared)        
Write variable X           Core 0: M (Modified)      
                           Core 1: I (Invalid)       
                                                     Read variable X
                           Core 0: S (Shared)        
                           Core 1: S (Shared)        
```

**Coherency Traffic:**
- **Read miss:** Request cache line from L3 or other cores
- **Write:** Invalidate cache line on all other cores
- **LOCK prefix:** Ensures atomic operation across all cores

**Performance Impact:**
- **Cache hit:** Fast (~4-40 cycles)
- **Cache miss:** Slow (~100-200 cycles)
- **False sharing:** Unnecessary coherency traffic (10-100x slowdown)

---

## Memory Ordering Model

### x86-64 Total Store Order (TSO)

**Guarantees:**
1. **Loads are not reordered with loads**
2. **Stores are not reordered with stores**
3. **Stores are not reordered with prior loads**
4. **Loads may be reordered with prior stores to different locations**

**Implications for Concurrency:**
- Most synchronization patterns work without explicit barriers
- LOCK prefix provides full memory barrier
- Atomic operations include implicit barriers
- Compiler barriers still needed to prevent compiler reordering

### Memory Barriers

| Barrier Type | x86-64 Instruction | Effect |
|--------------|-------------------|--------|
| **Full Barrier** | `MFENCE` | All loads/stores complete before barrier |
| **Store Barrier** | `SFENCE` | All stores complete before barrier |
| **Load Barrier** | `LFENCE` | All loads complete before barrier |
| **LOCK Prefix** | `LOCK <instr>` | Full barrier + atomic operation |
| **Compiler Barrier** | `asm volatile("" ::: "memory")` | Prevents compiler reordering |

**Example:**
```c
// Without barrier: compiler/CPU may reorder
int ready = 0;
int data = 0;

// Thread A
data = 42;
ready = 1;  // May be reordered before data = 42!

// Thread B
if (ready) {
    use(data);  // May see old value of data!
}

// With barrier:
data = 42;
__sync_synchronize();  // Full memory barrier
ready = 1;
```

---

## Atomic Operations

### x86-64 Atomic Instructions

| Instruction | Description | Use Case |
|-------------|-------------|----------|
| `LOCK ADD/SUB` | Atomic add/subtract | Counters, semaphores |
| `LOCK INC/DEC` | Atomic increment/decrement | Reference counting |
| `LOCK CMPXCHG` | Compare-and-swap | Mutexes, lock-free algorithms |
| `LOCK XCHG` | Atomic exchange | Spinlocks |
| `LOCK BTS/BTR` | Atomic bit test and set/reset | Bitmaps, flags |
| `XADD` | Atomic exchange and add | Fetch-and-add |

### Compare-and-Swap (CAS) Example

```c
// Atomic compare-and-swap
bool cas(int* ptr, int old_val, int new_val) {
    // x86-64 assembly:
    // LOCK CMPXCHG [ptr], new_val
    return __sync_bool_compare_and_swap(ptr, old_val, new_val);
}

// Lock-free increment
void atomic_increment(int* counter) {
    int old_val, new_val;
    do {
        old_val = *counter;
        new_val = old_val + 1;
    } while (!cas(counter, old_val, new_val));
}
```

### Hardware Execution

```
1. CPU fetches LOCK CMPXCHG instruction
2. LOCK prefix asserts bus lock or cache lock
3. Cache line enters "Exclusive" state (MESI)
4. Compare: RAX == [ptr]?
5. If equal: [ptr] = new_val, ZF = 1
6. If not equal: RAX = [ptr], ZF = 0
7. Cache coherency: invalidate line on other cores
8. Release lock, instruction retires
```

**Performance:**
- **Cache hit:** ~20-50 cycles
- **Cache miss:** ~100-200 cycles
- **Contention:** Serializes across cores (bottleneck)

---

## POSIX Primitives → Hardware Mapping

### pthread_mutex_t

**Implementation Layers:**

```
User Code: pthread_mutex_lock(&mutex)
    ↓
glibc (POSIX library):
    Fast path: LOCK CMPXCHG [mutex], thread_id
    if (success) return 0;
    Slow path: syscall(SYS_futex, FUTEX_WAIT, ...)
    ↓
Linux Kernel:
    futex_wait():
        - Add thread to wait queue
        - Set thread state: INTERRUPTIBLE_SLEEP
        - Call schedule()
    ↓
Scheduler (CFS):
    - Pick next runnable thread
    - Context switch: save/restore registers
    ↓
Intel Xeon CPU:
    - LOCK prefix: cache line locking
    - MESI protocol: invalidate other cores
    - Context switch: TLB flush, cache miss
    - Execute next thread's instructions
```

**Fast Path (Uncontended):**
```c
// Pseudo-code for mutex lock fast path
int pthread_mutex_lock(pthread_mutex_t* mutex) {
    int expected = 0;  // Unlocked
    int desired = gettid();  // Current thread ID
    
    // Atomic CAS: if mutex == 0, set to thread ID
    if (__sync_bool_compare_and_swap(&mutex->lock, expected, desired)) {
        return 0;  // Success, ~20-50 cycles
    }
    
    // Contended: fall through to slow path
    return futex_lock_slow_path(mutex);
}
```

**Slow Path (Contended):**
```c
// Pseudo-code for mutex lock slow path
int futex_lock_slow_path(pthread_mutex_t* mutex) {
    // Spin briefly (adaptive mutex)
    for (int i = 0; i < SPIN_COUNT; i++) {
        if (try_lock(mutex)) return 0;
        cpu_relax();  // PAUSE instruction
    }
    
    // Give up, block in kernel
    syscall(SYS_futex, &mutex->lock, FUTEX_WAIT, ...);
    // Thread blocked, ~1-10 microseconds
    
    // Woken up, try to acquire lock
    return try_lock(mutex);
}
```

### sem_t (Semaphore)

**Implementation:**

```c
// Semaphore structure
typedef struct {
    int count;       // Current count
    int waiters;     // Number of waiting threads
    int futex;       // Futex for blocking
} sem_t;

// sem_wait() implementation
int sem_wait(sem_t* sem) {
    // Atomic decrement
    int old_count = __sync_fetch_and_sub(&sem->count, 1);
    
    if (old_count > 0) {
        return 0;  // Success, fast path
    }
    
    // Count went negative, block
    __sync_fetch_and_add(&sem->waiters, 1);
    syscall(SYS_futex, &sem->futex, FUTEX_WAIT, ...);
    __sync_fetch_and_sub(&sem->waiters, 1);
    
    return 0;
}

// sem_post() implementation
int sem_post(sem_t* sem) {
    // Atomic increment
    int old_count = __sync_fetch_and_add(&sem->count, 1);
    
    if (old_count < 0 || sem->waiters > 0) {
        // Wake one waiter
        syscall(SYS_futex, &sem->futex, FUTEX_WAKE, 1);
    }
    
    return 0;
}
```

**Hardware Execution:**
1. `__sync_fetch_and_sub()` → `LOCK XADD` instruction
2. Cache line enters "Modified" state (MESI)
3. Other cores' cache lines invalidated
4. If blocking: futex syscall, context switch
5. If waking: futex syscall, scheduler wakes thread

---

## Thread Execution Flow

### Thread Creation: pthread_create()

```
User Code: pthread_create(&thread, NULL, func, arg)
    ↓
glibc:
    - Allocate thread stack (8MB default)
    - Set up thread-local storage (TLS)
    - Call clone() syscall
    ↓
Linux Kernel:
    clone(CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD):
        - Create new task_struct
        - Share address space (CLONE_VM)
        - Share file descriptors (CLONE_FILES)
        - Allocate new kernel stack
        - Add to scheduler runqueue
    ↓
Scheduler:
    - Assign thread to CPU core
    - Set initial priority (inherit from parent)
    - Calculate vruntime (CFS)
    ↓
Intel Xeon CPU:
    - Load thread context (registers, PC)
    - Start executing thread function
    - Instruction fetch from L1 I-cache
```

### Thread Scheduling: CFS (Completely Fair Scheduler)

**Algorithm:**
- Each thread has `vruntime` (virtual runtime)
- Scheduler picks thread with smallest `vruntime`
- After running, `vruntime` increases by actual runtime
- Red-black tree for O(log N) selection

**Time Slice:**
- Default: 6ms per thread
- Adjustable via `/proc/sys/kernel/sched_latency_ns`
- Preemption: timer interrupt every 1ms (CONFIG_HZ=1000)

**Priority:**
- Nice values: -20 (highest) to +19 (lowest)
- Real-time priorities: 1-99 (SCHED_FIFO, SCHED_RR)
- Default: nice 0, SCHED_OTHER

---

## Context Switching Mechanics

### What Gets Saved/Restored

**User-Space Registers:**
- General-purpose: RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8-R15
- Instruction pointer: RIP
- Flags: RFLAGS
- Segment registers: CS, DS, ES, FS, GS, SS

**FPU/SIMD State:**
- x87 FPU registers (8 × 80-bit)
- SSE registers (16 × 128-bit XMM)
- AVX registers (16 × 256-bit YMM)
- AVX-512 registers (32 × 512-bit ZMM) - if used

**Kernel State:**
- Kernel stack pointer
- Thread-local storage (FS register)
- Signal mask
- CPU affinity mask

### Context Switch Cost

**Direct Cost:**
- Save registers: ~50-100 cycles
- Restore registers: ~50-100 cycles
- Kernel overhead: ~500-1000 cycles
- **Total: ~1-10 microseconds**

**Indirect Cost:**
- **TLB flush:** Clear virtual address translations (~1000 cycles)
- **Cache pollution:** New thread evicts old thread's cache lines
- **Working set reload:** ~10,000-100,000 cycles
- **Branch predictor:** Mispredictions until warmed up

**Measurement:**
```bash
# Measure context switch time
perf stat -e context-switches,cpu-cycles ./program

# Typical results:
# 10,000 context switches
# 1,000,000,000 cycles (1 GHz CPU)
# Average: 100,000 cycles per context switch (~100 μs)
```

---

## Performance Optimization Strategies

### 1. Minimize Synchronization

**Problem:** Frequent locking causes contention and context switching.

**Solutions:**
- **Batch operations:** Acquire lock once, process multiple items
- **Lock-free algorithms:** Use atomic operations instead of locks
- **Read-write locks:** Allow concurrent readers
- **RCU (Read-Copy-Update):** Defer updates, no reader locks

**Example:**
```c
// Bad: Lock per item
for (int i = 0; i < N; i++) {
    pthread_mutex_lock(&mutex);
    process_item(i);
    pthread_mutex_unlock(&mutex);
}

// Good: Lock once, process all items
pthread_mutex_lock(&mutex);
for (int i = 0; i < N; i++) {
    process_item(i);
}
pthread_mutex_unlock(&mutex);
```

### 2. Prevent False Sharing

**Problem:** Multiple threads access different variables in same cache line.

**Solution:** Pad structures to separate cache lines (64 bytes).

```c
// Bad: False sharing
struct counter {
    int count_a;  // Thread A writes
    int count_b;  // Thread B writes
};  // Both in same cache line!

// Good: Separate cache lines
struct counter {
    int count_a;
    char padding[60];  // Pad to 64 bytes
    int count_b;
    char padding2[60];
};
```

### 3. CPU Affinity

**Problem:** Thread migration causes cache misses.

**Solution:** Pin threads to specific cores.

```c
#include <sched.h>

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}
```

### 4. NUMA Awareness

**Problem:** Cross-NUMA memory access is slow (~2x latency).

**Solution:** Allocate memory on local NUMA node.

```c
#include <numaif.h>

void* numa_aware_alloc(size_t size) {
    int node = numa_node_of_cpu(sched_getcpu());
    return numa_alloc_onnode(size, node);
}
```

### 5. Lock-Free Algorithms

**Problem:** Locks serialize execution, limit scalability.

**Solution:** Use atomic operations for lock-free data structures.

```c
// Lock-free stack (Treiber stack)
struct node {
    int data;
    struct node* next;
};

struct node* stack_top = NULL;

void push(int data) {
    struct node* new_node = malloc(sizeof(struct node));
    new_node->data = data;
    
    do {
        new_node->next = stack_top;
    } while (!__sync_bool_compare_and_swap(&stack_top, new_node->next, new_node));
}

int pop(void) {
    struct node* old_top;
    struct node* new_top;
    
    do {
        old_top = stack_top;
        if (!old_top) return -1;  // Empty
        new_top = old_top->next;
    } while (!__sync_bool_compare_and_swap(&stack_top, old_top, new_top));
    
    int data = old_top->data;
    free(old_top);
    return data;
}
```

---

## NUMA Architecture

### Multi-Socket Configuration

```
┌─────────────────────────────────────────────────────────────┐
│ Socket 0 (NUMA Node 0)                                      │
│   ├─ 60 cores, 120 threads                                  │
│   ├─ 112.5MB L3 cache                                       │
│   └─ 2TB DDR5 memory (local)                                │
└─────────────────────────────────────────────────────────────┘
                            ↕ UPI (Ultra Path Interconnect)
┌─────────────────────────────────────────────────────────────┐
│ Socket 1 (NUMA Node 1)                                      │
│   ├─ 60 cores, 120 threads                                  │
│   ├─ 112.5MB L3 cache                                       │
│   └─ 2TB DDR5 memory (local)                                │
└─────────────────────────────────────────────────────────────┘
```

### Memory Access Latency

| Access Type | Latency | Bandwidth |
|-------------|---------|-----------|
| **Local memory** | ~100 ns | ~300 GB/s |
| **Remote memory** | ~200 ns | ~150 GB/s |
| **Ratio** | 2x slower | 2x lower |

### NUMA-Aware Programming

```c
#include <numa.h>

// Check NUMA availability
if (numa_available() < 0) {
    fprintf(stderr, "NUMA not available\n");
    exit(1);
}

// Get current NUMA node
int node = numa_node_of_cpu(sched_getcpu());
printf("Running on NUMA node %d\n", node);

// Allocate memory on local node
void* buffer = numa_alloc_onnode(size, node);

// Interleave memory across all nodes (for shared data)
void* shared = numa_alloc_interleaved(size);

// Pin thread to local node
struct bitmask* nodemask = numa_allocate_nodemask();
numa_bitmask_setbit(nodemask, node);
numa_bind(nodemask);
```

---

## Summary: Key Takeaways for OS Programmers

1. **Synchronization is Expensive:**
   - Uncontended lock: ~20-50 cycles
   - Contended lock: ~1-10 μs (kernel syscall)
   - Context switch: ~1-10 μs direct + cache pollution

2. **Cache Coherency Matters:**
   - False sharing: 10-100x slowdown
   - Pad critical structures to 64-byte cache lines
   - Minimize shared writable data

3. **Memory Ordering:**
   - x86-64 TSO is relatively strong
   - LOCK prefix provides full barrier
   - Compiler barriers prevent reordering

4. **NUMA Awareness:**
   - Local memory: 2x faster than remote
   - Pin threads to local node
   - Interleave shared data

5. **Lock-Free Algorithms:**
   - Eliminate kernel involvement
   - Scale better under contention
   - Harder to implement correctly

6. **Measure, Don't Guess:**
   - Use `perf` to measure performance
   - Profile cache misses, context switches
   - Optimize hot paths only

**Further Reading:**
- Intel 64 and IA-32 Architectures Optimization Reference Manual
- "What Every Programmer Should Know About Memory" by Ulrich Drepper
- Linux kernel documentation: Documentation/scheduler/
- WikiChip: Intel Sapphire Rapids microarchitecture
