/*
 * CS4440 Project 2 - Problem 1: Bounded-Buffer Producer/Consumer
 * 
 * HARDWARE-AWARE IMPLEMENTATION CONNECTING:
 * - POSIX pthread/semaphore primitives
 * - Unix kernel scheduling and futex system calls
 * - Intel Xeon Sapphire Rapids CPU architecture
 * 
 * END-TO-END EXECUTION FLOW:
 * User Code → POSIX Library → Kernel Syscall → Scheduler → CPU Hardware
 * 
 * INTEL XEON SAPPHIRE RAPIDS ARCHITECTURE FEATURES:
 * - Up to 60 cores per socket, 2-way SMT (120 hardware threads)
 * - 3-level cache hierarchy: L1 (48KB I + 32KB D per core), L2 (2MB per core), L3 (shared)
 * - MESI cache coherency protocol for multi-core synchronization
 * - Hardware lock elision (HLE) and restricted transactional memory (RTM)
 * - Memory ordering: x86-64 Total Store Order (TSO) model
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

/* 
 * BUFFER CONFIGURATION
 * Buffer size affects cache behavior on Intel Xeon:
 * - Small buffer (< 32KB) fits in L1 data cache
 * - Medium buffer (< 2MB) fits in L2 cache
 * - Large buffer requires L3 or main memory access
 */
#define BUFFER_SIZE 10
#define NUM_ITEMS 50

/*
 * SHARED BUFFER STRUCTURE
 * 
 * CACHE LINE CONSIDERATIONS (Intel Xeon: 64 bytes):
 * - False sharing occurs when multiple threads access different variables
 *   in the same cache line, causing unnecessary cache coherency traffic
 * - Solution: Pad critical variables to separate cache lines
 */
typedef struct {
    char buffer[BUFFER_SIZE];           // Circular buffer for characters
    int in;                              // Producer index
    int out;                             // Consumer index
    int count;                           // Current item count
    
    /* 
     * SYNCHRONIZATION PRIMITIVES
     * 
     * pthread_mutex_t HARDWARE IMPLEMENTATION:
     * 1. Uncontended case (fast path):
     *    - Uses atomic compare-and-swap (CMPXCHG on x86-64)
     *    - CPU executes LOCK prefix instruction
     *    - Cache line containing mutex enters "Exclusive" state (MESI)
     *    - No kernel involvement - pure userspace operation
     * 
     * 2. Contended case (slow path):
     *    - Falls back to futex() system call
     *    - Kernel puts thread on wait queue
     *    - CPU scheduler removes thread from runnable state
     *    - Context switch saves thread state to memory
     * 
     * MEMORY BARRIERS:
     * - LOCK prefix provides full memory barrier
     * - Ensures all prior memory operations complete before lock acquisition
     * - Prevents CPU reordering across critical section boundaries
     */
    pthread_mutex_t mutex;
    
    /*
     * sem_t HARDWARE IMPLEMENTATION:
     * 
     * POSIX semaphores use atomic operations + futex:
     * 1. sem_wait() (P operation):
     *    - Atomically decrements counter using LOCK DEC
     *    - If result >= 0: continues (fast path)
     *    - If result < 0: futex() syscall blocks thread
     * 
     * 2. sem_post() (V operation):
     *    - Atomically increments counter using LOCK INC
     *    - If waiters exist: futex() syscall wakes one thread
     * 
     * CPU CACHE COHERENCY:
     * - Semaphore variable resides in cache line
     * - LOCK prefix triggers cache line invalidation on other cores
     * - MESI protocol ensures coherent view across all cores
     */
    sem_t empty;    // Counts empty slots (initialized to BUFFER_SIZE)
    sem_t full;     // Counts full slots (initialized to 0)
} bounded_buffer_t;

bounded_buffer_t shared_buffer;

/*
 * PRODUCER THREAD FUNCTION
 * 
 * THREAD EXECUTION ON INTEL XEON:
 * 1. OS scheduler assigns thread to CPU core
 * 2. Thread state loaded from memory to CPU registers
 * 3. Instruction fetch begins from L1 instruction cache
 * 4. Branch predictor speculates on conditional branches
 * 5. Out-of-order execution engine schedules independent instructions
 */
void* producer(void* arg) {
    int id = *(int*)arg;
    
    for (int i = 0; i < NUM_ITEMS; i++) {
        // Generate printable character (A-Z)
        char item = 'A' + (rand() % 26);
        
        /*
         * SEMAPHORE WAIT (P operation) - DETAILED HARDWARE FLOW:
         * 
         * sem_wait(&shared_buffer.empty):
         * 
         * 1. USERSPACE (glibc):
         *    - Loads semaphore value from memory (may hit L1/L2/L3 cache)
         *    - Executes atomic decrement: LOCK DEC [semaphore_address]
         * 
         * 2. CPU HARDWARE (Intel Xeon):
         *    - LOCK prefix asserts bus lock or cache lock
         *    - For cached data: cache line enters "Modified" state (MESI)
         *    - Cache coherency protocol invalidates line on other cores
         *    - Atomic operation completes, result stored in cache
         * 
         * 3. KERNEL SYSCALL (if blocked):
         *    - If semaphore < 0: glibc calls futex(FUTEX_WAIT)
         *    - CPU switches to kernel mode (privilege level 0)
         *    - Kernel scheduler adds thread to futex wait queue
         *    - Context switch: save registers, load new thread
         *    - TLB (Translation Lookaside Buffer) flushed for address space change
         * 
         * 4. CPU SCHEDULING:
         *    - Scheduler picks next runnable thread
         *    - May migrate thread to different core (NUMA considerations)
         *    - Thread state restored from memory to CPU registers
         */
        sem_wait(&shared_buffer.empty);
        
        /*
         * MUTEX LOCK - HARDWARE EXECUTION:
         * 
         * pthread_mutex_lock(&shared_buffer.mutex):
         * 
         * FAST PATH (uncontended):
         * 1. Atomic compare-and-swap (CAS):
         *    LOCK CMPXCHG [mutex_address], new_value
         *    - Compares mutex value with 0 (unlocked)
         *    - If equal: sets to thread ID (locked)
         *    - If not equal: lock failed, go to slow path
         * 
         * 2. CACHE COHERENCY:
         *    - Cache line containing mutex enters "Exclusive" state
         *    - Other cores' copies invalidated (MESI "Invalid" state)
         *    - Subsequent accesses by other cores cause cache miss
         * 
         * 3. MEMORY ORDERING:
         *    - LOCK prefix acts as full memory barrier
         *    - All prior loads/stores complete before lock acquisition
         *    - Prevents CPU from reordering memory operations
         * 
         * SLOW PATH (contended):
         * 1. Spin briefly (adaptive mutex)
         * 2. futex(FUTEX_LOCK_PI) system call
         * 3. Kernel priority inheritance to prevent priority inversion
         */
        pthread_mutex_lock(&shared_buffer.mutex);
        
        /*
         * CRITICAL SECTION - CACHE BEHAVIOR:
         * 
         * Buffer access patterns:
         * - Sequential writes maintain spatial locality
         * - Circular buffer reuses same cache lines (temporal locality)
         * - On Intel Xeon: hardware prefetcher predicts sequential access
         * 
         * CACHE LINE STATES (MESI Protocol):
         * - Modified (M): This core has exclusive, modified copy
         * - Exclusive (E): This core has exclusive, clean copy
         * - Shared (S): Multiple cores have clean copies
         * - Invalid (I): Cache line is invalid
         */
        shared_buffer.buffer[shared_buffer.in] = item;
        printf("[Producer %d] Produced: %c at position %d (count: %d)\n", 
               id, item, shared_buffer.in, shared_buffer.count + 1);
        
        // Update producer index (circular buffer)
        shared_buffer.in = (shared_buffer.in + 1) % BUFFER_SIZE;
        shared_buffer.count++;
        
        /*
         * MUTEX UNLOCK - HARDWARE EXECUTION:
         * 
         * pthread_mutex_unlock(&shared_buffer.mutex):
         * 
         * 1. MEMORY BARRIER:
         *    - Ensures all stores in critical section complete
         *    - x86-64 TSO model: stores are not reordered with stores
         *    - But compiler barriers prevent compiler reordering
         * 
         * 2. ATOMIC STORE:
         *    - Sets mutex to 0 (unlocked) using atomic operation
         *    - LOCK prefix or XCHG instruction (implicitly locked)
         * 
         * 3. CACHE COHERENCY:
         *    - Cache line transitions to "Shared" or "Invalid" state
         *    - Other cores can now acquire the cache line
         * 
         * 4. WAKEUP (if waiters):
         *    - futex(FUTEX_UNLOCK_PI) wakes one waiting thread
         *    - Kernel moves thread from wait queue to runnable queue
         *    - Scheduler may immediately schedule woken thread
         */
        pthread_mutex_unlock(&shared_buffer.mutex);
        
        /*
         * SEMAPHORE POST (V operation) - HARDWARE FLOW:
         * 
         * sem_post(&shared_buffer.full):
         * 
         * 1. ATOMIC INCREMENT:
         *    LOCK INC [semaphore_address]
         *    - Increments semaphore value atomically
         *    - Cache coherency ensures visibility to all cores
         * 
         * 2. WAKEUP (if waiters):
         *    - If threads waiting: futex(FUTEX_WAKE)
         *    - Kernel wakes one waiting thread
         *    - Woken thread becomes runnable
         * 
         * 3. CPU SCHEDULING:
         *    - Scheduler may preempt current thread
         *    - Higher priority thread may run immediately
         *    - Context switch overhead: ~1-10 microseconds on modern CPUs
         */
        sem_post(&shared_buffer.full);
        
        /*
         * SLEEP SIMULATION:
         * usleep(100) demonstrates:
         * - Voluntary context switch
         * - Thread enters sleep state
         * - CPU can execute other threads
         * - Timer interrupt wakes thread after 100 microseconds
         */
        usleep(100);  // Simulate production time
    }
    
    return NULL;
}

/*
 * CONSUMER THREAD FUNCTION
 * 
 * SYMMETRIC EXECUTION TO PRODUCER:
 * - Same synchronization primitives
 * - Same hardware execution flow
 * - Demonstrates concurrent access to shared buffer
 */
void* consumer(void* arg) {
    int id = *(int*)arg;
    
    for (int i = 0; i < NUM_ITEMS; i++) {
        // Wait for full slot (blocks if buffer empty)
        sem_wait(&shared_buffer.full);
        
        // Acquire mutex for exclusive access
        pthread_mutex_lock(&shared_buffer.mutex);
        
        // Remove item from buffer
        char item = shared_buffer.buffer[shared_buffer.out];
        printf("[Consumer %d] Consumed: %c from position %d (count: %d)\n", 
               id, item, shared_buffer.out, shared_buffer.count - 1);
        
        // Update consumer index (circular buffer)
        shared_buffer.out = (shared_buffer.out + 1) % BUFFER_SIZE;
        shared_buffer.count--;
        
        // Release mutex
        pthread_mutex_unlock(&shared_buffer.mutex);
        
        // Signal empty slot available
        sem_post(&shared_buffer.empty);
        
        usleep(100);  // Simulate consumption time
    }
    
    return NULL;
}

/*
 * MAIN FUNCTION - INITIALIZATION AND THREAD MANAGEMENT
 * 
 * PROCESS STARTUP ON INTEL XEON:
 * 1. Kernel loads executable into memory
 * 2. Dynamic linker resolves pthread library symbols
 * 3. Main thread starts on assigned CPU core
 * 4. Virtual memory pages allocated for stack, heap, data
 * 5. TLB entries created for virtual-to-physical address translation
 */
int main(void) {
    pthread_t prod_thread, cons_thread;
    int prod_id = 1, cons_id = 1;
    
    // Seed random number generator
    srand(time(NULL));
    
    /*
     * INITIALIZATION OF SYNCHRONIZATION PRIMITIVES
     * 
     * Memory allocation:
     * - Static global variables reside in data segment
     * - Shared across all threads in process
     * - Virtual memory maps to physical RAM pages
     * 
     * NUMA considerations on multi-socket Xeon:
     * - Memory allocated on local NUMA node for best performance
     * - Cross-NUMA access has higher latency (~2x)
     * - Use numa_alloc_local() for NUMA-aware allocation
     */
    shared_buffer.in = 0;
    shared_buffer.out = 0;
    shared_buffer.count = 0;
    
    /*
     * pthread_mutex_init() - KERNEL INITIALIZATION:
     * 
     * 1. Allocates futex data structure in kernel
     * 2. Initializes mutex state to 0 (unlocked)
     * 3. Sets mutex attributes (type, protocol, etc.)
     * 
     * PTHREAD_MUTEX_DEFAULT:
     * - Non-recursive (deadlock if same thread locks twice)
     * - No priority inheritance (can cause priority inversion)
     * - Fast userspace locking when uncontended
     */
    if (pthread_mutex_init(&shared_buffer.mutex, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        return EXIT_FAILURE;
    }
    
    /*
     * sem_init() - SEMAPHORE INITIALIZATION:
     * 
     * Parameters:
     * - sem: pointer to semaphore
     * - pshared: 0 = shared between threads, 1 = shared between processes
     * - value: initial count
     * 
     * Implementation:
     * - Stores count in semaphore structure
     * - Initializes futex for blocking operations
     * - No kernel allocation for thread-shared semaphores
     */
    if (sem_init(&shared_buffer.empty, 0, BUFFER_SIZE) != 0) {
        fprintf(stderr, "Empty semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&shared_buffer.full, 0, 0) != 0) {
        fprintf(stderr, "Full semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    printf("=== Bounded-Buffer Producer/Consumer ===\n");
    printf("Buffer Size: %d\n", BUFFER_SIZE);
    printf("Items to produce/consume: %d\n\n", NUM_ITEMS);
    
    /*
     * pthread_create() - THREAD CREATION:
     * 
     * KERNEL OPERATIONS:
     * 1. Allocates new task_struct in kernel
     * 2. Clones parent's address space (shares memory)
     * 3. Allocates new stack (typically 8MB on Linux)
     * 4. Sets up thread-local storage (TLS)
     * 5. Adds thread to scheduler's runnable queue
     * 
     * CPU SCHEDULING:
     * - Linux CFS (Completely Fair Scheduler) assigns CPU time
     * - Each thread gets fair share of CPU cycles
     * - Priority and nice values affect scheduling weight
     * 
     * INTEL XEON THREAD PLACEMENT:
     * - Scheduler considers CPU topology
     * - Prefers placing threads on same socket (shared L3 cache)
     * - Avoids cross-NUMA memory access when possible
     * - SMT (Simultaneous Multithreading): 2 threads per core
     */
    if (pthread_create(&prod_thread, NULL, producer, &prod_id) != 0) {
        fprintf(stderr, "Producer thread creation failed\n");
        return EXIT_FAILURE;
    }
    
    if (pthread_create(&cons_thread, NULL, consumer, &cons_id) != 0) {
        fprintf(stderr, "Consumer thread creation failed\n");
        return EXIT_FAILURE;
    }
    
    /*
     * pthread_join() - THREAD SYNCHRONIZATION:
     * 
     * BLOCKING BEHAVIOR:
     * 1. Main thread calls futex(FUTEX_WAIT) if child not finished
     * 2. Kernel blocks main thread on futex
     * 3. When child exits, kernel calls futex(FUTEX_WAKE)
     * 4. Main thread wakes up and retrieves child's exit status
     * 
     * CONTEXT SWITCHING:
     * - Main thread yields CPU voluntarily
     * - Scheduler runs other threads
     * - When child exits, main thread becomes runnable
     * - Scheduler eventually schedules main thread
     */
    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);
    
    printf("\n=== Execution Complete ===\n");
    printf("Final buffer count: %d (should be 0)\n", shared_buffer.count);
    
    /*
     * CLEANUP - RESOURCE DEALLOCATION:
     * 
     * pthread_mutex_destroy():
     * - Releases kernel futex resources
     * - Marks mutex as invalid
     * 
     * sem_destroy():
     * - Releases semaphore resources
     * - Wakes any waiting threads (with error)
     */
    pthread_mutex_destroy(&shared_buffer.mutex);
    sem_destroy(&shared_buffer.empty);
    sem_destroy(&shared_buffer.full);
    
    return EXIT_SUCCESS;
}

/*
 * PERFORMANCE ANALYSIS ON INTEL XEON SAPPHIRE RAPIDS:
 * 
 * 1. CACHE EFFICIENCY:
 *    - Small buffer fits in L1 cache (32KB data cache per core)
 *    - Circular buffer reuses cache lines (good temporal locality)
 *    - Sequential access enables hardware prefetching
 * 
 * 2. SYNCHRONIZATION OVERHEAD:
 *    - Uncontended mutex: ~20-50 CPU cycles (fast path)
 *    - Contended mutex: ~1-10 microseconds (kernel syscall)
 *    - Semaphore operations: similar to mutex
 * 
 * 3. CONTEXT SWITCHING:
 *    - Direct cost: ~1-10 microseconds (save/restore registers)
 *    - Indirect cost: cache pollution, TLB misses
 *    - On Xeon: large L3 cache reduces context switch penalty
 * 
 * 4. SCALABILITY:
 *    - Single producer/consumer: minimal contention
 *    - Multiple producers/consumers: contention increases
 *    - Lock-free algorithms can improve scalability
 * 
 * 5. NUMA EFFECTS (multi-socket Xeon):
 *    - Local memory access: ~100ns latency
 *    - Remote memory access: ~200ns latency
 *    - Use numa_alloc_onnode() for NUMA-aware allocation
 * 
 * OPTIMIZATION OPPORTUNITIES:
 * - Use lock-free ring buffer (atomic operations only)
 * - Batch operations to reduce synchronization frequency
 * - Align buffer to cache line boundaries (64 bytes)
 * - Pin threads to specific cores (CPU affinity)
 * - Use huge pages (2MB) for large buffers
 */
