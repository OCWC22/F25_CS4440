# Part 2: Operating System Fundamentals
## From Hardware to Software Abstraction

**Learning Objectives:**
- Understand what an OS does and why we need it
- Master system calls (the bridge between user and kernel)
- Learn process lifecycle and management
- Understand threads and their relationship to processes
- Master Inter-Process Communication (IPC)

---

## 1. What is an Operating System?

### 1.1 The Government of Your Computer

**CEO Analogy:** An OS is like a government:
- **Resource Manager:** Allocates CPU time, memory, disk space
- **Law Enforcer:** Prevents programs from interfering with each other
- **Service Provider:** Provides common services (file system, networking)
- **Intermediary:** Translates between hardware and software

### 1.2 OS Responsibilities

```
┌─────────────────────────────────────────────────────────┐
│              User Applications                          │
│  (Chrome, Excel, Your Custom Software)                 │
└─────────────────────────────────────────────────────────┘
                         ↕ System Calls
┌─────────────────────────────────────────────────────────┐
│                 Operating System                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ Process  │  │  Memory  │  │   File   │             │
│  │   Mgmt   │  │   Mgmt   │  │  System  │             │
│  └──────────┘  └──────────┘  └──────────┘             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │   CPU    │  │   I/O    │  │ Network  │             │
│  │Scheduling│  │   Mgmt   │  │  Stack   │             │
│  └──────────┘  └──────────┘  └──────────┘             │
└─────────────────────────────────────────────────────────┘
                         ↕ Hardware Interface
┌─────────────────────────────────────────────────────────┐
│                    Hardware                             │
│  (CPU, Memory, Disk, Network Card)                     │
└─────────────────────────────────────────────────────────┘
```

### 1.3 Kernel vs. User Space

**Two Privilege Levels:**

```
User Space (Ring 3):
├─ Your applications run here
├─ Limited privileges
├─ Cannot directly access hardware
└─ Safer (crashes don't affect other programs)

Kernel Space (Ring 0):
├─ Operating system core runs here
├─ Full hardware access
├─ Can execute privileged instructions
└─ Dangerous (bugs can crash entire system)
```

**Transition Cost:** Switching between user and kernel space takes ~100-1000 cycles (~30-300 ns).

**Business Analogy:**
- **User space:** Regular employees doing their jobs
- **Kernel space:** Executive team with full access to company resources
- **System call:** Employee submitting a request to executives

---

## 2. System Calls: The Bridge

### 2.1 What is a System Call?

**Definition:** A request from a user program to the OS kernel.

**Common System Calls:**

```c
// File Operations
int fd = open("file.txt", O_RDONLY);  // Open file
read(fd, buffer, size);                // Read data
write(fd, buffer, size);               // Write data
close(fd);                             // Close file

// Process Operations
pid_t pid = fork();                    // Create new process
execve("/bin/ls", args, env);         // Replace process image
wait(&status);                         // Wait for child
exit(0);                               // Terminate

// Memory Operations
void* ptr = mmap(NULL, size, ...);    // Map memory
munmap(ptr, size);                     // Unmap memory

// Communication
int pipefd[2];
pipe(pipefd);                          // Create pipe
```

### 2.2 System Call Execution Flow

**Example:** `read(fd, buffer, 1024)`

```
Step 1: User Program (Ring 3)
┌────────────────────────────────────┐
│ read(fd, buffer, 1024);            │
└────────────────────────────────────┘
              ↓
Step 2: C Library (glibc)
┌────────────────────────────────────┐
│ - Prepare arguments in registers   │
│ - Load syscall number (0 for read) │
│ - Execute SYSCALL instruction      │
└────────────────────────────────────┘
              ↓
Step 3: CPU Mode Switch
┌────────────────────────────────────┐
│ - SYSCALL triggers trap            │
│ - CPU switches to Ring 0           │
│ - Saves user registers             │
│ - Jumps to kernel handler          │
└────────────────────────────────────┘
              ↓
Step 4: Kernel (Ring 0)
┌────────────────────────────────────┐
│ - Validates file descriptor        │
│ - Checks permissions               │
│ - Reads data from disk/cache       │
│ - Copies data to user buffer       │
└────────────────────────────────────┘
              ↓
Step 5: Return to User Space
┌────────────────────────────────────┐
│ - Restores user registers          │
│ - CPU switches back to Ring 3      │
│ - Returns to user program          │
└────────────────────────────────────┘
```

**Cost Breakdown:**
- Mode switch: ~100-200 cycles
- Kernel processing: ~500-5000 cycles
- **Total: ~1-10 microseconds**

**CEO Insight:** System calls are expensive! This is why:
- Buffered I/O is faster (fewer system calls)
- User-space libraries exist (avoid kernel when possible)
- Asynchronous I/O matters (don't block on system calls)

### 2.3 Real Example: File Copy

```c
// From week_1/example2-copy.c
#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *source, *destination;
    int ch;
    
    // System call: open() (wrapped by fopen)
    source = fopen("source.txt", "r");
    destination = fopen("destination.txt", "w");
    
    // System call: read() (wrapped by fgetc)
    ch = fgetc(source);
    while (ch != EOF) {
        // System call: write() (wrapped by fputc)
        fputc(ch, destination);
        ch = fgetc(source);
    }
    
    // System call: close() (wrapped by fclose)
    fclose(source);
    fclose(destination);
    
    return 0;
}
```

**What Actually Happens:**

```
User Program          C Library              Kernel
────────────          ─────────              ──────
fopen("source.txt")
                  →   open("source.txt", O_RDONLY)
                                          →  sys_open()
                                             - Allocate fd
                                             - Open file
                                          ←  return fd=3
                  ←   return FILE*
←  return FILE*

fgetc(source)
                  →   read(3, buffer, 4096)  ← Buffered!
                                          →  sys_read()
                                             - Read 4KB
                                          ←  return 4096
                  ←   return first byte
←  return 'H'

(Next 4095 fgetc calls use buffer—no syscalls!)
```

**Performance Optimization:** C library buffers data (default 4KB), so one `read()` syscall serves many `fgetc()` calls.

---

## 3. Processes: Programs in Motion

### 3.1 Definition

**Process:** A program in execution—a running instance of code.

**CEO Analogy:**
- **Program:** Recipe in a cookbook (static)
- **Process:** Chef actively cooking the recipe (dynamic)

### 3.2 Process Memory Layout

```
Process Address Space:

High Address
┌─────────────────────────────────────┐
│         Kernel Space                │  ← OS kernel (not accessible)
├─────────────────────────────────────┤  0xFFFFFFFFFFFFFFFF
│         Stack                       │  ← Local variables, function calls
│           ↓ (grows down)            │
│                                     │
│         (unused)                    │
│                                     │
│           ↑ (grows up)              │
│         Heap                        │  ← Dynamic memory (malloc)
├─────────────────────────────────────┤
│         BSS (uninitialized data)    │  ← Global variables (zero-init)
├─────────────────────────────────────┤
│         Data (initialized data)     │  ← Global variables (initialized)
├─────────────────────────────────────┤
│         Text (code)                 │  ← Program instructions (read-only)
└─────────────────────────────────────┘  0x0000000000000000
Low Address
```

**Example:**

```c
// Text segment (code)
int main() {
    // Stack (local variables)
    int x = 10;
    
    // Heap (dynamic allocation)
    int* ptr = malloc(sizeof(int));
    *ptr = 20;
    
    free(ptr);
    return 0;
}

// Data segment (initialized global)
int global_init = 42;

// BSS segment (uninitialized global)
int global_uninit;
```

### 3.3 Process Control Block (PCB)

**What the OS Tracks:**

```c
struct task_struct {  // Linux kernel (simplified)
    // Identification
    pid_t pid;                    // Process ID
    pid_t parent_pid;             // Parent process ID
    
    // CPU state
    unsigned long rip;            // Instruction pointer
    unsigned long rsp;            // Stack pointer
    unsigned long regs[16];       // General-purpose registers
    
    // Scheduling
    int priority;                 // Priority level
    unsigned long vruntime;       // Virtual runtime (CFS)
    int state;                    // RUNNING, WAITING, etc.
    
    // Memory management
    struct mm_struct *mm;         // Memory descriptor
    unsigned long brk;            // Heap end pointer
    
    // File descriptors
    struct files_struct *files;   // Open file table
    
    // Accounting
    unsigned long utime;          // User CPU time
    unsigned long stime;          // System CPU time
};
```

### 3.4 Process States

```
Process State Diagram:

┌─────────┐
│   NEW   │  ← Process being created
└────┬────┘
     │ admitted
     ↓
┌─────────┐  scheduler dispatch  ┌─────────┐
│  READY  │ ───────────────────→ │ RUNNING │
└────┬────┘                      └────┬────┘
     ↑                                │
     │ I/O or event completion        │ interrupt
     │                                ↓
     │                           ┌─────────┐
     └───────────────────────────│ WAITING │
                                 └────┬────┘
                                      │ exit
                                      ↓
                                 ┌─────────┐
                                 │TERMINATED│
                                 └─────────┘
```

**State Descriptions:**

- **NEW:** Process being created (allocating memory, loading code)
- **READY:** Ready to run, waiting for CPU
- **RUNNING:** Currently executing on a CPU core
- **WAITING:** Blocked (waiting for I/O, lock, etc.)
- **TERMINATED:** Finished execution

**Business Analogy:**
- **NEW:** Onboarding new employee
- **READY:** Employee ready to work, waiting for assignment
- **RUNNING:** Employee actively working on task
- **WAITING:** Employee waiting for information from another department
- **TERMINATED:** Employee completed project and left

### 3.5 Process Creation: fork()

**The Most Important Unix System Call**

```c
// From week_2/example3-fork.c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid;
    
    printf("Before fork: PID = %d\n", getpid());
    
    pid = fork();  // ← Magic happens here!
    
    if (pid < 0) {
        // Fork failed
        printf("fork error\n");
    }
    else if (pid == 0) {
        // Child process
        printf("Child: PID = %d, parent = %d\n", 
               getpid(), getppid());
    }
    else {
        // Parent process
        wait(NULL);  // Wait for child
        printf("Parent: PID = %d, child = %d\n", 
               getpid(), pid);
    }
    
    return 0;
}
```

**What fork() Does:**

```
Before fork():
┌─────────────────────────────────┐
│ Parent Process (PID = 1234)     │
│  - Code, data, stack, heap      │
│  - Open files, registers        │
└─────────────────────────────────┘

After fork():
┌─────────────────────────────────┐     ┌─────────────────────────────────┐
│ Parent Process (PID = 1234)     │     │ Child Process (PID = 5678)      │
│  - Code, data, stack, heap      │     │  - COPY of parent's memory      │
│  - fork() returns 5678          │     │  - fork() returns 0             │
│  - Continues execution          │     │  - Continues execution          │
└─────────────────────────────────┘     └─────────────────────────────────┘
```

**Key Points:**
1. **Child is a copy:** Gets duplicate of parent's memory
2. **Different return values:** Parent gets child PID, child gets 0
3. **Copy-on-write:** Modern OSes don't actually copy memory until written to
4. **Shared file descriptors:** Both processes share open files

**Hardware Execution:**

```
1. User calls fork():
   - SYSCALL instruction (Ring 3 → Ring 0)

2. Kernel (sys_fork):
   - Allocate new task_struct
   - Copy parent's page tables (virtual memory)
   - Mark pages as copy-on-write
   - Allocate new PID
   - Copy file descriptor table
   - Add child to scheduler's ready queue

3. Scheduler:
   - Parent and child both READY
   - May run on different cores!

4. Return to user space:
   - Parent: fork() returns child PID
   - Child: fork() returns 0
```

**Cost:** ~100-500 microseconds

---

## 4. Threads: Lightweight Concurrency

### 4.1 Process vs. Thread

**CEO Analogy:**
- **Process:** Entire company with separate office, employees, resources
- **Thread:** Employee within a company sharing the same office and resources

```
Multiple Processes:
┌─────────────────┐     ┌─────────────────┐
│   Process A     │     │   Process B     │
│  ┌───────────┐  │     │  ┌───────────┐  │
│  │   Code    │  │     │  │   Code    │  │
│  ├───────────┤  │     │  ├───────────┤  │
│  │   Data    │  │     │  │   Data    │  │
│  ├───────────┤  │     │  ├───────────┤  │
│  │   Heap    │  │     │  │   Heap    │  │
│  ├───────────┤  │     │  ├───────────┤  │
│  │  Stack    │  │     │  │  Stack    │  │
│  └───────────┘  │     │  └───────────┘  │
└─────────────────┘     └─────────────────┘
   Separate memory         Separate memory

Multiple Threads (same process):
┌─────────────────────────────────────┐
│         Process                     │
│  ┌───────────┐  ← Shared           │
│  │   Code    │                      │
│  ├───────────┤  ← Shared           │
│  │   Data    │                      │
│  ├───────────┤  ← Shared           │
│  │   Heap    │                      │
│  ├───────────┤                      │
│  │ Stack 1   │  ← Thread 1 private │
│  ├───────────┤                      │
│  │ Stack 2   │  ← Thread 2 private │
│  └───────────┘                      │
└─────────────────────────────────────┘
```

**Key Differences:**

| Aspect | Process | Thread |
|--------|---------|--------|
| **Memory** | Separate address space | Shared address space |
| **Creation cost** | High (~500 µs) | Low (~10 µs) |
| **Context switch** | Expensive (TLB flush) | Cheap (same address space) |
| **Communication** | IPC (pipes, sockets) | Shared memory (fast!) |
| **Isolation** | Strong | Weak (crash kills all) |

### 4.2 Thread Creation: pthread_create()

```c
// From week_3/example7_thread_1.cpp
#include <pthread.h>
#include <stdio.h>

void* thread_function(void* arg) {
    int id = *(int*)arg;
    printf("Thread %d running\n", id);
    return NULL;
}

int main() {
    pthread_t thread;
    int id = 1;
    
    // Create thread
    pthread_create(&thread, NULL, thread_function, &id);
    
    // Wait for thread to finish
    pthread_join(thread, NULL);
    
    return 0;
}
```

**Hardware Execution:**

```
1. pthread_create() call:
   - Allocates new stack (default 8MB)
   - Creates new task_struct in kernel
   - Shares parent's address space (CLONE_VM)
   - Adds thread to scheduler's ready queue

2. Scheduler picks thread:
   - May run on same core (time-sliced)
   - May run on different core (true parallelism!)

3. Thread execution on Intel Xeon:
   - Thread assigned to CPU core
   - Registers loaded with thread context
   - Instruction pointer → thread_function
   - Stack pointer → thread's stack
   - Execution begins!

4. pthread_join():
   - Parent blocks (futex syscall)
   - When child finishes, parent wakes
   - Parent retrieves return value
```

---

## 5. Inter-Process Communication (IPC)

### 5.1 Why IPC?

**CEO Context:** In microservices (Netflix, Amazon), services need to communicate. IPC mechanisms are the "communication protocols."

### 5.2 IPC Mechanisms

```
┌──────────────────────────────────────────────────────────┐
│                  IPC Mechanisms                           │
├──────────────────────────────────────────────────────────┤
│ 1. Pipes          │ Unidirectional byte stream          │
│ 2. FIFOs          │ Named pipes (persistent)            │
│ 3. Message Queues │ Structured messages                 │
│ 4. Shared Memory  │ Direct memory sharing (fastest!)    │
│ 5. Sockets        │ Network communication               │
│ 6. Signals        │ Asynchronous notifications          │
└──────────────────────────────────────────────────────────┘
```

### 5.3 Pipes: The Unix Way

**Concept:** One-way data channel between processes.

```c
// From week_2/example4-pipe.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];  // [0]=read end, [1]=write end
    pid_t pid;
    
    // Create pipe
    pipe(pipefd);
    
    pid = fork();
    
    if (pid == 0) {
        // Child: Reader
        close(pipefd[1]);  // Close write end
        
        FILE *output = fopen("destination.txt", "w");
        char ch;
        
        // Read from pipe, convert to lowercase
        while (read(pipefd[0], &ch, 1) > 0) {
            if (ch >= 'A' && ch <= 'Z') {
                ch = ch + 32;
            }
            fputc(ch, output);
        }
        
        fclose(output);
        close(pipefd[0]);
    }
    else {
        // Parent: Writer
        close(pipefd[0]);  // Close read end
        
        FILE *input = fopen("source.txt", "r");
        char ch;
        
        // Write to pipe
        while ((ch = fgetc(input)) != EOF) {
            write(pipefd[1], &ch, 1);
        }
        
        fclose(input);
        close(pipefd[1]);  // Signal EOF
        
        wait(NULL);
    }
    
    return 0;
}
```

**Pipe Architecture:**

```
Parent Process              Kernel              Child Process
──────────────              ──────              ─────────────
write(pipefd[1], data)
                        →   Pipe Buffer
                            (4KB default)
                                            ←   read(pipefd[0], buf)

Flow:
1. Parent writes to pipefd[1]
2. Kernel copies data to pipe buffer
3. Child reads from pipefd[0]
4. Kernel copies data from buffer to child
```

**Performance:**
- Throughput: ~10-20 GB/s (memory-to-memory)
- Latency: ~1-5 microseconds per message
- Overhead: Two context switches + two memory copies

**Business Analogy:** Conveyor belt between departments. One puts items on, the other takes them off. If belt is full, sender waits. If empty, receiver waits.

---

## Summary: Key OS Concepts

### Must-Know Facts
1. **OS roles:** Resource manager, law enforcer, service provider
2. **System call cost:** ~1-10 µs (expensive!)
3. **Process:** Program in execution, separate address space
4. **Thread:** Lightweight process, shared address space
5. **fork():** Creates child process, copy-on-write optimization
6. **Pipe:** Unidirectional IPC, kernel-buffered

### Business Takeaways
- **System calls are expensive:** Batch operations, use buffering
- **Threads vs. Processes:** Threads for shared data, processes for isolation
- **IPC overhead:** Shared memory fastest, pipes moderate, sockets slowest
- **Context switching:** Minimize by reducing thread count

### Next Steps
Continue to **Part 3: Synchronization Deep Dive** to learn how to coordinate concurrent threads safely.
