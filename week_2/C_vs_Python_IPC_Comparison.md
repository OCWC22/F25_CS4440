# C vs Python IPC: Pipe & Process Communication

## 🎯 What We're Learning

This document compares **Inter-Process Communication (IPC)** using **pipes** and **process forking/spawning** in C vs Python. We'll learn how two processes can communicate to transform data.

## 📋 The Problem

**Goal**: Read text from `source.txt`, convert uppercase letters to lowercase, and save to `destination.txt` using two separate processes that communicate through a pipe.

**Example Input**:
```
Hello WORLD!
This is a TEST of the PIPE system.
```

**Expected Output**:
```
hello world!
this is a test of the pipe system.
```

---

## 🔧 C Implementation Deep Dive

### Headers & Setup
```c
#include <stdio.h>      // File operations (fopen, fclose, fgetc, fputc)
#include <stdlib.h>     // Memory management & exit codes
#include <unistd.h>     // System calls: pipe(), fork(), read(), write(), close()
#include <sys/types.h>  // Data types: pid_t
#include <sys/wait.h>   // Process synchronization: wait()
```

**Why these headers?**
- `<unistd.h>`: Contains all the low-level system calls for Unix/Linux
- `<sys/types.h>`: Defines system data types like `pid_t` for process IDs
- `<sys/wait.h>`: Functions to wait for child processes to finish

### Step 1: Create Pipe
```c
int pipefd[2];              // Array to hold pipe file descriptors
pid_t pid;                  // Variable to store process ID

if (pipe(pipefd) == -1) {   // Create pipe
    perror("pipe");         // Print error if pipe creation fails
    return 1;
}
```

**What happens here:**
- `pipefd[0]`: Read end of pipe (like a water pipe's intake)
- `pipefd[1]`: Write end of pipe (like a water pipe's output)
- Returns -1 if pipe creation fails (e.g., too many open files)

### Step 2: Fork Process
```c
pid = fork();

if (pid < 0) {          // Fork failed
    perror("fork");
    return 1;
}
```

**What is forking?**
- `fork()` creates an **exact copy** of the current process
- After fork, there are **two identical processes** running the same code
- The child process gets `pid = 0`
- The parent process gets `pid = child's actual process ID`

### Step 3: Child Process (Reader/Converter)
```c
if (pid == 0) {         // This is the child process
    close(pipefd[1]);   // Close write end (child only reads)

    FILE *output = fopen("destination.txt", "w");

    char ch2;
    while (read(pipefd[0], &ch2, 1) > 0) {  // Read 1 char at a time
        if (ch2 >= 'A' && ch2 <= 'Z') {     // If uppercase letter
            ch2 = ch2 + 32;                 // Convert to lowercase
        }
        fputc(ch2, output);                 // Write to file
    }

    fclose(output);
    close(pipefd[0]);   // Close read end
}
```

**Child's job:**
1. Close write end (`pipefd[1]`) - child only reads
2. Open destination file for writing
3. Read characters from pipe one by one
4. Convert uppercase to lowercase using ASCII math
5. Write converted character to file
6. Clean up: close file and pipe

### Step 4: Parent Process (Reader/Writer)
```c
} else {                 // This is the parent process
    close(pipefd[0]);    // Close read end (parent only writes)

    FILE *input = fopen("source.txt", "r");

    char ch1;
    while ((ch1 = fgetc(input)) != EOF) {  // Read 1 char at a time
        write(pipefd[1], &ch1, 1);         // Write to pipe
    }

    fclose(input);
    close(pipefd[1]);   // Close write end

    wait(NULL);         // Wait for child to finish
}
```

**Parent's job:**
1. Close read end (`pipefd[0]`) - parent only writes
2. Open source file for reading
3. Read characters from file one by one
4. Write each character to pipe
5. Clean up: close file and pipe
6. Wait for child process to complete

---

## 🐍 Python Implementation Deep Dive

### Imports & Setup
```python
import multiprocessing as mp  # Process creation & IPC
import os                     # System operations (getpid)
```

**Why these imports?**
- `multiprocessing`: High-level process management and IPC
- `os`: System-level operations like getting process IDs

### Step 1: Define Child Process Function
```python
def child_process(pipe_end, output_file):
    """Child process: reads from pipe and converts uppercase to lowercase"""
    print(f"Child process PID: {os.getpid()}")

    with open(output_file, 'w') as dest:
        while True:
            data = pipe_end.recv()          # Receive data from pipe
            if data == 'EOF':               # End of file marker
                break

            ch = data
            if 'A' <= ch <= 'Z':            # If uppercase letter
                ch = chr(ord(ch) + 32)      # Convert to lowercase

            dest.write(ch)

    pipe_end.close()
    print("Child process finished")
```

**Child function:**
- Takes pipe connection and output filename as parameters
- Uses `with open()` (context manager) for automatic file cleanup
- Receives data using `pipe_end.recv()` (vs C's `read()`)
- Converts using `chr(ord(ch) + 32)` (Python's way of ASCII math)
- Uses string `'EOF'` as end marker (C uses EOF from file)

### Step 2: Define Parent Process Function
```python
def parent_process(pipe_end, input_file):
    """Parent process: reads from file and writes to pipe"""
    print(f"Parent process PID: {os.getpid()}")

    with open(input_file, 'r') as source:
        while True:
            ch = source.read(1)             # Read 1 character
            if not ch:                      # End of file
                break

            pipe_end.send(ch)               # Send to pipe

    pipe_end.send('EOF')                    # Send end marker
    pipe_end.close()
    print("Parent process finished")
```

**Parent function:**
- Takes pipe connection and input filename as parameters
- Uses `source.read(1)` (vs C's `fgetc()`)
- Sends data using `pipe_end.send()` (vs C's `write()`)
- Sends explicit `'EOF'` marker since Python strings don't have EOF

### Step 3: Main Function (Process Orchestration)
```python
def main():
    input_file = "source.txt"
    output_file = "destination.txt"

    # Create pipe (equivalent to C's pipe() system call)
    parent_conn, child_conn = mp.Pipe()

    # Create child process (equivalent to C's fork())
    child = mp.Process(target=child_process, args=(child_conn, output_file))
    child.start()

    # Parent process work
    parent_process(parent_conn, input_file)

    # Wait for child to finish (equivalent to C's wait())
    child.join()

    print("Both processes completed!")
```

**Main orchestration:**
- `mp.Pipe()` creates bidirectional pipe (vs C's unidirectional array)
- `mp.Process()` explicitly targets a function (vs C's fork creating identical copy)
- `.start()` launches child process (vs fork's implicit creation)
- `.join()` waits for completion (vs C's `wait()`)

---

## ⚖️ Side-by-Side Comparison

| **Aspect** | **C Approach** | **Python Approach** | **Why Different?** |
|------------|----------------|-------------------|-------------------|
| **Process Creation** | `fork()` - creates identical copy | `mp.Process(target=function)` - explicit targeting | C copies everything, Python specifies what to run |
| **Pipe Creation** | `int pipefd[2]` - array of file descriptors | `parent_conn, child_conn = mp.Pipe()` - connection objects | C uses raw file descriptors, Python uses objects |
| **Data Transfer** | `read(fd, &buffer, size)` / `write(fd, &data, size)` | `conn.recv()` / `conn.send(data)` | C uses raw bytes, Python handles serialization |
| **End of Data** | File `EOF` or pipe close | Explicit `'EOF'` string marker | Python needs explicit signal since no EOF concept |
| **Process Sync** | `wait(NULL)` - system call | `process.join()` - method call | Different API design philosophies |
| **Resource Mgmt** | Manual `close()` calls | Context managers + garbage collection | Python automates cleanup |

---

## 🎓 Key Concepts to Internalize

### 1. **IPC (Inter-Process Communication)**
- **What**: Way for processes to exchange data
- **Why**: Processes have separate memory spaces
- **How**: Pipes create communication channel between processes

### 2. **Process Creation**
- **C**: `fork()` creates identical child process
- **Python**: `Process()` creates child running specific function
- **Both**: Child and parent run concurrently

### 3. **Pipe Mechanics**
- **Unidirectional in C**: `pipefd[0]` for reading, `pipefd[1]` for writing
- **Bidirectional in Python**: Both ends can send/receive
- **Both**: First-in-first-out (FIFO) data flow

### 4. **Synchronization**
- **Problem**: Parent might finish before child
- **Solution**: Parent waits for child completion
- **C**: `wait()` system call
- **Python**: `.join()` method

### 5. **Resource Management**
- **C**: Manual cleanup required (memory leaks if forgotten)
- **Python**: Automatic cleanup (garbage collection + context managers)

---

## 🚀 Why This Matters

### Real-World Applications
- **Shell pipelines**: `cat file.txt | grep "search" | sort`
- **Web servers**: Handle multiple requests concurrently
- **Data processing**: Parallel processing of large datasets
- **Microservices**: Services communicating via message queues

### Performance Considerations
- **C**: Lower overhead, direct hardware access
- **Python**: Higher level, easier development, cross-platform
- **Trade-off**: Development speed vs runtime performance

### Learning Benefits
- **Systems programming**: Understand OS-level process management
- **Concurrency**: Foundation for multi-threading/multi-processing
- **IPC patterns**: Basis for distributed systems

---

## 🛠️ Running the Examples

### C Version
```bash
gcc -o pipe_demo example4-pipe.c
./pipe_demo
cat destination.txt
```

### Python Version
```bash
python3 example4-pipe.py
cat destination.txt
```

Both produce identical output, demonstrating the same IPC concepts with different implementation approaches.

---

## 📚 Next Steps

1. **Experiment**: Modify the code to do different transformations
2. **Extend**: Add error handling and logging
3. **Compare**: Try both approaches for performance benchmarks
4. **Apply**: Use these concepts in larger multi-process applications

Remember: C gives you raw power and control, Python gives you productivity and safety. Choose based on your project's needs!

---

# 🔄 Parent and Child Process Deep Dive

## Process Hierarchy and Relationships

### Parent Process Characteristics
- **Process ID (PID)**: Unique identifier assigned by OS
- **Parent Process ID (PPID)**: Links to parent process
- **Resource Ownership**: Controls child processes and resources
- **Signal Handling**: Receives signals from children (SIGCHLD)
- **Exit Status**: Collects child termination status

### Child Process Characteristics
- **Inheritance**: Gets copy of parent's memory space (C fork)
- **Independence**: Runs concurrently with parent
- **PID/PPID**: Child gets new PID, parent's PID becomes its PPID
- **Resource Limits**: Inherits parent's limits but gets own accounting
- **Signal Handling**: Can send signals to parent

## Process Creation Mechanics

### C Fork Implementation
```c
pid_t pid = fork();
if (pid == 0) {
    // Child process code
    printf("Child PID: %d, Parent PID: %d\n", getpid(), getppid());
} else if (pid > 0) {
    // Parent process code
    printf("Parent PID: %d, Child PID: %d\n", getpid(), pid);
} else {
    // Fork failed
    perror("fork");
}
```

**What happens during fork:**
1. **Memory Duplication**: Child gets exact copy of parent's memory
2. **File Descriptor Duplication**: Child inherits open file descriptors
3. **Process Table Entry**: New entry created in process table
4. **Execution Context**: Both processes continue from fork() return

### Python Process Creation
```python
import multiprocessing as mp
import os

def child_function():
    print(f"Child PID: {os.getpid()}, Parent PID: {os.getppid()}")

if __name__ == "__main__":
    process = mp.Process(target=child_function)
    process.start()
    print(f"Parent PID: {os.getpid()}, Child PID: {process.pid}")
    process.join()
```

**What happens during Process creation:**
1. **Function Targeting**: Child runs specific function
2. **Import Isolation**: Child gets fresh import of modules
3. **Serialization**: Arguments passed via pickle serialization
4. **Resource Separation**: Clean separation from parent resources

---

# 🧵 Threads vs Processes: Understanding the Difference

## Thread Characteristics
- **Shared Memory**: All threads share same memory space
- **Lightweight**: Lower creation and context switch overhead
- **Communication**: Direct memory access (no IPC needed)
- **Synchronization**: Requires locks, semaphores for thread safety

## Process Characteristics
- **Separate Memory**: Each process has isolated memory space
- **Heavyweight**: Higher creation and context switch overhead
- **Communication**: Requires IPC mechanisms (pipes, shared memory, etc.)
- **Synchronization**: OS handles process scheduling automatically

## When to Use What

### Use Threads When:
- **Shared Data**: Need frequent data sharing between tasks
- **Lightweight Tasks**: Quick, short-running operations
- **Memory Efficiency**: Minimize memory footprint
- **Same Process Context**: Need access to same global variables

### Use Processes When:
- **Isolation**: Need strong separation between tasks
- **Stability**: One process crash shouldn't affect others
- **CPU-Bound**: Maximize CPU utilization across cores
- **Security**: Different privilege levels required

## Example Comparison

### Threading Example (Python)
```python
import threading
import time

shared_data = []

def thread_worker():
    for i in range(10):
        shared_data.append(i)
        time.sleep(0.1)

threads = []
for i in range(3):
    t = threading.Thread(target=thread_worker)
    threads.append(t)
    t.start()

for t in threads:
    t.join()

print(f"Shared data: {shared_data}")
```

### Multiprocessing Example (Python)
```python
import multiprocessing as mp

def process_worker(queue):
    for i in range(10):
        queue.put(i)

if __name__ == "__main__":
    queue = mp.Queue()
    processes = []

    for i in range(3):
        p = mp.Process(target=process_worker, args=(queue,))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()

    while not queue.empty():
        print(queue.get())
```

---

# 🔀 Forking Mechanics: The Magic Behind Process Creation

## Fork System Call Deep Dive

### Memory Copy-on-Write
```
Before Fork:
┌─────────────────┐
│ Parent Memory   │
│ - Variables     │
│ - Heap          │
│ - Stack         │
└─────────────────┘

After Fork (Initial):
┌─────────────────┐    ┌─────────────────┐
│ Parent Memory   │    │ Child Memory    │
│ - Variables     │    │ - Variables     │
│ - Heap          │    │ - Heap          │
│ - Stack         │    │ - Stack         │
└─────────────────┘    └─────────────────┘

After Modification (Copy-on-Write):
┌─────────────────┐    ┌─────────────────┐
│ Parent Memory   │    │ Child Memory    │
│ - var_a = 10    │    │ - var_a = 10    │  ← Shared until modified
│ - var_b = 20    │    │ - var_b = 25    │  ← Copied when written
└─────────────────┘    └─────────────────┘
```

### File Descriptor Inheritance
```c
// Before fork
int fd = open("file.txt", O_RDONLY);
int pipefd[2];
pipe(pipefd);

// After fork: Child inherits ALL open file descriptors
// Child can read/write to same files and pipes as parent
```

### Process Table Entries
```
Process Table After Fork:
┌─────────────┬─────────────┬─────────────┬─────────────┐
│ PID         │ PPID        │ State       │ Command     │
├─────────────┼─────────────┼─────────────┼─────────────┤
│ 1234        │ 1           │ Running     │ parent      │
│ 1235        │ 1234        │ Running     │ parent      │
└─────────────┴─────────────┴─────────────┴─────────────┘
```

## Fork Failure Scenarios

### Common Fork Failures
```c
pid_t pid = fork();
if (pid == -1) {
    // Possible reasons:
    // 1. Too many processes (ulimit -u)
    // 2. Not enough memory for process table
    // 3. System resource limits exceeded
    perror("fork failed");
    exit(1);
}
```

### Resource Limits Check
```bash
# Check current process limits
ulimit -u  # Max user processes
ulimit -v  # Max virtual memory

# Check system process table
ps aux | wc -l  # Current processes
```

---

# 🔧 Piping Internals: How Data Flows Between Processes

## Pipe Implementation Details

### Kernel Pipe Structure
```c
struct pipe_inode_info {
    wait_queue_head_t wait;      // Waiting processes
    unsigned int readers;        // Number of readers
    unsigned int writers;        // Number of writers
    unsigned int waiting_writers; // Writers waiting for readers
    struct pipe_buffer bufs[PIPE_DEF_BUFFERS]; // Data buffers
};
```

### Data Flow Mechanics
```
Parent Process                    Kernel Pipe                    Child Process
     │                                │                               │
     │  write(pipefd[1], data, size)  │                               │
     │ ──────────────────────────────►│                               │
     │                                │  Store in buffer             │
     │                                │ ────────────────────────────►│
     │                                │                               │  read(pipefd[0], buffer, size)
     │                                │                               │ ◄────────────────────────────
```

## Pipe Capacity and Blocking

### Pipe Buffer Size
```c
// Default pipe buffer size (Linux)
#define PIPE_DEF_BUFFERS 16
#define PIPE_BUF 4096  // 4KB per buffer

// Check pipe buffer size
long pipe_size = fcntl(fd, F_GETPIPE_SZ);
```

### Blocking Behavior
```c
// Writing to full pipe (no readers)
write(pipefd[1], data, large_size);
// Process BLOCKS until reader consumes data

// Reading from empty pipe (no writers)
read(pipefd[0], buffer, size);
// Process BLOCKS until writer provides data
```

### Non-Blocking Mode
```c
// Set non-blocking mode
int flags = fcntl(pipefd[0], F_GETFL);
fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

// Now read/write won't block
ssize_t result = read(pipefd[0], buffer, size);
if (result == -1 && errno == EAGAIN) {
    // No data available, but won't block
}
```

## Named Pipes (FIFOs)

### Creating Named Pipes
```c
// Create named pipe
if (mkfifo("my_pipe", 0666) == -1) {
    perror("mkfifo");
}

// Use like regular files
int fd = open("my_pipe", O_WRONLY);
write(fd, data, size);

int fd2 = open("my_pipe", O_RDONLY);
read(fd2, buffer, size);
```

### Named Pipe Advantages
- **Persistence**: Survives after processes terminate
- **Multi-Process**: Any process can connect
- **Filesystem**: Appears as file in directory listing

---

# 📡 IPC Patterns and Best Practices

## Common IPC Patterns

### Producer-Consumer Pattern
```c
// Producer (writes to pipe)
while (has_data) {
    char data = get_next_item();
    write(pipefd[1], &data, sizeof(char));
}
close(pipefd[1]);  // Signal end of data

// Consumer (reads from pipe)
while (read(pipefd[0], &data, sizeof(char)) > 0) {
    process_item(data);
}
```

### Request-Response Pattern
```c
// Client
write(pipefd[1], request, sizeof(request));
read(pipefd[0], response, sizeof(response));

// Server
read(pipefd[0], request, sizeof(request));
process_request(request);
write(pipefd[1], response, sizeof(response));
```

### Pipeline Pattern
```bash
# Shell pipeline
cat file.txt | grep "pattern" | sort | uniq

# Equivalent in code
cat_process -> grep_process -> sort_process -> uniq_process
```

## Best Practices

### 1. Error Handling
```c
// Always check system calls
if (pipe(pipefd) == -1) {
    perror("pipe failed");
    exit(1);
}

if (fork() == -1) {
    perror("fork failed");
    exit(1);
}
```

### 2. Resource Cleanup
```c
// Close unused pipe ends immediately
if (pid == 0) {
    close(pipefd[1]);  // Child doesn't write
    // ... child work ...
    close(pipefd[0]);
} else {
    close(pipefd[0]);  // Parent doesn't read
    // ... parent work ...
    close(pipefd[1]);
}
```

### 3. Signal Handling
```c
// Handle SIGCHLD to prevent zombie processes
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

signal(SIGCHLD, sigchld_handler);
```

### 4. Buffer Management
```c
// Use appropriate buffer sizes
#define BUFFER_SIZE 4096
char buffer[BUFFER_SIZE];

// Handle partial reads/writes
ssize_t bytes_read;
while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
    // Process bytes_read bytes
    total_processed += bytes_read;
}
```

### 5. Synchronization
```c
// Use waitpid for specific child
pid_t child_pid = fork();
if (child_pid > 0) {
    int status;
    waitpid(child_pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Child exited with code: %d\n", WEXITSTATUS(status));
    }
}
```

---

# 🔄 Process States and Lifecycle

## Process States

### Linux Process States
```c
#define TASK_RUNNING    0  // Running or runnable
#define TASK_INTERRUPTIBLE 1  // Sleeping, can be interrupted
#define TASK_UNINTERRUPTIBLE 2  // Sleeping, cannot be interrupted
#define TASK_STOPPED    4  // Stopped by signal
#define TASK_TRACED     8  // Being traced
#define EXIT_ZOMBIE     16 // Zombie process
#define EXIT_DEAD       32 // Dead process
```

### State Transitions
```
Created ────► Ready ────► Running ────► Terminated
     │          │           │
     │          │           │
     └──────────┼───────────┼───► Waiting
                │           │
                └───────────┼───► Stopped
                            │
                            └────► Zombie
```

## Process Lifecycle

### Process Creation to Termination
```c
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();  // 1. Process created

    if (pid == 0) {
        // 2. Child process running
        sleep(2);        // 3. Child waiting
        exit(42);        // 4. Child terminates
    } else {
        // 5. Parent continues
        int status;
        wait(&status);   // 6. Parent waits for child
        // 7. Child becomes zombie, then reaped
        printf("Child exited with: %d\n", WEXITSTATUS(status));
    }

    return 0;
}
```

### Zombie Process Prevention
```c
// Method 1: Wait for specific child
waitpid(child_pid, &status, 0);

// Method 2: Wait for any child
while (wait(NULL) > 0);

// Method 3: Signal handler
void zombie_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
signal(SIGCHLD, zombie_handler);
```

---

# 💾 Memory and Resource Sharing

## Memory Sharing Models

### No Sharing (Default)
```
Parent Memory: [Variables, Heap, Stack]
Child Memory:  [Variables, Heap, Stack]  ← Complete copy
```

### Shared Memory (Advanced)
```c
#include <sys/shm.h>

// Create shared memory segment
key_t key = ftok("shared_memory", 123);
int shmid = shmget(key, 1024, IPC_CREAT | 0666);

// Attach to process memory
char *shared_memory = (char*) shmat(shmid, NULL, 0);

// Use shared memory
strcpy(shared_memory, "Hello from parent!");

// Detach
shmdt(shared_memory);
```

### Memory-Mapped Files
```c
#include <sys/mman.h>
#include <fcntl.h>

// Create memory-mapped file
int fd = open("shared.dat", O_RDWR | O_CREAT, 0666);
ftruncate(fd, 1024);

char *mapped = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
// Now both processes can access same memory region
```

## File Descriptor Inheritance

### Inherited File Descriptors
```c
// Before fork
FILE *log_file = fopen("app.log", "a");
int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

// After fork: Child inherits both descriptors
// Child can write to same log file and use same socket
```

### File Descriptor Table
```
Process File Descriptor Table:
┌─────┬─────────────────┬─────────┐
│ FD  │ File Pointer    │ Offset  │
├─────┼─────────────────┼─────────┤
│ 0   │ stdin           │ 0       │
│ 1   │ stdout          │ 0       │
│ 2   │ stderr          │ 0       │
│ 3   │ log_file        │ 1024    │
│ 4   │ socket_fd       │ 0       │
│ 5   │ pipefd[0]       │ 0       │
│ 6   │ pipefd[1]       │ 0       │
└─────┴─────────────────┴─────────┘
```

---

# 🚨 Error Handling in IPC

## Common IPC Errors and Solutions

### Pipe Errors
```c
int pipefd[2];
if (pipe(pipefd) == -1) {
    switch (errno) {
        case EMFILE:  // Too many open files
            fprintf(stderr, "Too many open file descriptors\n");
            break;
        case ENFILE:  // System file table full
            fprintf(stderr, "System file table full\n");
            break;
        default:
            perror("pipe");
    }
    exit(1);
}
```

### Fork Errors
```c
pid_t pid = fork();
if (pid == -1) {
    switch (errno) {
        case EAGAIN:  // System process limit reached
            fprintf(stderr, "Cannot fork: process limit reached\n");
            break;
        case ENOMEM:  // Not enough memory
            fprintf(stderr, "Cannot fork: insufficient memory\n");
            break;
        default:
            perror("fork");
    }
    exit(1);
}
```

### Read/Write Errors
```c
char buffer[1024];
ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer));

if (bytes_read == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Non-blocking read, no data available
        usleep(1000);  // Wait and retry
    } else if (errno == EINTR) {
        // Interrupted by signal, retry
        continue;
    } else {
        perror("read failed");
        exit(1);
    }
} else if (bytes_read == 0) {
    // Pipe closed by writer
    break;
}
```

## Robust IPC Error Handling Pattern
```c
#define RETRY_MAX 3
#define RETRY_DELAY_MS 100

int robust_write(int fd, const void *buf, size_t count) {
    int retries = 0;
    ssize_t written = 0;

    while (written < count && retries < RETRY_MAX) {
        ssize_t result = write(fd, (char*)buf + written, count - written);

        if (result == -1) {
            if (errno == EINTR) {
                // Interrupted, retry
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Would block, wait and retry
                usleep(RETRY_DELAY_MS * 1000);
                retries++;
                continue;
            } else {
                // Fatal error
                return -1;
            }
        }

        written += result;
    }

    return written;
}
```

---

# ⚡ Advanced IPC Techniques

## Message Queues
```c
#include <sys/msg.h>

// Define message structure
struct message {
    long msg_type;
    char msg_text[100];
};

// Create message queue
key_t key = ftok("msg_queue", 123);
int msgid = msgget(key, IPC_CREAT | 0666);

// Send message
struct message msg = {1, "Hello from sender!"};
msgsnd(msgid, &msg, sizeof(msg.msg_text), 0);

// Receive message
msgrcv(msgid, &msg, sizeof(msg.msg_text), 1, 0);
printf("Received: %s\n", msg.msg_text);
```

## Shared Memory with Semaphores
```c
#include <sys/shm.h>
#include <semaphore.h>

// Create shared memory
int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
int *shared_counter = (int*) shmat(shmid, NULL, 0);

// Create semaphore for synchronization
sem_t *semaphore = sem_open("/counter_sem", O_CREAT, 0644, 1);

// Protected access
sem_wait(semaphore);
(*shared_counter)++;
sem_post(semaphore);
```

## Sockets for IPC
```c
#include <sys/socket.h>
#include <sys/un.h>

// Create Unix domain socket
int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

struct sockaddr_un addr;
addr.sun_family = AF_UNIX;
strcpy(addr.sun_path, "/tmp/my_socket");

// Server
bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
listen(sockfd, 5);

// Client
connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
```

## Performance Comparison

### IPC Method Performance (Relative)
| Method | Speed | Complexity | Use Case |
|--------|-------|------------|----------|
| **Pipes** | Fast | Low | Parent-child communication |
| **Shared Memory** | Fastest | High | High-throughput data sharing |
| **Message Queues** | Medium | Medium | Structured message passing |
| **Sockets** | Medium | Medium | Network-style communication |
| **Files** | Slow | Low | Persistent data exchange |

### Choosing the Right IPC Method
- **Speed Critical**: Shared memory with semaphores
- **Simple Parent-Child**: Pipes
- **Complex Messages**: Message queues
- **Network-like**: Unix domain sockets
- **Persistence**: Named pipes or files

---

# 🎯 Complete IPC Reference

## System Calls Summary

| **System Call** | **Purpose** | **C Function** | **Python Equivalent** |
|----------------|-------------|----------------|----------------------|
| `pipe()` | Create pipe | `pipe(int fd[2])` | `multiprocessing.Pipe()` |
| `fork()` | Create child | `fork()` | `multiprocessing.Process()` |
| `read()` | Read from pipe | `read(int fd, void *buf, size_t count)` | `conn.recv()` |
| `write()` | Write to pipe | `write(int fd, const void *buf, size_t count)` | `conn.send(data)` |
| `wait()` | Wait for child | `wait(int *status)` | `process.join()` |
| `close()` | Close descriptor | `close(int fd)` | `conn.close()` |

## Best Practice Checklist

### ✅ Always Do
- [ ] Check return values of all system calls
- [ ] Close unused pipe ends immediately
- [ ] Handle SIGCHLD to prevent zombies
- [ ] Use appropriate buffer sizes
- [ ] Clean up resources properly

### ❌ Never Do
- [ ] Forget to close pipe ends
- [ ] Ignore system call errors
- [ ] Use busy waiting instead of blocking
- [ ] Share file descriptors without synchronization
- [ ] Assume processes will terminate in order

## Debugging IPC Issues

### Common Problems and Solutions
1. **Deadlock**: Parent and child both waiting
   - **Solution**: Ensure one process reads, other writes

2. **Zombie Processes**: Unwaited child processes
   - **Solution**: Always wait for children or handle SIGCHLD

3. **Broken Pipe**: Writing to closed pipe
   - **Solution**: Check if reader exists before writing

4. **Race Conditions**: Timing-dependent bugs
   - **Solution**: Use proper synchronization primitives

This comprehensive guide covers everything you need to understand and implement robust parent-child process communication using forking, piping, and IPC in both C and Python!
