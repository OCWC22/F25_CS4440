# Part 3: Synchronization Deep Dive
## From Race Conditions to Hardware Atomics

**Learning Objectives:**
- Understand race conditions and why they occur
- Master mutex and semaphore usage
- Learn hardware implementation of synchronization
- Solve the Producer-Consumer problem
- Prevent deadlock

---

## 1. The Race Condition Problem

### 1.1 What is a Race Condition?

**Definition:** Multiple threads accessing shared data simultaneously, causing incorrect results.

**CEO Analogy:** Two employees trying to update the same spreadsheet cell at the same time. Without coordination, data gets corrupted.

### 1.2 Example: Counter Increment

```c
// Shared variable
int counter = 0;

// Thread 1
void* increment() {
    for (int i = 0; i < 1000000; i++) {
        counter++;  // NOT ATOMIC!
    }
}

// Thread 2
void* increment() {
    for (int i = 0; i < 1000000; i++) {
        counter++;  // NOT ATOMIC!
    }
}

// Expected: counter = 2,000,000
// Actual: counter = 1,234,567 (WRONG!)
```

**Why It Fails:**

```
Assembly for "counter++":
1. LOAD counter into register    (MOV RAX, [counter])
2. INCREMENT register             (INC RAX)
3. STORE register to counter      (MOV [counter], RAX)

Interleaving:
Time | Thread 1          | Thread 2          | counter
─────┼───────────────────┼───────────────────┼────────
  1  | LOAD (0)          |                   | 0
  2  |                   | LOAD (0)          | 0
  3  | INC (0 → 1)       |                   | 0
  4  |                   | INC (0 → 1)       | 0
  5  | STORE 1           |                   | 1
  6  |                   | STORE 1           | 1  ← Lost update!

Result: counter = 1 (should be 2)
```

**Business Impact:** In financial systems, race conditions can cause:
- Lost transactions
- Incorrect balances
- Data corruption
- Security vulnerabilities

---

## 2. Mutex: Mutual Exclusion

### 2.1 Concept

**Mutex:** A lock that ensures only one thread accesses a critical section at a time.

**CEO Analogy:** Single-person bathroom. Only one person can enter at a time. Others wait outside.

### 2.2 Basic Usage

```c
// From week_5/example13_mutex.cpp
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lock;

void *print_a(void *) {
    pthread_mutex_lock(&lock);    // ← Acquire lock
    for (int i = 0; i < 10; i++) {
        printf("A\n");
        usleep(10000);
    }
    pthread_mutex_unlock(&lock);  // ← Release lock
    pthread_exit(NULL);
}

void *print_b(void *) {
    pthread_mutex_lock(&lock);    // ← Acquire lock
    for (int i = 0; i < 10; i++) {
        printf("B\n");
        usleep(10000);
    }
    pthread_mutex_unlock(&lock);  // ← Release lock
    pthread_exit(NULL);
}

int main(void) {
    pthread_t tid_1, tid_2;
    
    pthread_mutex_init(&lock, NULL);
    
    pthread_create(&tid_1, NULL, &print_a, NULL);
    pthread_create(&tid_2, NULL, &print_b, NULL);
    
    pthread_join(tid_1, NULL);
    pthread_join(tid_2, NULL);
    
    pthread_mutex_destroy(&lock);
    
    return 0;
}
```

**Output (with mutex):**
```
A
A
A
...
A (10 times)
B
B
B
...
B (10 times)
```

**Output (without mutex):**
```
A
B
A
B
... (interleaved, unpredictable)
```

### 2.3 Mutex Implementation (Software)

**glibc pthread_mutex_lock() (simplified):**

```c
int pthread_mutex_lock(pthread_mutex_t* mutex) {
    // Fast path: Try to acquire without kernel
    if (atomic_compare_and_swap(&mutex->lock, 0, 1)) {
        return 0;  // Success! (~20-50 cycles)
    }
    
    // Slow path: Contention detected
    return mutex_lock_slow(mutex);
}

int mutex_lock_slow(pthread_mutex_t* mutex) {
    // Spin briefly (adaptive mutex)
    for (int i = 0; i < SPIN_COUNT; i++) {
        if (atomic_compare_and_swap(&mutex->lock, 0, 1)) {
            return 0;  // Got it while spinning
        }
        CPU_PAUSE();  // PAUSE instruction
    }
    
    // Give up, block in kernel
    syscall(SYS_futex, &mutex->lock, FUTEX_WAIT, 1, NULL);
    // Thread blocked here (~1-10 microseconds)
    
    // Woken up, try again
    return pthread_mutex_lock(mutex);
}
```

### 2.4 Hardware Implementation

**x86-64 Atomic Compare-and-Swap:**

```assembly
; LOCK CMPXCHG instruction
; Atomically: if (*ptr == expected) *ptr = new_value

LOCK CMPXCHG [mutex_address], thread_id

; Hardware execution:
; 1. LOCK prefix asserts cache lock
; 2. Cache line enters "Exclusive" state (MESI)
; 3. Other cores' cache lines invalidated
; 4. Compare and swap operation
; 5. Memory barrier (prevents reordering)
```

**Intel Xeon Execution:**

```
Thread 1 (Core 0):                    Thread 2 (Core 1):
─────────────────                     ─────────────────
pthread_mutex_lock(&mutex)
  ↓
LOCK CMPXCHG [mutex], 1
  ↓
Cache line → "Modified" (M)
  ↓                                   pthread_mutex_lock(&mutex)
Lock acquired! (fast path)              ↓
  ↓                                   LOCK CMPXCHG [mutex], 1
Critical section                        ↓
  ↓                                   Cache miss! (Core 0 has line)
  ↓                                     ↓
  ↓                                   Cache line transferred
  ↓                                     ↓
  ↓                                   CAS fails (mutex = 1, not 0)
  ↓                                     ↓
  ↓                                   Spin loop (PAUSE)
  ↓                                     ↓
pthread_mutex_unlock(&mutex)          Still spinning...
  ↓                                     ↓
LOCK XCHG [mutex], 0                  CAS succeeds!
  ↓                                     ↓
Cache line → "Shared" (S)             Lock acquired!
```

**Performance:**

```
Uncontended Lock:
- Fast path: 20-50 CPU cycles
- No kernel involvement
- Pure userspace

Contended Lock (short wait):
- Spin loop: 100-1000 cycles
- Still no kernel
- PAUSE reduces power

Contended Lock (long wait):
- Futex syscall: 1-10 microseconds
- Kernel blocks thread
- Context switch
```

---

## 3. Semaphores: Counting Synchronization

### 3.1 Concept

**Semaphore:** A counter that controls access to a resource.

**Types:**
- **Binary semaphore:** Acts like mutex (0 or 1)
- **Counting semaphore:** Allows N concurrent accesses

**CEO Analogy:**
- **Mutex:** Single-person bathroom
- **Semaphore:** Conference room with 10 chairs (up to 10 people)

### 3.2 Basic Usage

```c
#include <semaphore.h>

sem_t semaphore;

// Initialize with count = 3 (allow 3 concurrent accesses)
sem_init(&semaphore, 0, 3);

void* worker(void* arg) {
    sem_wait(&semaphore);  // Decrement (block if 0)
    
    // Critical section (max 3 threads here)
    printf("Thread %d working\n", *(int*)arg);
    sleep(1);
    
    sem_post(&semaphore);  // Increment (wake waiting thread)
    return NULL;
}
```

### 3.3 Semaphore Implementation

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

```
sem_wait():
1. __sync_fetch_and_sub() → LOCK XADD instruction
2. Cache line enters "Modified" state (MESI)
3. Other cores' cache lines invalidated
4. If blocking: futex syscall, context switch
5. Thread enters WAITING state

sem_post():
1. __sync_fetch_and_add() → LOCK XADD instruction
2. Cache coherency update
3. If waiters: futex syscall, wake one thread
4. Woken thread enters READY state
5. Scheduler picks thread to run
```

---

## 4. Producer-Consumer Problem

### 4.1 Problem Statement

**Scenario:**
- Producer threads generate data
- Consumer threads process data
- Shared buffer (bounded size)

**Constraints:**
- Producer waits if buffer is full
- Consumer waits if buffer is empty
- Only one thread accesses buffer at a time

**Business Analogy:** Factory assembly line with limited storage between stations.

### 4.2 Complete Solution

```c
// From project_2/bounded_buffer.c
#define BUFFER_SIZE 10
#define NUM_ITEMS 50

typedef struct {
    char buffer[BUFFER_SIZE];
    int in;    // Producer index
    int out;   // Consumer index
    int count; // Current items
    
    pthread_mutex_t mutex;  // Protects buffer
    sem_t empty;            // Counts empty slots
    sem_t full;             // Counts full slots
} bounded_buffer_t;

bounded_buffer_t shared_buffer;

void* producer(void* arg) {
    int id = *(int*)arg;
    
    for (int i = 0; i < NUM_ITEMS; i++) {
        char item = 'A' + (rand() % 26);
        
        // Wait for empty slot
        sem_wait(&shared_buffer.empty);
        
        // Acquire exclusive access
        pthread_mutex_lock(&shared_buffer.mutex);
        
        // Add item to buffer
        shared_buffer.buffer[shared_buffer.in] = item;
        printf("[Producer %d] Produced: %c at position %d\n", 
               id, item, shared_buffer.in);
        shared_buffer.in = (shared_buffer.in + 1) % BUFFER_SIZE;
        shared_buffer.count++;
        
        // Release access
        pthread_mutex_unlock(&shared_buffer.mutex);
        
        // Signal full slot
        sem_post(&shared_buffer.full);
        
        usleep(100);
    }
    
    return NULL;
}

void* consumer(void* arg) {
    int id = *(int*)arg;
    
    for (int i = 0; i < NUM_ITEMS; i++) {
        // Wait for full slot
        sem_wait(&shared_buffer.full);
        
        // Acquire exclusive access
        pthread_mutex_lock(&shared_buffer.mutex);
        
        // Remove item from buffer
        char item = shared_buffer.buffer[shared_buffer.out];
        printf("[Consumer %d] Consumed: %c from position %d\n", 
               id, item, shared_buffer.out);
        shared_buffer.out = (shared_buffer.out + 1) % BUFFER_SIZE;
        shared_buffer.count--;
        
        // Release access
        pthread_mutex_unlock(&shared_buffer.mutex);
        
        // Signal empty slot
        sem_post(&shared_buffer.empty);
        
        usleep(100);
    }
    
    return NULL;
}

int main(void) {
    pthread_t prod_thread, cons_thread;
    int prod_id = 1, cons_id = 1;
    
    // Initialize
    shared_buffer.in = 0;
    shared_buffer.out = 0;
    shared_buffer.count = 0;
    
    pthread_mutex_init(&shared_buffer.mutex, NULL);
    sem_init(&shared_buffer.empty, 0, BUFFER_SIZE);  // 10 empty
    sem_init(&shared_buffer.full, 0, 0);             // 0 full
    
    // Create threads
    pthread_create(&prod_thread, NULL, producer, &prod_id);
    pthread_create(&cons_thread, NULL, consumer, &cons_id);
    
    // Wait for completion
    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);
    
    // Cleanup
    pthread_mutex_destroy(&shared_buffer.mutex);
    sem_destroy(&shared_buffer.empty);
    sem_destroy(&shared_buffer.full);
    
    return 0;
}
```

### 4.3 Why This Works

**Synchronization Guarantees:**

```
1. Mutual Exclusion (mutex):
   - Only one thread modifies buffer at a time
   - Prevents race conditions

2. Condition Synchronization (semaphores):
   - empty: Prevents producer from overflowing
   - full: Prevents consumer from underflowing

3. Deadlock-Free:
   - Semaphores acquired before mutex
   - Consistent lock ordering
   - No circular wait
```

**Execution Timeline:**

```
Initial State:
Buffer: [empty] × 10
empty = 10, full = 0, mutex = unlocked

Producer:
  sem_wait(&empty)     → empty = 9 (success)
  mutex_lock()         → acquired
  buffer[0] = 'A'      → buffer: [A, empty × 9]
  mutex_unlock()       → released
  sem_post(&full)      → full = 1

Consumer (was blocked):
  sem_wait(&full)      → full = 0 (success, was waiting)
  mutex_lock()         → acquired
  item = buffer[0]     → 'A'
  mutex_unlock()       → released
  sem_post(&empty)     → empty = 10
```

---

## 5. Deadlock

### 5.1 The Traffic Jam of Computing

**CEO Analogy:** Four cars at a 4-way intersection. Each waits for the car on their right. Nobody moves—deadlock!

### 5.2 Four Necessary Conditions

**All must be true for deadlock:**

1. **Mutual Exclusion:** Resources cannot be shared
2. **Hold and Wait:** Thread holds resource while waiting for another
3. **No Preemption:** Resources cannot be forcibly taken
4. **Circular Wait:** Circular chain of threads waiting

### 5.3 Example: Deadlock

```c
pthread_mutex_t lock_a, lock_b;

// Thread 1
void* thread1(void* arg) {
    pthread_mutex_lock(&lock_a);  // Acquire A
    sleep(1);
    pthread_mutex_lock(&lock_b);  // Wait for B (Thread 2 has it!)
    
    // Critical section
    
    pthread_mutex_unlock(&lock_b);
    pthread_mutex_unlock(&lock_a);
}

// Thread 2
void* thread2(void* arg) {
    pthread_mutex_lock(&lock_b);  // Acquire B
    sleep(1);
    pthread_mutex_lock(&lock_a);  // Wait for A (Thread 1 has it!)
    
    // Critical section
    
    pthread_mutex_unlock(&lock_a);
    pthread_mutex_unlock(&lock_b);
}

// DEADLOCK: Thread 1 waits for Thread 2, Thread 2 waits for Thread 1
```

**Visualization:**

```
Thread 1: [Holds A] ──→ [Waits for B]
                            ↑
                            │
                            │ Circular Wait!
                            │
                            ↓
Thread 2: [Holds B] ──→ [Waits for A]
```

### 5.4 Deadlock Prevention

**Strategy 1: Lock Ordering**

```c
// ALWAYS acquire locks in same order!

// Thread 1
pthread_mutex_lock(&lock_a);  // A first
pthread_mutex_lock(&lock_b);  // B second
// ...
pthread_mutex_unlock(&lock_b);
pthread_mutex_unlock(&lock_a);

// Thread 2
pthread_mutex_lock(&lock_a);  // A first (same order!)
pthread_mutex_lock(&lock_b);  // B second
// ...
pthread_mutex_unlock(&lock_b);
pthread_mutex_unlock(&lock_a);

// No circular wait → No deadlock!
```

**Strategy 2: Try-Lock**

```c
retry:
pthread_mutex_lock(&lock_a);

if (pthread_mutex_trylock(&lock_b) != 0) {
    // Failed to acquire B, release A and retry
    pthread_mutex_unlock(&lock_a);
    usleep(rand() % 1000);  // Random backoff
    goto retry;
}

// Got both locks!
// Critical section
pthread_mutex_unlock(&lock_b);
pthread_mutex_unlock(&lock_a);
```

**Strategy 3: Timeout**

```c
struct timespec timeout;
clock_gettime(CLOCK_REALTIME, &timeout);
timeout.tv_sec += 5;  // 5-second timeout

pthread_mutex_lock(&lock_a);

if (pthread_mutex_timedlock(&lock_b, &timeout) != 0) {
    // Timeout! Release A and handle error
    pthread_mutex_unlock(&lock_a);
    return ERROR;
}

// Got both locks!
```

---

## 6. Memory Ordering and Barriers

### 6.1 The Reordering Problem

**Problem:** Modern CPUs reorder memory operations for performance.

```c
// Thread 1
data = 42;
flag = true;

// Thread 2
if (flag) {
    print(data);  // Might print garbage!
}
```

**Why?** CPU might reorder:
```c
// Thread 1 (reordered!)
flag = true;   // ← Moved before data write!
data = 42;
```

### 6.2 Memory Barriers

**Solution:** Use memory barriers to prevent reordering.

```c
// Thread 1
data = 42;
__sync_synchronize();  // Full memory barrier
flag = true;

// Thread 2
if (flag) {
    __sync_synchronize();  // Full memory barrier
    print(data);  // Now guaranteed to see 42
}
```

**x86-64 Memory Ordering (TSO):**

```
Strong Ordering (compared to ARM):
- Loads not reordered with loads
- Stores not reordered with stores
- Stores not reordered with prior loads
- Loads MAY be reordered with prior stores (only exception!)

LOCK prefix provides full barrier:
- No loads/stores cross the LOCK instruction
- Ensures sequential consistency
```

---

## Summary: Synchronization Essentials

### Must-Know Concepts
1. **Race condition:** Multiple threads accessing shared data unsafely
2. **Mutex:** Mutual exclusion lock, one thread at a time
3. **Semaphore:** Counting synchronization, N threads allowed
4. **Deadlock:** Circular wait, prevented by lock ordering
5. **Atomic operations:** LOCK CMPXCHG, cache coherency
6. **Memory barriers:** Prevent CPU reordering

### Performance Costs
- **Uncontended mutex:** 20-50 cycles
- **Contended mutex:** 1-10 microseconds
- **Context switch:** 1-10 microseconds
- **Cache miss:** 100-200 cycles

### Business Takeaways
- **Minimize synchronization:** Batch operations, reduce lock scope
- **Avoid false sharing:** Pad structures to 64-byte cache lines
- **Lock ordering:** Prevent deadlock with consistent ordering
- **Lock-free algorithms:** Use atomics for high-performance

### Next Steps
Continue to **Part 4: Real-World Applications** to apply these concepts to AWS infrastructure and performance optimization.
