/*
 * CS4440 Project 2 - Problem 2: Mother Hubbard Synchronization
 * 
 * HARDWARE-AWARE CONCURRENCY: THREAD COORDINATION ON INTEL XEON
 * 
 * This implementation demonstrates:
 * - Complex thread synchronization using mutex and semaphores
 * - CPU scheduling and thread state transitions
 * - Cache coherency in multi-threaded workloads
 * - Memory barriers and atomic operations
 * 
 * SYNCHRONIZATION PATTERN:
 * - Mother thread performs tasks 1-5 on all children sequentially
 * - After each child's bath (task 5), Father can process that child
 * - Father performs tasks 6-7 (read book, tuck in bed) on each child
 * - When all children in bed, Father sleeps and wakes Mother
 * - Cycle repeats N times
 * 
 * INTEL XEON SAPPHIRE RAPIDS EXECUTION MODEL:
 * - 2 threads (Mother, Father) compete for CPU time
 * - Scheduler assigns threads to cores based on availability
 * - Synchronization primitives coordinate thread execution
 * - Cache coherency ensures consistent view of shared state
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_CHILDREN 12

/*
 * CHILD STATE TRACKING
 * 
 * CACHE LINE CONSIDERATIONS:
 * - Each child's state is a single integer (4 bytes)
 * - Array of 12 children = 48 bytes (fits in single cache line)
 * - False sharing potential: Mother and Father accessing same cache line
 * 
 * SOLUTION: Pad each child state to separate cache lines
 * - Intel Xeon cache line: 64 bytes
 * - Padding prevents false sharing between Mother and Father threads
 */
typedef struct {
    int task_completed;  // Tracks which task is completed (0-7)
    char padding[60];    // Pad to 64 bytes to prevent false sharing
} child_state_t;

child_state_t children[NUM_CHILDREN];

/*
 * SYNCHRONIZATION PRIMITIVES
 * 
 * HARDWARE MAPPING ON INTEL XEON:
 * 
 * pthread_mutex_t:
 * - Protects access to Mother's state
 * - Uses futex (fast userspace mutex) in Linux
 * - Uncontended: atomic CAS in userspace (~20 cycles)
 * - Contended: kernel syscall + context switch (~1-10 μs)
 * 
 * sem_t:
 * - Binary semaphore for thread coordination
 * - Mother semaphore: 1 = awake, 0 = sleeping
 * - Father semaphore: 1 = awake, 0 = sleeping
 * - Uses atomic operations + futex for blocking
 */
pthread_mutex_t mother_mutex;
sem_t mother_sem;  // Controls Mother's sleep/wake cycle
sem_t father_sem;  // Controls Father's sleep/wake cycle

// Shared state for tracking progress
int current_cycle = 0;
int children_bathed = 0;  // Tracks how many children Father can process

/*
 * MOTHER THREAD FUNCTION
 * 
 * THREAD EXECUTION FLOW ON INTEL XEON:
 * 
 * 1. THREAD SCHEDULING:
 *    - Linux CFS scheduler assigns Mother thread to CPU core
 *    - Thread state: RUNNING (actively executing on CPU)
 *    - CPU fetches instructions from L1 instruction cache
 * 
 * 2. INSTRUCTION PIPELINE:
 *    - Intel Xeon: 14-stage pipeline (Sapphire Rapids)
 *    - Out-of-order execution: CPU reorders independent instructions
 *    - Branch prediction: Predicts loop iterations and conditionals
 * 
 * 3. MEMORY ACCESS:
 *    - Child state array accessed sequentially
 *    - Hardware prefetcher loads next cache lines
 *    - L1 data cache hit: ~4 cycles latency
 *    - L2 cache hit: ~12 cycles latency
 *    - L3 cache hit: ~40 cycles latency
 *    - RAM access: ~100-200 cycles latency
 */
void* mother_thread(void* arg) {
    int num_cycles = *(int*)arg;
    
    for (int cycle = 0; cycle < num_cycles; cycle++) {
        /*
         * SEMAPHORE WAIT - BLOCKING BEHAVIOR:
         * 
         * sem_wait(&mother_sem):
         * 
         * 1. ATOMIC DECREMENT:
         *    - CPU executes: LOCK DEC [mother_sem]
         *    - Cache line containing semaphore enters "Modified" state
         *    - MESI protocol invalidates cache line on other cores
         * 
         * 2. CONDITIONAL BLOCK:
         *    - If semaphore >= 0: continue (fast path)
         *    - If semaphore < 0: futex(FUTEX_WAIT) syscall
         * 
         * 3. KERNEL BLOCKING:
         *    - Thread state: RUNNING → INTERRUPTIBLE_SLEEP
         *    - Thread removed from CPU runqueue
         *    - Context switch: save registers to memory
         *    - Scheduler picks next runnable thread
         * 
         * 4. CPU CONTEXT SWITCH:
         *    - Save: RIP, RSP, RBP, general-purpose registers
         *    - Save: FPU state, SSE/AVX registers (if used)
         *    - Load: Next thread's register state
         *    - TLB flush: Clear address translation cache
         *    - Cache pollution: New thread's data evicts old cache lines
         */
        sem_wait(&mother_sem);
        
        printf("\n=== Day #%d: Mother wakes up ===\n", cycle + 1);
        
        /*
         * TASK LOOP - SEQUENTIAL PROCESSING:
         * 
         * CACHE BEHAVIOR:
         * - Sequential access to children array
         * - Good spatial locality: adjacent children in same cache line
         * - Temporal locality: same children accessed in each task
         * - Hardware prefetcher: predicts sequential pattern
         */
        
        // Task 1: Wake up and feed breakfast
        for (int i = 0; i < NUM_CHILDREN; i++) {
            printf("Child #%d is being woken up and fed breakfast.\n", i + 1);
            children[i].task_completed = 1;
            usleep(100);  // Simulate task time
        }
        
        // Task 2: Send to school
        for (int i = 0; i < NUM_CHILDREN; i++) {
            printf("Child #%d is being sent to school.\n", i + 1);
            children[i].task_completed = 2;
            usleep(100);
        }
        
        // Task 3: Give dinner
        for (int i = 0; i < NUM_CHILDREN; i++) {
            printf("Child #%d is being given dinner.\n", i + 1);
            children[i].task_completed = 3;
            usleep(100);
        }
        
        // Task 4: Give bath (after this, Father can process each child)
        for (int i = 0; i < NUM_CHILDREN; i++) {
            printf("Child #%d is being given a bath.\n", i + 1);
            children[i].task_completed = 4;
            
            /*
             * SEMAPHORE POST - WAKEUP MECHANISM:
             * 
             * sem_post(&father_sem):
             * 
             * 1. ATOMIC INCREMENT:
             *    - CPU executes: LOCK INC [father_sem]
             *    - Cache coherency: invalidate cache line on other cores
             * 
             * 2. WAKEUP (if Father is blocked):
             *    - futex(FUTEX_WAKE, 1) syscall
             *    - Kernel moves Father thread: SLEEP → RUNNABLE
             *    - Thread added to CPU runqueue
             * 
             * 3. SCHEDULER DECISION:
             *    - CFS scheduler recalculates thread priorities
             *    - May preempt Mother if Father has higher priority
             *    - Or Father waits for next scheduling quantum
             * 
             * 4. CACHE EFFECTS:
             *    - Father thread's cache lines may be cold (evicted)
             *    - Context switch causes cache misses
             *    - Working set reload: ~1000s of cycles
             */
            sem_post(&father_sem);
            
            usleep(100);
        }
        
        printf("Mother has completed all tasks and is taking a nap.\n");
        
        /*
         * MOTHER BLOCKS - VOLUNTARY CONTEXT SWITCH:
         * 
         * Thread state transition:
         * RUNNING → INTERRUPTIBLE_SLEEP
         * 
         * CPU EFFECTS:
         * - Mother's cache lines remain in L3 (if not evicted)
         * - TLB entries for Mother's stack may be flushed
         * - CPU can execute Father thread or other processes
         */
    }
    
    return NULL;
}

/*
 * FATHER THREAD FUNCTION
 * 
 * CONCURRENT EXECUTION WITH MOTHER:
 * - Father processes children after Mother gives them baths
 * - Demonstrates producer-consumer pattern with dependencies
 * - Synchronization ensures correct ordering of operations
 */
void* father_thread(void* arg) {
    int num_cycles = *(int*)arg;
    
    for (int cycle = 0; cycle < num_cycles; cycle++) {
        /*
         * PROCESS EACH CHILD AFTER BATH:
         * 
         * SYNCHRONIZATION INVARIANT:
         * - Father can only process child #i after Mother bathes child #i
         * - Semaphore ensures this ordering
         * 
         * CACHE COHERENCY:
         * - Father reads children[i].task_completed
         * - Mother writes children[i].task_completed
         * - MESI protocol ensures Father sees Mother's writes
         * 
         * MEMORY ORDERING (x86-64 TSO):
         * - Stores are not reordered with other stores
         * - Loads may be reordered with older stores to different locations
         * - LOCK prefix provides full memory barrier
         * - Semaphore operations include memory barriers
         */
        for (int i = 0; i < NUM_CHILDREN; i++) {
            // Wait for Mother to bathe this child
            sem_wait(&father_sem);
            
            /*
             * CRITICAL SECTION - CHILD PROCESSING:
             * 
             * CACHE LINE ACCESS:
             * - Father reads/writes children[i]
             * - If Mother recently accessed: cache line in "Shared" state
             * - Father's write: cache line → "Modified" state
             * - Mother's cache line invalidated (MESI protocol)
             * 
             * FALSE SHARING MITIGATION:
             * - Each child_state_t padded to 64 bytes
             * - Prevents Mother and Father from sharing cache lines
             * - Without padding: ping-pong effect between cores
             */
            
            // Task 5: Read a book
            printf("Child #%d is being read a book by Father.\n", i + 1);
            children[i].task_completed = 5;
            usleep(100);
            
            // Task 6: Tuck in bed
            printf("Child #%d is being tucked in bed by Father.\n", i + 1);
            children[i].task_completed = 6;
            usleep(100);
        }
        
        printf("Father has tucked all children in bed and is going to sleep.\n");
        printf("Father wakes up Mother for the next day.\n");
        
        /*
         * WAKE MOTHER FOR NEXT CYCLE:
         * 
         * sem_post(&mother_sem):
         * - Atomic increment of semaphore
         * - If Mother is blocked: futex wakes Mother thread
         * - Mother transitions: SLEEP → RUNNABLE
         * - Scheduler will eventually run Mother
         */
        sem_post(&mother_sem);
    }
    
    return NULL;
}

/*
 * MAIN FUNCTION - PROGRAM INITIALIZATION
 * 
 * PROCESS STARTUP ON INTEL XEON:
 * 
 * 1. KERNEL LOADS EXECUTABLE:
 *    - ELF binary loaded into virtual memory
 *    - Code segment: read-only, executable
 *    - Data segment: read-write, non-executable
 *    - Stack segment: read-write, grows downward
 * 
 * 2. DYNAMIC LINKING:
 *    - ld-linux.so resolves pthread library symbols
 *    - libpthread.so mapped into address space
 *    - PLT (Procedure Linkage Table) for lazy binding
 * 
 * 3. MAIN THREAD STARTS:
 *    - Kernel assigns main thread to CPU core
 *    - Thread state: RUNNING
 *    - Instruction pointer: main() entry point
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_cycles>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int num_cycles = atoi(argv[1]);
    if (num_cycles <= 0) {
        fprintf(stderr, "Error: Number of cycles must be positive\n");
        return EXIT_FAILURE;
    }
    
    pthread_t mother, father;
    
    /*
     * INITIALIZE SYNCHRONIZATION PRIMITIVES:
     * 
     * pthread_mutex_init():
     * - Allocates mutex in process memory
     * - Initializes futex for kernel-based blocking
     * - Sets mutex state to unlocked (0)
     * 
     * sem_init():
     * - Initializes semaphore counter
     * - mother_sem = 1 (Mother starts awake)
     * - father_sem = 0 (Father starts asleep)
     * - pshared = 0 (shared between threads, not processes)
     */
    if (pthread_mutex_init(&mother_mutex, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&mother_sem, 0, 1) != 0) {  // Mother starts awake
        fprintf(stderr, "Mother semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&father_sem, 0, 0) != 0) {  // Father starts asleep
        fprintf(stderr, "Father semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    // Initialize children state
    for (int i = 0; i < NUM_CHILDREN; i++) {
        children[i].task_completed = 0;
    }
    
    printf("=== Mother Hubbard Simulation ===\n");
    printf("Number of cycles: %d\n", num_cycles);
    printf("Number of children: %d\n\n", NUM_CHILDREN);
    
    /*
     * CREATE THREADS:
     * 
     * pthread_create() KERNEL OPERATIONS:
     * 
     * 1. CLONE SYSTEM CALL:
     *    - Kernel creates new task_struct
     *    - Shares address space with parent (CLONE_VM)
     *    - Shares file descriptors (CLONE_FILES)
     *    - Shares signal handlers (CLONE_SIGHAND)
     * 
     * 2. STACK ALLOCATION:
     *    - Allocates new stack (default: 8MB on Linux)
     *    - Stack guard page prevents overflow
     *    - Stack grows downward from high address
     * 
     * 3. THREAD LOCAL STORAGE (TLS):
     *    - Allocates TLS block for thread-specific data
     *    - FS register (x86-64) points to TLS
     *    - errno, thread ID stored in TLS
     * 
     * 4. SCHEDULER INTEGRATION:
     *    - Thread added to runqueue
     *    - Initial priority set based on parent
     *    - CFS scheduler assigns vruntime
     * 
     * 5. CPU AFFINITY:
     *    - By default: thread can run on any CPU
     *    - Can set affinity: pthread_setaffinity_np()
     *    - NUMA-aware: prefer local memory node
     */
    if (pthread_create(&mother, NULL, mother_thread, &num_cycles) != 0) {
        fprintf(stderr, "Mother thread creation failed\n");
        return EXIT_FAILURE;
    }
    
    if (pthread_create(&father, NULL, father_thread, &num_cycles) != 0) {
        fprintf(stderr, "Father thread creation failed\n");
        return EXIT_FAILURE;
    }
    
    /*
     * WAIT FOR THREADS TO COMPLETE:
     * 
     * pthread_join() BLOCKING MECHANISM:
     * 
     * 1. FUTEX WAIT:
     *    - Main thread calls futex(FUTEX_WAIT)
     *    - Kernel blocks main thread
     *    - Thread state: INTERRUPTIBLE_SLEEP
     * 
     * 2. THREAD EXIT:
     *    - When Mother/Father exits: calls exit_thread()
     *    - Kernel stores exit status
     *    - Calls futex(FUTEX_WAKE) to wake joiner
     * 
     * 3. MAIN THREAD WAKEUP:
     *    - Main thread: SLEEP → RUNNABLE
     *    - Scheduler eventually runs main thread
     *    - Main thread retrieves exit status
     * 
     * 4. RESOURCE CLEANUP:
     *    - Thread stack deallocated
     *    - TLS freed
     *    - task_struct removed from kernel
     */
    pthread_join(mother, NULL);
    pthread_join(father, NULL);
    
    printf("\n=== Simulation Complete ===\n");
    printf("All %d cycles completed successfully.\n", num_cycles);
    
    /*
     * CLEANUP SYNCHRONIZATION PRIMITIVES:
     * 
     * pthread_mutex_destroy():
     * - Marks mutex as invalid
     * - Releases kernel futex resources
     * - Subsequent use causes undefined behavior
     * 
     * sem_destroy():
     * - Releases semaphore resources
     * - Wakes any waiting threads (with error)
     */
    pthread_mutex_destroy(&mother_mutex);
    sem_destroy(&mother_sem);
    sem_destroy(&father_sem);
    
    return EXIT_SUCCESS;
}

/*
 * PERFORMANCE ANALYSIS ON INTEL XEON SAPPHIRE RAPIDS:
 * 
 * 1. SYNCHRONIZATION OVERHEAD:
 *    - Each sem_wait/sem_post: ~20-50 cycles (uncontended)
 *    - Context switch: ~1-10 microseconds (1000-10000 cycles)
 *    - Total synchronization: 12 * 2 = 24 semaphore operations per cycle
 * 
 * 2. CACHE EFFECTS:
 *    - Children array: 12 * 64 = 768 bytes
 *    - Fits in L1 cache (32KB data cache per core)
 *    - False sharing prevented by padding
 *    - Cache coherency traffic: minimal with padding
 * 
 * 3. THREAD SCHEDULING:
 *    - 2 threads on multi-core Xeon: minimal contention
 *    - Threads can run on separate cores simultaneously
 *    - Synchronization enforces sequential ordering
 * 
 * 4. MEMORY ORDERING:
 *    - x86-64 TSO (Total Store Order) model
 *    - Stores visible to all cores in program order
 *    - LOCK prefix provides full memory barrier
 *    - Semaphore operations include barriers
 * 
 * 5. SCALABILITY:
 *    - Current design: inherently sequential
 *    - Optimization: parallelize independent tasks
 *    - Example: Mother could batch children in groups
 *    - Trade-off: complexity vs. performance
 * 
 * HARDWARE-AWARE OPTIMIZATIONS:
 * 
 * 1. CACHE LINE PADDING:
 *    - Prevents false sharing between threads
 *    - Each child state in separate cache line
 *    - Cost: 12 * 64 = 768 bytes (minimal)
 * 
 * 2. NUMA AWARENESS:
 *    - Pin Mother and Father to same NUMA node
 *    - Allocate children array on local node
 *    - Reduces cross-NUMA memory latency
 * 
 * 3. CPU AFFINITY:
 *    - Pin threads to specific cores
 *    - Reduces cache thrashing from migration
 *    - Improves cache locality
 * 
 * 4. LOCK-FREE ALTERNATIVES:
 *    - Use atomic operations instead of semaphores
 *    - Reduces kernel involvement
 *    - Example: atomic counters for progress tracking
 * 
 * END-TO-END EXECUTION FLOW SUMMARY:
 * 
 * User Code:
 *   sem_wait(&mother_sem)
 *     ↓
 * POSIX Library (glibc):
 *   LOCK DEC [mother_sem]
 *   if (result < 0) syscall(SYS_futex, FUTEX_WAIT, ...)
 *     ↓
 * Linux Kernel:
 *   futex_wait() → add to wait queue
 *   schedule() → pick next thread
 *   context_switch() → save/restore registers
 *     ↓
 * Intel Xeon CPU:
 *   LOCK prefix → cache line locking
 *   MESI protocol → cache coherency
 *   Context switch → TLB flush, cache pollution
 *   Scheduler → assign thread to core
 *     ↓
 * Hardware Execution:
 *   Instruction fetch from L1 I-cache
 *   Decode and dispatch to execution units
 *   Out-of-order execution
 *   Memory access through cache hierarchy
 *   Retire instructions and update architectural state
 */
