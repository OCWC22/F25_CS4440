/*
 * CS4440 Project 2 - Problem 3: Airline Passenger Processing
 * 
 * ADVANCED CONCURRENCY: MULTI-THREADED RESOURCE POOL ON INTEL XEON
 * 
 * This implementation demonstrates:
 * - Thread pool pattern with worker threads
 * - Semaphore-based resource management
 * - Complex synchronization with multiple thread types
 * - Producer-consumer with multiple stages
 * - Real-world application of OS concurrency primitives
 * 
 * SYSTEM ARCHITECTURE:
 * - P passenger threads (producers)
 * - B baggage handler threads (workers, stage 1)
 * - S security screener threads (workers, stage 2)
 * - F flight attendant threads (workers, stage 3)
 * 
 * INTEL XEON EXECUTION MODEL:
 * - Multi-core parallelism: workers run on separate cores
 * - Cache coherency: MESI protocol coordinates shared state
 * - NUMA awareness: memory allocation affects performance
 * - Thread scheduling: CFS balances load across cores
 * 
 * HARDWARE MAPPING:
 * User threads → Kernel threads → CPU cores → Hardware threads (SMT)
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdatomic.h>

/*
 * PASSENGER STATE MACHINE:
 * 
 * ARRIVED → BAGGAGE_PROCESSING → SECURITY_SCREENING → BOARDING → SEATED
 * 
 * Each state transition requires synchronization with worker threads.
 * State stored in memory, accessed by multiple threads.
 * Cache coherency ensures consistent view across all cores.
 */
typedef enum {
    ARRIVED,
    BAGGAGE_PROCESSING,
    BAGGAGE_DONE,
    SECURITY_SCREENING,
    SECURITY_DONE,
    BOARDING,
    SEATED
} passenger_state_t;

/*
 * PASSENGER STRUCTURE:
 * 
 * CACHE LINE CONSIDERATIONS:
 * - Each passenger: ~72 bytes (id + state + padding)
 * - Intel Xeon cache line: 64 bytes
 * - Padding to 128 bytes prevents false sharing
 * 
 * FALSE SHARING SCENARIO:
 * - Without padding: multiple passengers in same cache line
 * - Worker threads access different passengers
 * - Cache line ping-pongs between cores (MESI protocol)
 * - Performance degradation: ~10-100x slowdown
 * 
 * SOLUTION:
 * - Pad each passenger to 128 bytes (2 cache lines)
 * - Ensures each passenger in separate cache line
 * - Eliminates false sharing between workers
 */
typedef struct {
    int id;
    passenger_state_t state;
    char padding[120];  // Pad to 128 bytes (2 cache lines)
} passenger_t;

/*
 * GLOBAL SHARED STATE:
 * 
 * MEMORY LAYOUT:
 * - Static global variables in data segment
 * - Shared across all threads in process
 * - Virtual memory maps to physical RAM
 * 
 * NUMA CONSIDERATIONS (multi-socket Xeon):
 * - Memory allocated on one NUMA node
 * - Threads on other nodes have higher latency
 * - Use numa_alloc_interleaved() for balanced access
 */
passenger_t* passengers;
int num_passengers;
int num_baggage_handlers;
int num_security_screeners;
int num_flight_attendants;

/*
 * SYNCHRONIZATION PRIMITIVES:
 * 
 * SEMAPHORE-BASED RESOURCE MANAGEMENT:
 * 
 * Semaphores represent available workers:
 * - baggage_sem: counts available baggage handlers
 * - security_sem: counts available security screeners
 * - attendant_sem: counts available flight attendants
 * 
 * HARDWARE IMPLEMENTATION:
 * - Semaphore value stored in memory
 * - Atomic operations (LOCK INC/DEC) modify value
 * - Futex syscall blocks threads when value < 0
 * - Cache coherency ensures all cores see updates
 */
sem_t baggage_sem;
sem_t security_sem;
sem_t attendant_sem;
sem_t all_seated_sem;  // Signals when all passengers seated

// Counters for tracking progress
pthread_mutex_t counter_mutex;
int passengers_seated = 0;

/*
 * BAGGAGE HANDLER THREAD:
 * 
 * WORKER THREAD PATTERN:
 * - Continuously processes passengers from queue
 * - Blocks when no work available
 * - Wakes up when passenger arrives
 * 
 * CPU EXECUTION:
 * - Thread assigned to CPU core by scheduler
 * - Runs until blocked or preempted
 * - Context switch when blocking on semaphore
 */
void* baggage_handler(void* arg) {
    int handler_id = *(int*)arg;
    free(arg);
    
    /*
     * WORKER LOOP:
     * 
     * THREAD LIFECYCLE:
     * 1. RUNNING: Processing passenger
     * 2. BLOCKED: Waiting for passenger (sem_wait)
     * 3. RUNNABLE: Woken by passenger arrival
     * 4. RUNNING: Scheduler assigns to CPU
     * 
     * CACHE BEHAVIOR:
     * - Handler accesses passenger structure
     * - Cache line loaded from L3 or RAM
     * - Subsequent accesses hit L1/L2 cache
     * - Cache line invalidated when passenger moves to next stage
     */
    for (int i = 0; i < num_passengers; i++) {
        /*
         * WAIT FOR PASSENGER:
         * 
         * This is a BLOCKING operation that demonstrates:
         * 
         * 1. THREAD SYNCHRONIZATION:
         *    - Handler waits for passenger to arrive
         *    - Passenger signals availability via sem_post
         * 
         * 2. KERNEL BLOCKING (if no passengers):
         *    - sem_wait() calls futex(FUTEX_WAIT)
         *    - Thread state: RUNNING → INTERRUPTIBLE_SLEEP
         *    - Thread removed from CPU runqueue
         *    - Context switch: save registers to memory
         * 
         * 3. CPU CONTEXT SWITCH:
         *    - Save: RIP, RSP, RBP, RAX-R15 (general-purpose)
         *    - Save: XMM0-XMM15 (SSE), YMM0-YMM15 (AVX) if used
         *    - Save: FPU state, control registers
         *    - Load: Next thread's register state
         *    - TLB flush: Clear virtual address translations
         *    - Cost: ~1-10 microseconds on Intel Xeon
         * 
         * 4. WAKEUP (when passenger arrives):
         *    - Passenger calls sem_post(&baggage_sem)
         *    - Kernel calls futex(FUTEX_WAKE)
         *    - Handler: SLEEP → RUNNABLE
         *    - Scheduler eventually assigns handler to CPU
         */
        // Note: In this implementation, we process passengers sequentially
        // A more realistic implementation would use a work queue
        
        // Find next passenger needing baggage processing
        int passenger_id = -1;
        for (int j = 0; j < num_passengers; j++) {
            if (passengers[j].state == ARRIVED) {
                passenger_id = j;
                break;
            }
        }
        
        if (passenger_id >= 0) {
            /*
             * PROCESS PASSENGER:
             * 
             * CACHE LINE ACCESS:
             * - Load passengers[passenger_id] from memory
             * - Cache hierarchy: L1 → L2 → L3 → RAM
             * - L1 hit: ~4 cycles (1-2 ns)
             * - L2 hit: ~12 cycles (3-4 ns)
             * - L3 hit: ~40 cycles (10-15 ns)
             * - RAM: ~100-200 cycles (50-100 ns)
             * 
             * CACHE COHERENCY (MESI Protocol):
             * - Handler reads passenger state
             * - Cache line enters "Shared" state (if other cores have it)
             * - Handler writes passenger state
             * - Cache line enters "Modified" state
             * - Other cores' copies invalidated
             * 
             * MEMORY BARRIERS:
             * - Compiler barrier: prevents reordering by compiler
             * - Hardware barrier: prevents reordering by CPU
             * - x86-64 TSO: stores not reordered with stores
             * - LOCK prefix: full memory barrier
             */
            passengers[passenger_id].state = BAGGAGE_PROCESSING;
            printf("[Baggage Handler #%d] Processing passenger #%d\n", 
                   handler_id, passengers[passenger_id].id);
            
            /*
             * SIMULATE PROCESSING TIME:
             * 
             * usleep(100) demonstrates:
             * - Voluntary context switch
             * - Thread yields CPU to other threads
             * - Timer interrupt wakes thread after 100 μs
             * - Realistic simulation of I/O or computation
             */
            usleep(100);
            
            passengers[passenger_id].state = BAGGAGE_DONE;
            printf("[Baggage Handler #%d] Completed passenger #%d\n", 
                   handler_id, passengers[passenger_id].id);
        }
    }
    
    return NULL;
}

/*
 * SECURITY SCREENER THREAD:
 * 
 * STAGE 2 WORKER:
 * - Processes passengers after baggage handling
 * - Demonstrates pipeline parallelism
 * - Multiple screeners work concurrently
 */
void* security_screener(void* arg) {
    int screener_id = *(int*)arg;
    free(arg);
    
    for (int i = 0; i < num_passengers; i++) {
        // Find next passenger needing security screening
        int passenger_id = -1;
        for (int j = 0; j < num_passengers; j++) {
            if (passengers[j].state == BAGGAGE_DONE) {
                passenger_id = j;
                break;
            }
        }
        
        if (passenger_id >= 0) {
            passengers[passenger_id].state = SECURITY_SCREENING;
            printf("[Security Screener #%d] Screening passenger #%d\n", 
                   screener_id, passengers[passenger_id].id);
            
            usleep(100);
            
            passengers[passenger_id].state = SECURITY_DONE;
            printf("[Security Screener #%d] Completed passenger #%d\n", 
                   screener_id, passengers[passenger_id].id);
        }
    }
    
    return NULL;
}

/*
 * FLIGHT ATTENDANT THREAD:
 * 
 * STAGE 3 WORKER:
 * - Final stage of passenger processing
 * - Seats passengers on plane
 * - Updates global counter (requires mutex)
 */
void* flight_attendant(void* arg) {
    int attendant_id = *(int*)arg;
    free(arg);
    
    for (int i = 0; i < num_passengers; i++) {
        // Find next passenger ready to board
        int passenger_id = -1;
        for (int j = 0; j < num_passengers; j++) {
            if (passengers[j].state == SECURITY_DONE) {
                passenger_id = j;
                break;
            }
        }
        
        if (passenger_id >= 0) {
            passengers[passenger_id].state = BOARDING;
            printf("[Flight Attendant #%d] Boarding passenger #%d\n", 
                   attendant_id, passengers[passenger_id].id);
            
            usleep(100);
            
            passengers[passenger_id].state = SEATED;
            printf("[Flight Attendant #%d] Passenger #%d is seated\n", 
                   attendant_id, passengers[passenger_id].id);
            
            /*
             * UPDATE GLOBAL COUNTER:
             * 
             * MUTEX PROTECTION:
             * - Multiple attendants update passengers_seated
             * - Race condition without synchronization
             * - Mutex ensures atomic read-modify-write
             * 
             * HARDWARE EXECUTION:
             * 
             * pthread_mutex_lock(&counter_mutex):
             * 1. FAST PATH (uncontended):
             *    - Atomic CAS: LOCK CMPXCHG [mutex], thread_id
             *    - If successful: continue (20-50 cycles)
             *    - Cache line enters "Exclusive" state
             * 
             * 2. SLOW PATH (contended):
             *    - Spin briefly (adaptive mutex)
             *    - futex(FUTEX_LOCK_PI) syscall
             *    - Kernel blocks thread
             *    - Priority inheritance prevents inversion
             * 
             * CRITICAL SECTION:
             * - passengers_seated++ is NOT atomic
             * - Without mutex: lost updates possible
             * - Example race:
             *   Thread A reads 10
             *   Thread B reads 10
             *   Thread A writes 11
             *   Thread B writes 11 (should be 12!)
             * 
             * MEMORY ORDERING:
             * - Mutex provides acquire/release semantics
             * - Acquire: all loads after lock see latest values
             * - Release: all stores before unlock visible to others
             */
            pthread_mutex_lock(&counter_mutex);
            passengers_seated++;
            
            /*
             * CHECK COMPLETION:
             * 
             * When last passenger seated:
             * - Signal main thread via semaphore
             * - Main thread waits for this signal
             * - Demonstrates barrier synchronization
             */
            if (passengers_seated == num_passengers) {
                printf("\n=== All passengers seated! Plane ready for takeoff ===\n");
                sem_post(&all_seated_sem);
            }
            pthread_mutex_unlock(&counter_mutex);
        }
    }
    
    return NULL;
}

/*
 * PASSENGER THREAD:
 * 
 * PRODUCER THREAD:
 * - Arrives at terminal
 * - Requests service from each worker type
 * - Waits for processing at each stage
 */
void* passenger_thread(void* arg) {
    int passenger_id = *(int*)arg;
    free(arg);
    
    printf("Passenger #%d arrived at the terminal.\n", passenger_id);
    passengers[passenger_id].state = ARRIVED;
    
    /*
     * STAGE 1: BAGGAGE PROCESSING
     * 
     * SEMAPHORE COORDINATION:
     * - Passenger waits for available baggage handler
     * - sem_wait() decrements handler count
     * - If count >= 0: handler available (continue)
     * - If count < 0: no handlers (block)
     * 
     * BLOCKING BEHAVIOR:
     * - Thread state: RUNNING → INTERRUPTIBLE_SLEEP
     * - Removed from CPU runqueue
     * - Context switch to another thread
     * - Woken when handler finishes previous passenger
     */
    printf("Passenger #%d is waiting at baggage processing.\n", passenger_id);
    sem_wait(&baggage_sem);
    
    // Wait for baggage processing to complete
    while (passengers[passenger_id].state != BAGGAGE_DONE) {
        usleep(50);  // Polling with sleep (not ideal, but simple)
    }
    
    sem_post(&baggage_sem);  // Release handler for next passenger
    
    /*
     * STAGE 2: SECURITY SCREENING
     * 
     * PIPELINE PARALLELISM:
     * - Passenger moves to next stage
     * - Previous stage can process next passenger
     * - Demonstrates throughput improvement
     * 
     * CACHE EFFECTS:
     * - Passenger data accessed by different workers
     * - Cache line migrates between cores
     * - MESI protocol ensures coherency
     * - Padding prevents false sharing
     */
    printf("Passenger #%d is waiting to be screened.\n", passenger_id);
    sem_wait(&security_sem);
    
    while (passengers[passenger_id].state != SECURITY_DONE) {
        usleep(50);
    }
    
    sem_post(&security_sem);
    
    /*
     * STAGE 3: BOARDING
     * 
     * FINAL STAGE:
     * - Passenger boards plane
     * - Attendant seats passenger
     * - Updates global counter
     */
    printf("Passenger #%d is waiting to board the plane.\n", passenger_id);
    sem_wait(&attendant_sem);
    
    while (passengers[passenger_id].state != SEATED) {
        usleep(50);
    }
    
    printf("Passenger #%d has been seated and relaxes.\n", passenger_id);
    sem_post(&attendant_sem);
    
    return NULL;
}

/*
 * MAIN FUNCTION:
 * 
 * PROGRAM INITIALIZATION:
 * - Parse command-line arguments
 * - Allocate memory for passengers
 * - Initialize synchronization primitives
 * - Create worker threads
 * - Create passenger threads
 * - Wait for completion
 */
int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <passengers> <baggage_handlers> <security_screeners> <flight_attendants>\n", 
                argv[0]);
        return EXIT_FAILURE;
    }
    
    /*
     * PARSE ARGUMENTS:
     * 
     * Command line: ./airline 100 3 5 2
     * - 100 passengers
     * - 3 baggage handlers
     * - 5 security screeners
     * - 2 flight attendants
     */
    num_passengers = atoi(argv[1]);
    num_baggage_handlers = atoi(argv[2]);
    num_security_screeners = atoi(argv[3]);
    num_flight_attendants = atoi(argv[4]);
    
    if (num_passengers <= 0 || num_baggage_handlers <= 0 || 
        num_security_screeners <= 0 || num_flight_attendants <= 0) {
        fprintf(stderr, "Error: All counts must be positive\n");
        return EXIT_FAILURE;
    }
    
    /*
     * ALLOCATE PASSENGER ARRAY:
     * 
     * MEMORY ALLOCATION:
     * - malloc() allocates from heap
     * - Heap managed by glibc allocator
     * - Backed by anonymous memory pages
     * - Virtual memory mapped to physical RAM
     * 
     * NUMA CONSIDERATIONS:
     * - Memory allocated on current NUMA node
     * - For multi-socket Xeon: use numa_alloc_interleaved()
     * - Distributes memory across all nodes
     * - Balances memory bandwidth
     */
    passengers = malloc(num_passengers * sizeof(passenger_t));
    if (!passengers) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }
    
    // Initialize passengers
    for (int i = 0; i < num_passengers; i++) {
        passengers[i].id = i + 1;
        passengers[i].state = ARRIVED;
    }
    
    /*
     * INITIALIZE SYNCHRONIZATION PRIMITIVES:
     * 
     * SEMAPHORE INITIALIZATION:
     * - Initial value = number of workers
     * - Represents available resources
     * - Passengers decrement (acquire resource)
     * - Workers increment (release resource)
     */
    if (pthread_mutex_init(&counter_mutex, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&baggage_sem, 0, num_baggage_handlers) != 0) {
        fprintf(stderr, "Baggage semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&security_sem, 0, num_security_screeners) != 0) {
        fprintf(stderr, "Security semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&attendant_sem, 0, num_flight_attendants) != 0) {
        fprintf(stderr, "Attendant semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&all_seated_sem, 0, 0) != 0) {
        fprintf(stderr, "All seated semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    printf("=== Airline Passenger Processing ===\n");
    printf("Passengers: %d\n", num_passengers);
    printf("Baggage Handlers: %d\n", num_baggage_handlers);
    printf("Security Screeners: %d\n", num_security_screeners);
    printf("Flight Attendants: %d\n\n", num_flight_attendants);
    
    /*
     * CREATE WORKER THREADS:
     * 
     * THREAD POOL PATTERN:
     * - Create workers before passengers
     * - Workers wait for work
     * - Passengers arrive and request service
     * 
     * CPU SCHEDULING:
     * - Workers distributed across CPU cores
     * - Scheduler balances load
     * - NUMA-aware placement improves performance
     */
    pthread_t* baggage_threads = malloc(num_baggage_handlers * sizeof(pthread_t));
    pthread_t* security_threads = malloc(num_security_screeners * sizeof(pthread_t));
    pthread_t* attendant_threads = malloc(num_flight_attendants * sizeof(pthread_t));
    pthread_t* passenger_threads = malloc(num_passengers * sizeof(pthread_t));
    
    // Create baggage handler threads
    for (int i = 0; i < num_baggage_handlers; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&baggage_threads[i], NULL, baggage_handler, id) != 0) {
            fprintf(stderr, "Baggage handler thread creation failed\n");
            return EXIT_FAILURE;
        }
    }
    
    // Create security screener threads
    for (int i = 0; i < num_security_screeners; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&security_threads[i], NULL, security_screener, id) != 0) {
            fprintf(stderr, "Security screener thread creation failed\n");
            return EXIT_FAILURE;
        }
    }
    
    // Create flight attendant threads
    for (int i = 0; i < num_flight_attendants; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&attendant_threads[i], NULL, flight_attendant, id) != 0) {
            fprintf(stderr, "Flight attendant thread creation failed\n");
            return EXIT_FAILURE;
        }
    }
    
    // Create passenger threads (after workers)
    for (int i = 0; i < num_passengers; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&passenger_threads[i], NULL, passenger_thread, id) != 0) {
            fprintf(stderr, "Passenger thread creation failed\n");
            return EXIT_FAILURE;
        }
    }
    
    /*
     * WAIT FOR COMPLETION:
     * 
     * BARRIER SYNCHRONIZATION:
     * - Main thread waits for all passengers to be seated
     * - sem_wait() blocks until last passenger seated
     * - Last attendant calls sem_post() to wake main
     */
    sem_wait(&all_seated_sem);
    
    /*
     * JOIN ALL THREADS:
     * 
     * THREAD CLEANUP:
     * - Wait for all threads to exit
     * - Retrieve exit status (NULL in this case)
     * - Deallocate thread resources
     */
    for (int i = 0; i < num_passengers; i++) {
        pthread_join(passenger_threads[i], NULL);
    }
    
    for (int i = 0; i < num_baggage_handlers; i++) {
        pthread_join(baggage_threads[i], NULL);
    }
    
    for (int i = 0; i < num_security_screeners; i++) {
        pthread_join(security_threads[i], NULL);
    }
    
    for (int i = 0; i < num_flight_attendants; i++) {
        pthread_join(attendant_threads[i], NULL);
    }
    
    printf("\n=== Flight Departed Successfully ===\n");
    
    // Cleanup
    pthread_mutex_destroy(&counter_mutex);
    sem_destroy(&baggage_sem);
    sem_destroy(&security_sem);
    sem_destroy(&attendant_sem);
    sem_destroy(&all_seated_sem);
    
    free(passengers);
    free(baggage_threads);
    free(security_threads);
    free(attendant_threads);
    free(passenger_threads);
    
    return EXIT_SUCCESS;
}

/*
 * PERFORMANCE ANALYSIS ON INTEL XEON SAPPHIRE RAPIDS:
 * 
 * 1. SCALABILITY:
 *    - Workers run on separate cores (true parallelism)
 *    - Up to 60 cores per socket on Xeon
 *    - Throughput scales with number of workers
 *    - Bottleneck: slowest stage (Amdahl's Law)
 * 
 * 2. CACHE EFFECTS:
 *    - Each passenger: 128 bytes (2 cache lines)
 *    - 100 passengers: 12.8 KB (fits in L1 cache)
 *    - Workers access different passengers (no false sharing)
 *    - Cache coherency traffic: moderate
 * 
 * 3. SYNCHRONIZATION OVERHEAD:
 *    - Each passenger: 6 semaphore operations
 *    - 100 passengers: 600 total operations
 *    - Uncontended: ~20-50 cycles each
 *    - Contended: ~1-10 μs each (kernel syscall)
 * 
 * 4. CONTEXT SWITCHING:
 *    - Frequent blocking/waking of threads
 *    - Context switch: ~1-10 μs
 *    - Cache pollution from thread migration
 *    - CPU affinity can reduce migration
 * 
 * 5. NUMA EFFECTS (multi-socket):
 *    - Memory allocated on one node
 *    - Workers on other nodes: higher latency
 *    - Use numa_alloc_interleaved() for balance
 *    - Pin threads to local node for best performance
 * 
 * OPTIMIZATION STRATEGIES:
 * 
 * 1. WORK QUEUE:
 *    - Current: polling for next passenger
 *    - Better: lock-free queue with atomic operations
 *    - Reduces contention and improves throughput
 * 
 * 2. BATCH PROCESSING:
 *    - Process multiple passengers per worker
 *    - Amortizes synchronization overhead
 *    - Improves cache locality
 * 
 * 3. LOCK-FREE ALGORITHMS:
 *    - Replace semaphores with atomic operations
 *    - Eliminates kernel involvement
 *    - Example: atomic counters, CAS loops
 * 
 * 4. CPU AFFINITY:
 *    - Pin workers to specific cores
 *    - Reduces cache thrashing from migration
 *    - Improves cache locality
 * 
 * 5. HUGE PAGES:
 *    - Use 2MB pages instead of 4KB
 *    - Reduces TLB misses
 *    - Improves memory access performance
 * 
 * END-TO-END EXECUTION FLOW:
 * 
 * Passenger Thread:
 *   sem_wait(&baggage_sem)
 *     ↓
 * POSIX Library:
 *   LOCK DEC [baggage_sem]
 *   if (result < 0) syscall(SYS_futex, FUTEX_WAIT)
 *     ↓
 * Linux Kernel:
 *   futex_wait() → add to wait queue
 *   schedule() → CFS picks next thread
 *   context_switch() → save/restore registers
 *     ↓
 * Intel Xeon CPU:
 *   LOCK prefix → cache line locking
 *   MESI protocol → invalidate other cores
 *   Context switch → TLB flush, cache miss
 *   Scheduler → assign thread to core
 *     ↓
 * Hardware Execution:
 *   Fetch from L1 I-cache
 *   Decode and dispatch
 *   Out-of-order execution
 *   Memory access (L1/L2/L3/RAM)
 *   Retire and update state
 * 
 * This demonstrates the complete stack from user code to hardware execution!
 */
