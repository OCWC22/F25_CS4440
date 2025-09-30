# Part 5: Complete Code Examples
## Line-by-Line Hardware Execution Analysis

**Learning Objectives:**
- Understand complete execution flow from C code to CPU hardware
- See system calls in action
- Master synchronization primitives with real examples
- Connect theory to practice

---

## 1. System Calls: File Copy

### 1.1 Source Code

```c
// From week_1/example2-copy.c
#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *source, *destination;
    int ch;
    
    source = fopen("source.txt", "r");
    destination = fopen("destination.txt", "w");
    
    ch = fgetc(source);
    while (ch != EOF) {
        fputc(ch, destination);
        ch = fgetc(source);
    }
    
    fclose(source);
    fclose(destination);
    
    return 0;
}
```

### 1.2 Execution Flow

**Line: `source = fopen("source.txt", "r");`**

```
User Space (Ring 3):
├─ fopen() is C library wrapper
├─ Prepares arguments for open() syscall
└─ Calls open("source.txt", O_RDONLY)

System Call Transition:
├─ SYSCALL instruction executed
├─ CPU switches to Ring 0 (kernel mode)
├─ Saves user registers to kernel stack
└─ Jumps to sys_open() handler

Kernel Space (Ring 0):
├─ Validates path string
├─ Checks file permissions
├─ Allocates file descriptor (fd = 3)
├─ Opens file on disk
├─ Creates file table entry
└─ Returns fd to user space

Return to User:
├─ CPU switches back to Ring 3
├─ Restores user registers
├─ fopen() wraps fd in FILE* structure
└─ Returns FILE* to user

Cost: ~1-10 microseconds
```

**Line: `ch = fgetc(source);`**

```
First Call (Buffer Empty):
├─ fgetc() checks internal buffer (empty)
├─ Calls read(fd, buffer, 4096)  ← Syscall!
├─ Kernel reads 4KB from disk/cache
├─ Copies data to user buffer
├─ fgetc() returns first byte
└─ Cost: ~1-10 microseconds

Subsequent Calls (Buffer Has Data):
├─ fgetc() reads from internal buffer
├─ No system call needed!
├─ Pure userspace operation
└─ Cost: ~10-20 nanoseconds (1000x faster!)

Buffering Benefit:
- 1 syscall serves 4096 fgetc() calls
- Amortized cost: ~2-5 nanoseconds per byte
```

---

## 2. Process Creation: fork()

### 2.1 Source Code

```c
// From week_2/example3-fork.c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid;
    
    printf("Before fork: PID = %d\n", getpid());
    
    pid = fork();
    
    if (pid < 0) {
        printf("fork error\n");
    }
    else if (pid == 0) {
        printf("Child: PID = %d, parent = %d\n", 
               getpid(), getppid());
    }
    else {
        wait(NULL);
        printf("Parent: PID = %d, child = %d\n", 
               getpid(), pid);
    }
    
    return 0;
}
```

### 2.2 Execution Flow

**Line: `pid = fork();`**

```
User Space:
├─ fork() wrapper prepares syscall
└─ Executes SYSCALL instruction

Kernel (sys_fork):
Step 1: Allocate new process
├─ Create new task_struct
├─ Assign new PID (e.g., 5678)
└─ Copy parent's task_struct fields

Step 2: Memory management
├─ Copy page tables (virtual memory mappings)
├─ Mark all pages as copy-on-write (COW)
├─ Don't actually copy memory yet!
└─ Cost: ~100-200 microseconds

Step 3: File descriptors
├─ Copy file descriptor table
├─ Increment reference counts
└─ Both processes share open files

Step 4: Scheduling
├─ Add child to ready queue
├─ Child state: READY
└─ May run on different core!

Step 5: Return to user space
├─ Parent: fork() returns child PID (5678)
├─ Child: fork() returns 0
└─ Both continue execution

Intel Xeon Hardware:
Core 0: Parent process continues
Core 1: Child process starts (scheduler decision)
Both cores execute independently!
```

**Copy-on-Write (COW) Optimization:**

```
Initial State (after fork):
Parent and child share same physical memory pages
All pages marked read-only

When parent writes to page:
1. CPU triggers page fault (write to read-only page)
2. Kernel handles fault:
   ├─ Allocate new physical page
   ├─ Copy original page to new page
   ├─ Update parent's page table → new page
   └─ Mark both pages as writable
3. Parent continues with write

Result: Only modified pages are copied!
- Saves memory
- Faster fork() (no immediate copying)
- Typical: Only 10-20% of pages are written
```

---

## 3. Inter-Process Communication: Pipes

### 3.1 Source Code

```c
// From week_2/example4-pipe.c (simplified)
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[100];
    
    pipe(pipefd);  // Create pipe
    pid = fork();
    
    if (pid == 0) {
        // Child: Reader
        close(pipefd[1]);  // Close write end
        
        int n = read(pipefd[0], buffer, sizeof(buffer));
        printf("Child received: %s\n", buffer);
        
        close(pipefd[0]);
    }
    else {
        // Parent: Writer
        close(pipefd[0]);  // Close read end
        
        write(pipefd[1], "Hello from parent!", 18);
        
        close(pipefd[1]);
        wait(NULL);
    }
    
    return 0;
}
```

### 3.2 Execution Flow

**Line: `pipe(pipefd);`**

```
System Call:
├─ Allocates kernel pipe buffer (4KB default)
├─ Creates two file descriptors:
│  ├─ pipefd[0] = 3 (read end)
│  └─ pipefd[1] = 4 (write end)
├─ Initializes wait queues (for blocking)
└─ Returns 0 (success)

Kernel Data Structure:
struct pipe_buffer {
    char data[4096];      // Circular buffer
    int read_pos;         // Read position
    int write_pos;        // Write position
    int count;            // Bytes in buffer
    wait_queue_head_t readers;  // Blocked readers
    wait_queue_head_t writers;  // Blocked writers
    spinlock_t lock;      // Protects buffer
};
```

**Line: `write(pipefd[1], "Hello from parent!", 18);`**

```
User Space (Parent):
├─ write() syscall with 18 bytes
└─ SYSCALL instruction

Kernel:
Step 1: Validate
├─ Check fd 4 is valid
├─ Check fd 4 is write end of pipe
└─ Check permissions

Step 2: Acquire lock
├─ spin_lock(&pipe->lock)
└─ Protects pipe buffer from concurrent access

Step 3: Check space
├─ Available space = 4096 - count
├─ Need 18 bytes
└─ Space available: proceed

Step 4: Copy data
├─ copy_from_user(pipe->data + write_pos, user_buffer, 18)
├─ Copies from user space to kernel pipe buffer
├─ Update write_pos: (write_pos + 18) % 4096
└─ Update count: count += 18

Step 5: Wake readers
├─ Check if any readers waiting
├─ If yes: wake_up(&pipe->readers)
└─ Child process wakes up!

Step 6: Release lock
├─ spin_unlock(&pipe->lock)
└─ Return 18 (bytes written)

Cost: ~1-5 microseconds
```

**Line: `int n = read(pipefd[0], buffer, sizeof(buffer));` (Child)**

```
Kernel:
Step 1: Validate
├─ Check fd 3 is valid
└─ Check fd 3 is read end of pipe

Step 2: Acquire lock
├─ spin_lock(&pipe->lock)
└─ Protects pipe buffer

Step 3: Check data
├─ If count > 0: data available
├─ If count == 0 and write end closed: return EOF
└─ If count == 0 and write end open: block

Step 4: Copy data (data available)
├─ bytes_to_read = min(requested, count)
├─ copy_to_user(user_buffer, pipe->data + read_pos, bytes_to_read)
├─ Update read_pos: (read_pos + bytes_to_read) % 4096
└─ Update count: count -= bytes_to_read

Step 5: Wake writers
├─ If writers waiting (buffer was full)
└─ wake_up(&pipe->writers)

Step 6: Release lock
└─ Return bytes_to_read

Intel Xeon Hardware:
- Parent and child may run on different cores
- Pipe buffer in shared memory (kernel space)
- Cache coherency ensures data visibility
- MESI protocol: pipe buffer cache line shared between cores
```

---

## 4. Thread Synchronization: Mutex

### 4.1 Source Code

```c
// From week_5/example13_mutex.cpp
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lock;
int shared_counter = 0;

void *increment(void *arg) {
    for (int i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&lock);
        shared_counter++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    pthread_t tid_1, tid_2;
    
    pthread_mutex_init(&lock, NULL);
    
    pthread_create(&tid_1, NULL, &increment, NULL);
    pthread_create(&tid_2, NULL, &increment, NULL);
    
    pthread_join(tid_1, NULL);
    pthread_join(tid_2, NULL);
    
    printf("Counter: %d\n", shared_counter);
    
    pthread_mutex_destroy(&lock);
    return 0;
}
```

### 4.2 Hardware Execution

**Line: `pthread_mutex_lock(&lock);`**

```
Fast Path (Uncontended):

User Space (glibc):
├─ Load mutex value from memory
├─ Execute atomic CAS:
│  LOCK CMPXCHG [mutex], thread_id
└─ If successful: return immediately

Intel Xeon CPU (Core 0):
Step 1: Fetch instruction
├─ LOCK CMPXCHG from L1 I-cache
└─ Decode to micro-ops

Step 2: Load mutex value
├─ Load from L1 D-cache (if cached)
├─ Or from L2/L3/RAM (if not cached)
└─ Current value: 0 (unlocked)

Step 3: LOCK prefix
├─ Asserts cache lock (not bus lock)
├─ Cache line enters "Exclusive" state
└─ Other cores' cache lines invalidated

Step 4: Compare and swap
├─ Compare mutex (0) with expected (0)
├─ Match! Write thread_id to mutex
└─ Set ZF flag (success)

Step 5: Memory barrier
├─ LOCK prefix provides full barrier
├─ All prior loads/stores complete
└─ Prevents reordering

Step 6: Return
├─ Lock acquired!
└─ Cost: 20-50 cycles (~10 ns)

Slow Path (Contended):

Thread 2 (Core 1) tries to acquire:
Step 1: Load mutex value
├─ Cache miss! (Core 0 has line in "Modified" state)
├─ Request cache line from Core 0
└─ Cache coherency protocol transfers line

Step 2: LOCK CMPXCHG
├─ Compare mutex (thread_1_id) with expected (0)
├─ No match! CAS fails
└─ Clear ZF flag (failure)

Step 3: Spin loop
for (int i = 0; i < SPIN_COUNT; i++) {
    PAUSE;  // Hint to CPU: we're spinning
    if (CAS succeeds) return;
}

Step 4: Futex syscall (if still contended)
├─ syscall(SYS_futex, &mutex, FUTEX_WAIT, ...)
├─ Kernel blocks thread
├─ Thread state: WAITING
├─ Context switch to another thread
└─ Cost: 1-10 microseconds

Step 5: Wake up (when lock released)
├─ Thread 1 calls pthread_mutex_unlock()
├─ Kernel wakes Thread 2
├─ Thread 2 state: READY → RUNNING
└─ Thread 2 tries to acquire lock again
```

**Line: `shared_counter++;`**

```
Without Mutex (Race Condition):
Thread 1 (Core 0):          Thread 2 (Core 1):
MOV RAX, [counter]          MOV RBX, [counter]
  (load 0)                    (load 0)
INC RAX                     INC RBX
  (0 → 1)                     (0 → 1)
MOV [counter], RAX          MOV [counter], RBX
  (store 1)                   (store 1)

Result: counter = 1 (should be 2) ← Lost update!

With Mutex (Correct):
Thread 1 (Core 0):          Thread 2 (Core 1):
LOCK CMPXCHG [mutex], 1     LOCK CMPXCHG [mutex], 2
  (acquired)                  (fails, blocks)
MOV RAX, [counter]
  (load 0)
INC RAX
  (0 → 1)
MOV [counter], RAX
  (store 1)
LOCK XCHG [mutex], 0        (still blocked)
  (released)
                            (wakes up)
                            LOCK CMPXCHG [mutex], 2
                              (acquired)
                            MOV RBX, [counter]
                              (load 1)
                            INC RBX
                              (1 → 2)
                            MOV [counter], RBX
                              (store 2)
                            LOCK XCHG [mutex], 0
                              (released)

Result: counter = 2 (correct!)
```

---

## 5. Producer-Consumer: Complete Solution

### 5.1 Source Code (Simplified)

```c
// From project_2/bounded_buffer.c
#define BUFFER_SIZE 5

typedef struct {
    char buffer[BUFFER_SIZE];
    int in, out, count;
    pthread_mutex_t mutex;
    sem_t empty, full;
} bounded_buffer_t;

bounded_buffer_t buf;

void* producer(void* arg) {
    for (int i = 0; i < 10; i++) {
        char item = 'A' + i;
        
        sem_wait(&buf.empty);          // Wait for empty slot
        pthread_mutex_lock(&buf.mutex); // Acquire lock
        
        buf.buffer[buf.in] = item;     // Add item
        buf.in = (buf.in + 1) % BUFFER_SIZE;
        buf.count++;
        printf("Produced: %c (count=%d)\n", item, buf.count);
        
        pthread_mutex_unlock(&buf.mutex); // Release lock
        sem_post(&buf.full);              // Signal full slot
        
        usleep(100000);  // 100ms
    }
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 0; i < 10; i++) {
        sem_wait(&buf.full);           // Wait for full slot
        pthread_mutex_lock(&buf.mutex); // Acquire lock
        
        char item = buf.buffer[buf.out]; // Remove item
        buf.out = (buf.out + 1) % BUFFER_SIZE;
        buf.count--;
        printf("Consumed: %c (count=%d)\n", item, buf.count);
        
        pthread_mutex_unlock(&buf.mutex); // Release lock
        sem_post(&buf.empty);             // Signal empty slot
        
        usleep(150000);  // 150ms
    }
    return NULL;
}
```

### 5.2 Execution Timeline

```
Time | Producer (Core 0)         | Consumer (Core 1)         | Buffer State
─────┼───────────────────────────┼───────────────────────────┼─────────────
  0  | sem_wait(&empty)          |                           | empty=5
     |   → empty=4 (success)     |                           | full=0
     | mutex_lock()              |                           | count=0
     |   → acquired              |                           |
     | buffer[0] = 'A'           |                           |
     | in=1, count=1             |                           |
     | mutex_unlock()            |                           |
     | sem_post(&full)           |                           |
     |   → full=1                |                           |
─────┼───────────────────────────┼───────────────────────────┼─────────────
 100 | usleep(100ms)             | sem_wait(&full)           | [A,_,_,_,_]
     |                           |   → full=0 (success)      | empty=4
     |                           | mutex_lock()              | full=0
     |                           |   → acquired              | count=1
     |                           | item = buffer[0] = 'A'    |
     |                           | out=1, count=0            |
     |                           | mutex_unlock()            |
     |                           | sem_post(&empty)          |
     |                           |   → empty=5               |
─────┼───────────────────────────┼───────────────────────────┼─────────────
 200 | sem_wait(&empty)          | usleep(150ms)             | [_,_,_,_,_]
     |   → empty=4 (success)     |                           | empty=5
     | mutex_lock()              |                           | full=0
     |   → acquired              |                           | count=0
     | buffer[1] = 'B'           |                           |
     | in=2, count=1             |                           |
     | mutex_unlock()            |                           |
     | sem_post(&full)           |                           |
     |   → full=1                |                           |
─────┼───────────────────────────┼───────────────────────────┼─────────────

(Pattern continues...)
```

**Key Synchronization Points:**

```
Scenario 1: Buffer Full (Producer Blocks)
─────────────────────────────────────────
Buffer: [A,B,C,D,E] (full)
empty = 0, full = 5, count = 5

Producer:
├─ sem_wait(&empty)
├─ Atomic decrement: 0 → -1
├─ Result < 0: Block!
├─ futex(FUTEX_WAIT) syscall
├─ Kernel adds thread to wait queue
└─ Thread state: WAITING

Consumer (later):
├─ Consumes item 'A'
├─ sem_post(&empty)
├─ Atomic increment: -1 → 0
├─ Waiters exist: futex(FUTEX_WAKE)
├─ Kernel wakes producer
└─ Producer state: READY → RUNNING

Scenario 2: Buffer Empty (Consumer Blocks)
───────────────────────────────────────────
Buffer: [_,_,_,_,_] (empty)
empty = 5, full = 0, count = 0

Consumer:
├─ sem_wait(&full)
├─ Atomic decrement: 0 → -1
├─ Result < 0: Block!
└─ Thread state: WAITING

Producer (later):
├─ Produces item 'A'
├─ sem_post(&full)
├─ Atomic increment: -1 → 0
├─ Wakes consumer
└─ Consumer state: READY → RUNNING
```

---

## Summary: Code to Hardware Mapping

### System Calls
```
C Function    → Syscall      → Kernel Function → Hardware
────────────────────────────────────────────────────────────
fopen()       → open()       → sys_open()      → Disk I/O
fread()       → read()       → sys_read()      → DMA transfer
fork()        → fork()       → sys_fork()      → MMU (page tables)
pipe()        → pipe()       → sys_pipe()      → Memory allocation
```

### Synchronization
```
POSIX API              → Atomic Instruction → CPU Hardware
──────────────────────────────────────────────────────────
pthread_mutex_lock()   → LOCK CMPXCHG       → Cache coherency (MESI)
pthread_mutex_unlock() → LOCK XCHG          → Memory barrier
sem_wait()             → LOCK DEC           → Atomic decrement
sem_post()             → LOCK INC           → Atomic increment
```

### Performance Costs
```
Operation                    Cost (cycles)    Cost (time)
────────────────────────────────────────────────────────
Register access              1                0.3 ns
L1 cache hit                 4                1.3 ns
L2 cache hit                 12               4 ns
L3 cache hit                 40               13 ns
RAM access                   100-200          50-100 ns
Uncontended mutex            20-50            10-25 ns
Contended mutex (spin)       100-1000         50-500 ns
System call                  1000-10000       0.5-5 µs
Context switch               10000-100000     5-50 µs
```

### Next Steps
Use these examples as templates for your own code. Understand the complete execution flow from user space to hardware, and you'll be able to write high-performance, correct concurrent programs.
