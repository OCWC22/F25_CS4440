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
 * SEMAPHORE-BASED RESOURCE MANAGEMENT AND QUEUING:
 * 
 * We maintain explicit task queues per processing stage. Each passenger pushes
 * their ID into the appropriate queue and blocks on a per-passenger semaphore
 * until a worker thread completes the stage. Worker availability is tracked via
 * counting semaphores so that passengers never busy-wait—if no worker is
 * available the passenger blocks inside sem_wait().
 */

typedef struct {
    int* buffer;
    int capacity;
    int head;
    int tail;
    pthread_mutex_t mutex;
    sem_t items;
    sem_t spaces;
} task_queue_t;

typedef struct {
    sem_t baggage_done;
    sem_t security_done;
    sem_t boarded;
} passenger_sync_t;

task_queue_t baggage_queue;
task_queue_t security_queue;
task_queue_t boarding_queue;

sem_t baggage_handlers_available;
sem_t security_screeners_available;
sem_t attendants_available;
sem_t all_seated_sem;  // Signals when all passengers seated

passenger_sync_t* passenger_sync;

// Counters for tracking progress
pthread_mutex_t counter_mutex;
pthread_mutex_t state_mutex;
int passengers_seated = 0;

static int queue_init(task_queue_t* q, int capacity) {
    q->buffer = malloc(sizeof(int) * capacity);
    if (!q->buffer) {
        return -1;
    }
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&q->mutex, NULL);
    sem_init(&q->items, 0, 0);
    sem_init(&q->spaces, 0, capacity);
    return 0;
}

static void queue_destroy(task_queue_t* q) {
    pthread_mutex_destroy(&q->mutex);
    sem_destroy(&q->items);
    sem_destroy(&q->spaces);
    free(q->buffer);
}

static void queue_push(task_queue_t* q, int value) {
    sem_wait(&q->spaces);
    pthread_mutex_lock(&q->mutex);
    q->buffer[q->tail] = value;
    q->tail = (q->tail + 1) % q->capacity;
    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->items);
}

static int queue_pop(task_queue_t* q) {
    sem_wait(&q->items);
    pthread_mutex_lock(&q->mutex);
    int value = q->buffer[q->head];
    q->head = (q->head + 1) % q->capacity;
    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->spaces);
    return value;
}

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
    
    while (true) {
        int passenger_index = queue_pop(&baggage_queue);
        if (passenger_index < 0) {
            break;
        }

        int passenger_id = passengers[passenger_index].id;

        pthread_mutex_lock(&state_mutex);
        passengers[passenger_index].state = BAGGAGE_PROCESSING;
        pthread_mutex_unlock(&state_mutex);

        printf("[Baggage Handler #%d] Processing passenger #%d\n", handler_id, passenger_id);
        usleep(100);

        pthread_mutex_lock(&state_mutex);
        passengers[passenger_index].state = BAGGAGE_DONE;
        pthread_mutex_unlock(&state_mutex);

        printf("[Baggage Handler #%d] Completed passenger #%d\n", handler_id, passenger_id);

        sem_post(&passenger_sync[passenger_index].baggage_done);
        sem_post(&baggage_handlers_available);
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
    
    while (true) {
        int passenger_index = queue_pop(&security_queue);
        if (passenger_index < 0) {
            break;
        }

        int passenger_id = passengers[passenger_index].id;

        pthread_mutex_lock(&state_mutex);
        passengers[passenger_index].state = SECURITY_SCREENING;
        pthread_mutex_unlock(&state_mutex);

        printf("[Security Screener #%d] Screening passenger #%d\n", screener_id, passenger_id);
        usleep(100);

        pthread_mutex_lock(&state_mutex);
        passengers[passenger_index].state = SECURITY_DONE;
        pthread_mutex_unlock(&state_mutex);

        printf("[Security Screener #%d] Completed passenger #%d\n", screener_id, passenger_id);

        sem_post(&passenger_sync[passenger_index].security_done);
        sem_post(&security_screeners_available);
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
    
    while (true) {
        int passenger_index = queue_pop(&boarding_queue);
        if (passenger_index < 0) {
            break;
        }

        int passenger_id = passengers[passenger_index].id;

        pthread_mutex_lock(&state_mutex);
        passengers[passenger_index].state = BOARDING;
        pthread_mutex_unlock(&state_mutex);

        printf("[Flight Attendant #%d] Boarding passenger #%d\n", attendant_id, passenger_id);
        usleep(100);

        pthread_mutex_lock(&state_mutex);
        passengers[passenger_index].state = SEATED;
        pthread_mutex_unlock(&state_mutex);

        printf("[Flight Attendant #%d] Passenger #%d is seated\n", attendant_id, passenger_id);

        sem_post(&passenger_sync[passenger_index].boarded);

        pthread_mutex_lock(&counter_mutex);
        passengers_seated++;
        if (passengers_seated == num_passengers) {
            printf("\n=== All passengers seated! Plane ready for takeoff ===\n");
            sem_post(&all_seated_sem);
        }
        pthread_mutex_unlock(&counter_mutex);

        sem_post(&attendants_available);
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
    int passenger_index = passenger_id - 1;
    pthread_mutex_lock(&state_mutex);
    passengers[passenger_index].state = ARRIVED;
    pthread_mutex_unlock(&state_mutex);
    
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
    printf("Passenger #%d is waiting at baggage processing for a handler.\n", passenger_id);
    sem_wait(&baggage_handlers_available);
    queue_push(&baggage_queue, passenger_index);
    sem_wait(&passenger_sync[passenger_index].baggage_done);
    
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
    printf("Passenger #%d is waiting to be screened by a screener.\n", passenger_id);
    sem_wait(&security_screeners_available);
    queue_push(&security_queue, passenger_index);
    sem_wait(&passenger_sync[passenger_index].security_done);
    
    /*
     * STAGE 3: BOARDING
     * 
     * FINAL STAGE:
     * - Passenger boards plane
     * - Attendant seats passenger
     * - Updates global counter
     */
    printf("Passenger #%d is waiting to board the plane by an attendant.\n", passenger_id);
    sem_wait(&attendants_available);
    queue_push(&boarding_queue, passenger_index);
    sem_wait(&passenger_sync[passenger_index].boarded);
    printf("Passenger #%d has been seated and relaxes.\n", passenger_id);
    
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
    if (pthread_mutex_init(&state_mutex, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&baggage_handlers_available, 0, num_baggage_handlers) != 0) {
        fprintf(stderr, "Baggage semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&security_screeners_available, 0, num_security_screeners) != 0) {
        fprintf(stderr, "Security semaphore initialization failed\n");
        return EXIT_FAILURE;
    }
    
    if (sem_init(&attendants_available, 0, num_flight_attendants) != 0) {
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
    if (queue_init(&baggage_queue, num_passengers) != 0 ||
        queue_init(&security_queue, num_passengers) != 0 ||
        queue_init(&boarding_queue, num_passengers) != 0) {
        fprintf(stderr, "Queue initialization failed\n");
        return EXIT_FAILURE;
    }

    passenger_sync = malloc(num_passengers * sizeof(passenger_sync_t));
    if (!passenger_sync) {
        fprintf(stderr, "Passenger sync allocation failed\n");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < num_passengers; i++) {
        sem_init(&passenger_sync[i].baggage_done, 0, 0);
        sem_init(&passenger_sync[i].security_done, 0, 0);
        sem_init(&passenger_sync[i].boarded, 0, 0);
    }

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

    for (int i = 0; i < num_baggage_handlers; i++) {
        queue_push(&baggage_queue, -1);
    }
    for (int i = 0; i < num_security_screeners; i++) {
        queue_push(&security_queue, -1);
    }
    for (int i = 0; i < num_flight_attendants; i++) {
        queue_push(&boarding_queue, -1);
    }
    
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
    pthread_mutex_destroy(&state_mutex);
    sem_destroy(&baggage_handlers_available);
    sem_destroy(&security_screeners_available);
    sem_destroy(&attendants_available);
    sem_destroy(&all_seated_sem);

    for (int i = 0; i < num_passengers; i++) {
        sem_destroy(&passenger_sync[i].baggage_done);
        sem_destroy(&passenger_sync[i].security_done);
        sem_destroy(&passenger_sync[i].boarded);
    }

    queue_destroy(&baggage_queue);
    queue_destroy(&security_queue);
    queue_destroy(&boarding_queue);

    free(passenger_sync);
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
