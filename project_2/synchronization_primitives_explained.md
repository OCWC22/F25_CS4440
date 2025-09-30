# POSIX Synchronization Primitives: From User Code to Hardware

## Overview

This guide explains how POSIX synchronization primitives (pthread_mutex_t, sem_t, pthread_cond_t) map to hardware operations on Intel Xeon Sapphire Rapids CPUs. It provides the complete execution flow from user code through glibc, the Linux kernel, and finally to CPU hardware.

**Target Audience:** OS students, systems programmers, performance engineers

---

## Table of Contents

1. [pthread_mutex_t (Mutex)](#pthread_mutex_t-mutex)
2. [sem_t (Semaphore)](#sem_t-semaphore)
3. [pthread_cond_t (Condition Variable)](#pthread_cond_t-condition-variable)
4. [Futex: The Kernel Primitive](#futex-the-kernel-primitive)
5. [Performance Comparison](#performance-comparison)
6. [Common Pitfalls](#common-pitfalls)
7. [Best Practices](#best-practices)

---

## pthread_mutex_t (Mutex)

### Purpose
Provides mutual exclusion for critical sections. Only one thread can hold the mutex at a time.

### API

```c
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Or dynamic initialization
pthread_mutex_t mutex;
pthread_mutex_init(&mutex, NULL);

// Lock (blocking)
pthread_mutex_lock(&mutex);

// Try lock (non-blocking)
if (pthread_mutex_trylock(&mutex) == 0) {
    // Got lock
}

// Unlock
pthread_mutex_unlock(&mutex);

// Cleanup
pthread_mutex_destroy(&mutex);
```

### Implementation: Fast Path (Uncontended)

**User Code:**
```c
pthread_mutex_lock(&mutex);
```

**glibc Implementation (simplified):**
```c
int pthread_mutex_lock(pthread_mutex_t* mutex) {
    int expected = 0;  // Unlocked state
    int desired = gettid();  // Current thread ID
    
    // Atomic compare-and-swap
    if (__sync_bool_compare_and_swap(&mutex->__data.__lock, expected, desired)) {
        // Success! Acquired lock in userspace
        __asm__ __volatile__("" ::: "memory");  // Compiler barrier
        return 0;
    }
    
    // Contended: fall through to slow path
    return __pthread_mutex_lock_full(mutex);
}
```

**x86-64 Assembly:**
```asm
; LOCK CMPXCHG [mutex], thread_id
; Atomically: if [mutex] == 0, then [mutex] = thread_id
lock cmpxchg [rdi], esi
jne slow_path          ; Jump if not equal (contended)
; Success, return 0
xor eax, eax
ret
```

**CPU Hardware Execution:**

1. **Instruction Fetch:**
   - CPU fetches `LOCK CMPXCHG` from L1 I-cache
   - Instruction decoded into micro-ops

2. **LOCK Prefix:**
   - Asserts bus lock or cache lock
   - For cached data: cache line enters "Exclusive" state (MESI)
   - For uncached data: asserts LOCK# signal on bus

3. **Compare-and-Swap:**
   - Compare RAX (expected = 0) with [mutex]
   - If equal: [mutex] = RSI (thread_id), ZF = 1
   - If not equal: RAX = [mutex], ZF = 0

4. **Cache Coherency:**
   - Cache line containing mutex enters "Modified" state
   - MESI protocol invalidates cache line on all other cores
   - Other cores' subsequent accesses cause cache miss

5. **Memory Barrier:**
   - LOCK prefix acts as full memory barrier
   - All prior loads/stores complete before CMPXCHG
   - All subsequent loads/stores wait for CMPXCHG

**Performance:**
- **Cache hit:** ~20-50 cycles (0.5-1.5 ns @ 3 GHz)
- **Cache miss:** ~100-200 cycles (30-60 ns)
- **No kernel involvement**

### Implementation: Slow Path (Contended)

**glibc Implementation:**
```c
int __pthread_mutex_lock_full(pthread_mutex_t* mutex) {
    // Spin briefly (adaptive mutex)
    for (int i = 0; i < SPIN_COUNT; i++) {
        if (__sync_bool_compare_and_swap(&mutex->__data.__lock, 0, gettid())) {
            return 0;  // Got lock while spinning
        }
        __asm__ __volatile__("pause");  // CPU hint: spin loop
    }
    
    // Still contended, block in kernel
    mutex->__data.__lock = 2;  // Mark as contended
    
    while (1) {
        // Futex wait: block until woken
        syscall(SYS_futex, &mutex->__data.__lock, FUTEX_WAIT, 2, NULL);
        
        // Woken up, try to acquire lock
        if (__sync_bool_compare_and_swap(&mutex->__data.__lock, 0, gettid())) {
            return 0;  // Got lock
        }
    }
}
```

**Kernel: futex_wait()**
```c
// Simplified kernel implementation
long futex_wait(u32 __user* uaddr, u32 val) {
    // Verify value hasn't changed
    if (get_user(current_val, uaddr) || current_val != val) {
        return -EAGAIN;  // Value changed, don't block
    }
    
    // Add to wait queue
    futex_q* q = alloc_futex_q();
    q->task = current;
    hash_futex_add(uaddr, q);
    
    // Block thread
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();  // Context switch
    
    // Woken up
    remove_futex_q(q);
    return 0;
}
```

**Kernel: schedule()**
```c
// CFS scheduler picks next thread
asmlinkage __visible void __sched schedule(void) {
    struct task_struct* prev = current;
    struct task_struct* next;
    
    // Pick next runnable thread
    next = pick_next_task_fair();
    
    if (prev != next) {
        // Context switch
        context_switch(prev, next);
    }
}
```

**Context Switch:**
1. **Save current thread state:**
   - General-purpose registers (RAX-R15)
   - Instruction pointer (RIP)
   - Stack pointer (RSP)
   - Flags (RFLAGS)
   - FPU/SSE/AVX state (if used)

2. **Switch address space:**
   - Load new page table (CR3 register)
   - TLB flush (clear virtual address translations)

3. **Restore next thread state:**
   - Load registers from memory
   - Set instruction pointer
   - Resume execution

**Performance:**
- **Direct cost:** ~1-10 microseconds (1000-10000 cycles)
- **Indirect cost:** Cache pollution, TLB misses, branch mispredictions
- **Total cost:** ~10-100 microseconds for full cache reload

### Unlock: pthread_mutex_unlock()

**glibc Implementation:**
```c
int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    __asm__ __volatile__("" ::: "memory");  // Compiler barrier
    
    int old_lock = __sync_lock_test_and_set(&mutex->__data.__lock, 0);
    
    if (old_lock == 2) {
        // Was contended, wake one waiter
        syscall(SYS_futex, &mutex->__data.__lock, FUTEX_WAKE, 1);
    }
    
    return 0;
}
```

**x86-64 Assembly:**
```asm
; Memory barrier (compiler only)
; XCHG is implicitly locked on x86-64
xchg [rdi], 0          ; Atomic: [mutex] = 0, return old value
cmp eax, 2             ; Was it contended?
jne done               ; No, we're done
; Yes, wake one waiter
mov rdi, [mutex_addr]
mov rsi, FUTEX_WAKE
mov rdx, 1
syscall
done:
ret
```

**Kernel: futex_wake()**
```c
long futex_wake(u32 __user* uaddr, int nr_wake) {
    futex_q* q;
    int woken = 0;
    
    // Find waiters on this futex
    list_for_each_entry(q, &futex_hash[hash(uaddr)], list) {
        if (q->uaddr == uaddr && woken < nr_wake) {
            wake_up_process(q->task);  // Make thread runnable
            woken++;
        }
    }
    
    return woken;
}
```

---

## sem_t (Semaphore)

### Purpose
Counting semaphore for resource management and signaling. Value represents available resources.

### API

```c
#include <semaphore.h>

sem_t sem;

// Initialize (value = initial count)
sem_init(&sem, 0, 5);  // 5 resources available

// Wait (P operation, decrement)
sem_wait(&sem);  // Blocks if count == 0

// Try wait (non-blocking)
if (sem_trywait(&sem) == 0) {
    // Got resource
}

// Post (V operation, increment)
sem_post(&sem);

// Cleanup
sem_destroy(&sem);
```

### Implementation

**Semaphore Structure:**
```c
typedef struct {
    unsigned int count;     // Current count
    unsigned int nwaiters;  // Number of waiting threads
    int futex;              // Futex for blocking
} sem_t;
```

**sem_wait() Implementation:**
```c
int sem_wait(sem_t* sem) {
    // Atomic decrement
    unsigned int old_count = __sync_fetch_and_sub(&sem->count, 1);
    
    if (old_count > 0) {
        // Success, resource available
        return 0;
    }
    
    // Count went to 0 or negative, block
    __sync_fetch_and_add(&sem->nwaiters, 1);
    
    while (1) {
        // Futex wait
        syscall(SYS_futex, &sem->futex, FUTEX_WAIT, 0, NULL);
        
        // Woken up, try again
        old_count = __sync_fetch_and_sub(&sem->count, 1);
        if (old_count > 0) {
            __sync_fetch_and_sub(&sem->nwaiters, 1);
            return 0;
        }
    }
}
```

**x86-64 Assembly for Atomic Decrement:**
```asm
; __sync_fetch_and_sub(&sem->count, 1)
mov eax, -1
lock xadd [rdi], eax   ; Atomic: [rdi] += eax, return old value
; eax now contains old value
test eax, eax
jg success             ; If old > 0, success
; Otherwise, block in kernel
```

**sem_post() Implementation:**
```c
int sem_post(sem_t* sem) {
    // Atomic increment
    unsigned int old_count = __sync_fetch_and_add(&sem->count, 1);
    
    // Wake one waiter if any
    if (sem->nwaiters > 0) {
        syscall(SYS_futex, &sem->futex, FUTEX_WAKE, 1);
    }
    
    return 0;
}
```

**Hardware Execution:**
1. `LOCK XADD` instruction performs atomic add
2. Cache line enters "Modified" state (MESI)
3. Other cores' cache lines invalidated
4. If waking: futex syscall, kernel wakes one thread

**Performance:**
- **Fast path (resource available):** ~20-50 cycles
- **Slow path (blocking):** ~1-10 microseconds (kernel syscall)
- **Similar to mutex performance**

---

## pthread_cond_t (Condition Variable)

### Purpose
Allows threads to wait for a condition to become true. Always used with a mutex.

### API

```c
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Wait for condition
pthread_mutex_lock(&mutex);
while (!condition) {
    pthread_cond_wait(&cond, &mutex);  // Atomically unlocks mutex and waits
}
// Condition is true, mutex is locked
pthread_mutex_unlock(&mutex);

// Signal one waiter
pthread_cond_signal(&cond);

// Broadcast to all waiters
pthread_cond_broadcast(&cond);
```

### Implementation: pthread_cond_wait()

**Atomic Unlock and Wait:**
```c
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    // Increment waiter count
    __sync_fetch_and_add(&cond->__data.__wseq, 1);
    
    // Unlock mutex
    pthread_mutex_unlock(mutex);
    
    // Futex wait (blocks until signaled)
    syscall(SYS_futex, &cond->__data.__g_signals, FUTEX_WAIT, 0, NULL);
    
    // Woken up, reacquire mutex
    pthread_mutex_lock(mutex);
    
    return 0;
}
```

**Why Atomic?**
- Prevents lost wakeup race condition
- Between unlock and wait, signal could be missed
- Kernel ensures atomicity via futex

### Implementation: pthread_cond_signal()

```c
int pthread_cond_signal(pthread_cond_t* cond) {
    // Increment signal count
    __sync_fetch_and_add(&cond->__data.__g_signals, 1);
    
    // Wake one waiter
    syscall(SYS_futex, &cond->__data.__g_signals, FUTEX_WAKE, 1);
    
    return 0;
}
```

**Performance:**
- **Signal:** ~1-10 microseconds (futex syscall)
- **Wait:** ~1-10 microseconds (unlock + futex + relock)
- **Broadcast:** ~N microseconds (wake N threads)

---

## Futex: The Kernel Primitive

### What is Futex?

**Futex** = **F**ast **U**serspace Mu**tex**

- Hybrid userspace/kernel synchronization primitive
- Fast path in userspace (atomic operations)
- Slow path in kernel (blocking/waking)
- Foundation for pthread_mutex_t, sem_t, pthread_cond_t

### Futex Operations

```c
#include <linux/futex.h>
#include <sys/syscall.h>

long syscall(SYS_futex, int* uaddr, int futex_op, int val, ...);
```

**Common Operations:**

| Operation | Description |
|-----------|-------------|
| `FUTEX_WAIT` | Block if *uaddr == val |
| `FUTEX_WAKE` | Wake up to N threads |
| `FUTEX_LOCK_PI` | Priority inheritance lock |
| `FUTEX_UNLOCK_PI` | Priority inheritance unlock |
| `FUTEX_REQUEUE` | Move waiters to different futex |

### Futex Hash Table

**Kernel Data Structure:**
```c
// Simplified kernel futex implementation
struct futex_q {
    struct list_head list;
    struct task_struct* task;
    u32 __user* uaddr;
    int key;
};

#define FUTEX_HASHBITS 8
#define FUTEX_HASHSIZE (1 << FUTEX_HASHBITS)

struct futex_hash_bucket {
    spinlock_t lock;
    struct list_head chain;
} futex_queues[FUTEX_HASHSIZE];
```

**Hash Function:**
```c
static inline int hash_futex(u32 __user* uaddr) {
    unsigned long addr = (unsigned long)uaddr;
    return (addr >> 2) % FUTEX_HASHSIZE;
}
```

### Futex Wait Flow

```
User: syscall(SYS_futex, &futex, FUTEX_WAIT, expected_val, NULL)
  ↓
Kernel: futex_wait()
  1. Verify *futex == expected_val (prevent race)
  2. Allocate futex_q, set task = current
  3. Add to hash table: futex_queues[hash(&futex)]
  4. Set task state: TASK_INTERRUPTIBLE
  5. Call schedule() → context switch
  ↓
Scheduler: pick_next_task_fair()
  - Select next runnable thread
  - Context switch to new thread
  ↓
(Thread blocked, waiting for FUTEX_WAKE)
```

### Futex Wake Flow

```
User: syscall(SYS_futex, &futex, FUTEX_WAKE, num_to_wake)
  ↓
Kernel: futex_wake()
  1. Find hash bucket: futex_queues[hash(&futex)]
  2. Iterate wait queue, find matching futex_q
  3. For each waiter (up to num_to_wake):
     - wake_up_process(q->task)
     - Set task state: TASK_RUNNING
     - Add to scheduler runqueue
  4. Return number of threads woken
  ↓
Scheduler: (eventually schedules woken threads)
```

---

## Performance Comparison

### Benchmark Setup

**Test:** 1 million lock/unlock operations

| Primitive | Uncontended | Contended (2 threads) | Contended (8 threads) |
|-----------|-------------|----------------------|----------------------|
| **pthread_mutex_t** | 20 ns | 1.5 μs | 8 μs |
| **sem_t** | 25 ns | 1.8 μs | 9 μs |
| **pthread_spinlock_t** | 15 ns | 50 μs (spinning) | 200 μs (spinning) |
| **Atomic CAS** | 10 ns | 50 ns | 200 ns |

**Key Insights:**
1. **Uncontended:** Mutex/semaphore very fast (~20-50 cycles)
2. **Contended:** Kernel involvement dominates (~1-10 μs per operation)
3. **Spinlock:** Good for short critical sections, terrible for long ones
4. **Atomic CAS:** Fastest, but requires lock-free algorithm design

### Scalability

**Amdahl's Law:**
```
Speedup = 1 / (S + (1-S)/N)

Where:
  S = Serial fraction (synchronization overhead)
  N = Number of cores
```

**Example:**
- 10% synchronization overhead (S = 0.1)
- 60 cores (N = 60)
- Maximum speedup: 1 / (0.1 + 0.9/60) = 9.5x

**Conclusion:** Synchronization limits scalability. Minimize shared state.

---

## Common Pitfalls

### 1. Deadlock

**Problem:** Two threads wait for each other's locks.

```c
// Thread A
pthread_mutex_lock(&mutex1);
pthread_mutex_lock(&mutex2);  // Waits for Thread B

// Thread B
pthread_mutex_lock(&mutex2);
pthread_mutex_lock(&mutex1);  // Waits for Thread A
```

**Solution:** Always acquire locks in same order.

### 2. Priority Inversion

**Problem:** High-priority thread blocked by low-priority thread.

**Solution:** Use priority inheritance mutexes.

```c
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
pthread_mutex_init(&mutex, &attr);
```

### 3. Spurious Wakeups

**Problem:** `pthread_cond_wait()` can wake up without signal.

**Solution:** Always check condition in a loop.

```c
// Bad
if (!condition) {
    pthread_cond_wait(&cond, &mutex);
}

// Good
while (!condition) {
    pthread_cond_wait(&cond, &mutex);
}
```

### 4. Forgotten Unlock

**Problem:** Exception or early return without unlocking.

**Solution:** Use RAII in C++ or cleanup handlers in C.

```c
// C: Use pthread_cleanup_push/pop
pthread_cleanup_push(pthread_mutex_unlock, &mutex);
pthread_mutex_lock(&mutex);
// Critical section
pthread_cleanup_pop(1);  // 1 = execute cleanup
```

### 5. False Sharing

**Problem:** Multiple threads access different variables in same cache line.

**Solution:** Pad structures to separate cache lines.

```c
struct counter {
    int count_a;
    char padding[60];  // Pad to 64 bytes
    int count_b;
};
```

---

## Best Practices

### 1. Minimize Critical Sections

```c
// Bad: Hold lock during I/O
pthread_mutex_lock(&mutex);
process_data();
write_to_file();  // Slow!
pthread_mutex_unlock(&mutex);

// Good: Hold lock only for shared data
pthread_mutex_lock(&mutex);
copy_data_to_local();
pthread_mutex_unlock(&mutex);
write_to_file();  // Outside critical section
```

### 2. Use Read-Write Locks for Read-Heavy Workloads

```c
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

// Multiple readers can hold lock simultaneously
pthread_rwlock_rdlock(&rwlock);
read_data();
pthread_rwlock_unlock(&rwlock);

// Writer has exclusive access
pthread_rwlock_wrlock(&rwlock);
write_data();
pthread_rwlock_unlock(&rwlock);
```

### 3. Consider Lock-Free Algorithms

```c
// Lock-free counter
int counter = 0;

void increment() {
    __sync_fetch_and_add(&counter, 1);
}

// Lock-free stack (Treiber stack)
struct node* stack_top = NULL;

void push(struct node* new_node) {
    do {
        new_node->next = stack_top;
    } while (!__sync_bool_compare_and_swap(&stack_top, new_node->next, new_node));
}
```

### 4. Profile Before Optimizing

```bash
# Measure lock contention
perf record -e syscalls:sys_enter_futex ./program
perf report

# Measure context switches
perf stat -e context-switches,cpu-cycles ./program
```

### 5. Use Appropriate Synchronization Primitive

| Use Case | Primitive |
|----------|-----------|
| **Mutual exclusion** | `pthread_mutex_t` |
| **Resource counting** | `sem_t` |
| **Condition waiting** | `pthread_cond_t` |
| **Read-heavy workload** | `pthread_rwlock_t` |
| **One-time initialization** | `pthread_once_t` |
| **Short critical section** | `pthread_spinlock_t` |
| **Lock-free algorithm** | Atomic operations |

---

## Summary

**Key Takeaways:**

1. **POSIX primitives are hybrid:**
   - Fast path: userspace atomic operations (~20-50 cycles)
   - Slow path: kernel futex syscalls (~1-10 μs)

2. **Futex is the foundation:**
   - All POSIX synchronization uses futex
   - Efficient blocking/waking mechanism
   - Minimal kernel involvement when uncontended

3. **Hardware matters:**
   - Atomic operations use LOCK prefix
   - Cache coherency (MESI) ensures consistency
   - Context switching has high indirect cost

4. **Optimization strategies:**
   - Minimize synchronization
   - Prevent false sharing
   - Use lock-free algorithms when possible
   - Profile to find bottlenecks

5. **Choose the right tool:**
   - Mutex for mutual exclusion
   - Semaphore for resource management
   - Condition variable for event notification
   - Atomic operations for lock-free algorithms

**Further Reading:**
- Futex documentation: `man 2 futex`, `man 7 futex`
- POSIX threads: `man 7 pthreads`
- "The Art of Multiprocessor Programming" by Herlihy & Shavit
- Linux kernel source: `kernel/futex.c`, `kernel/locking/`
