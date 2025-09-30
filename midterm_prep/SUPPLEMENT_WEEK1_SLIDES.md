# Week 1 Supplement: Complete Slide Coverage
## OS Introduction & System Calls - All Topics

**Purpose:** Complete coverage of all slides from intro_os.pdf and system_calls.pdf

---

## Part A: Operating System Introduction (Complete)

### 1. Four Components of a Computer System

```
┌─────────────────────────────────────────────────────────┐
│                    Users (1, 2, 3, ... n)               │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│     Application Programs                                │
│  (compiler, assembler, text editor, database system)    │
├─────────────────────────────────────────────────────────┤
│              Operating System                           │
├─────────────────────────────────────────────────────────┤
│            Computer Hardware                            │
└─────────────────────────────────────────────────────────┘
```

**Four Components:**
1. **Hardware:** CPU, memory, I/O devices (physical resources)
2. **Operating System:** Controls and coordinates hardware use
3. **Application Programs:** Define how resources are used (compilers, databases, games)
4. **Users:** People, machines, other computers

**CEO Analogy:** 
- **Hardware:** Office building and equipment
- **OS:** Building management (security, utilities, maintenance)
- **Applications:** Departments (accounting, HR, sales)
- **Users:** Employees and customers

---

### 2. What Operating Systems Do

**User View:**
- **PC/Laptop:** Ease of use, don't care about resource utilization
- **Mainframe/Minicomputer:** Maximize resource utilization (many users sharing)
- **Workstation:** Balance between ease of use and resource utilization
- **Handheld/Embedded:** Limited UI, mostly run without user intervention

**System View:**
- **Resource Allocator:** Manages CPU time, memory, file storage, I/O devices
- **Control Program:** Controls execution of programs to prevent errors
- **Kernel:** The one program running at all times (everything else is application)

**Business Analogy:** OS is like a city government:
- Allocates resources (roads, utilities)
- Enforces laws (prevents conflicts)
- Provides services (water, electricity)

---

### 3. Multiprogramming

**Evolution:**
```
Uni-programming → Multiprogramming → Time-sharing
```

**Multiprogramming Systems:**
- **Goal:** Maximize CPU utilization
- **Method:** Keep multiple programs in memory simultaneously
- **Benefit:** When one program waits for I/O, CPU switches to another

**Example Timeline:**
```
Program A:  [Run]      [Wait]           [Run]      [Wait]
Program B:       [Wait] [Run] [Wait]         [Run]      [Wait]
Program C:            [Wait]      [Run]           [Run]      [Wait]
Combined:   [Run] [Run] [Run] [Run] [Wait] [Run] [Run] [Run] [Wait]
            └─A─┘ └─B─┘ └─C─┘ └─A─┘        └─B─┘ └─C─┘ └─A─┘
```

**Key Insight:** If memory can hold several programs, CPU can switch whenever a program is waiting for I/O. This is **multi-tasking (multi-programming)**.

**Business Analogy:** Instead of one employee working on one task at a time (waiting for information), multiple employees work on different tasks, switching when they need to wait for something.

---

### 4. How a Modern Computer Works

**Von Neumann Architecture:**

```
┌─────────────────────────────────────────────────────────┐
│                    CPU (*N)                             │
│  ┌──────────────────────────────────────┐              │
│  │   Thread of Execution                │              │
│  │   (instruction execution cycle)      │              │
│  └──────────────────────────────────────┘              │
│         ↕                    ↕                          │
│    cache (data)         cache (instruction)            │
└─────────────────────────────────────────────────────────┘
         ↕                    ↕
    I/O request          interrupt
         ↕                    ↕
┌─────────────────────────────────────────────────────────┐
│                  Device (*M)                            │
└─────────────────────────────────────────────────────────┘
                     ↕
                    DMA
                     ↕
┌─────────────────────────────────────────────────────────┐
│              Memory                                     │
│  (instructions and data)                                │
└─────────────────────────────────────────────────────────┘
```

**Key Components:**
- **CPU:** Executes instructions from memory
- **Memory:** Stores instructions and data
- **I/O Devices:** Communicate with external world
- **Cache:** Fast memory between CPU and main memory
- **DMA (Direct Memory Access):** Devices transfer data directly to/from memory without CPU

**Instruction Execution Cycle:**
1. Fetch instruction from memory
2. Decode instruction
3. Execute instruction
4. Store result
5. Repeat

---

### 5. Storage Hierarchy

**Performance of Various Levels:**

| Level | Name | Size | Technology | Access Time | Bandwidth | Managed By | Backed By |
|-------|------|------|------------|-------------|-----------|------------|-----------|
| 1 | Registers | < 1 KB | Custom CMOS | 0.25-0.5 ns | 20,000-100,000 MB/s | Compiler | Cache |
| 2 | Cache | < 16 MB | On/off-chip SRAM | 0.5-25 ns | 5,000-10,000 MB/s | Hardware | Main memory |
| 3 | Main Memory | < 64 GB | CMOS SRAM | 80-250 ns | 1,000-5,000 MB/s | OS | Disk |
| 4 | Solid State Disk | < 1 TB | Flash memory | 25,000-50,000 ns | 500 MB/s | OS | Disk |
| 5 | Magnetic Disk | < 10 TB | Magnetic disk | 5,000,000 ns | 20-150 MB/s | OS | Disk or tape |

**Key Principle:** Movement between levels can be **explicit** (programmer-controlled) or **implicit** (automatic by hardware/OS).

**CEO Insight:** This hierarchy is a cost-performance tradeoff:
- Fast memory is expensive (registers, cache)
- Slow memory is cheap (disk, tape)
- OS manages this automatically for optimal performance

---

### 6. Operating System Services

**A View of Operating System Services:**

```
┌─────────────────────────────────────────────────────────┐
│         User and Other System Programs                  │
├─────────────────────────────────────────────────────────┤
│              User Interfaces                            │
│         (GUI, batch, command line)                      │
├─────────────────────────────────────────────────────────┤
│                 System Calls                            │
├─────────────────────────────────────────────────────────┤
│  ┌──────────┬──────────┬──────────┬──────────┬────────┐ │
│  │ Program  │   I/O    │   File   │Communi-  │Resource│ │
│  │Execution │Operations│ Systems  │cation    │Alloc   │ │
│  └──────────┴──────────┴──────────┴──────────┴────────┘ │
│  ┌──────────┐                      ┌──────────────────┐ │
│  │  Error   │                      │  Protection &    │ │
│  │Detection │                      │    Security      │ │
│  └──────────┘                      └──────────────────┘ │
│                     Services                            │
├─────────────────────────────────────────────────────────┤
│              Operating System                           │
├─────────────────────────────────────────────────────────┤
│                  Hardware                               │
└─────────────────────────────────────────────────────────┘
```

**Services for Users:**
1. **Program Execution:** Load program into memory and run it
2. **I/O Operations:** Read/write files, devices
3. **File Systems:** Create, delete, read, write files
4. **Communication:** Between processes (same computer or network)
5. **Resource Allocation:** Allocate CPU, memory, files to multiple users/jobs
6. **Accounting:** Track resource usage for billing or statistics
7. **Error Detection:** Detect and handle errors in CPU, memory, I/O devices
8. **Protection & Security:** Control access to resources, authenticate users

---

### 7. Computing Environments

#### 7.1 Traditional Computing

**Characteristics:**
- Stand-alone general-purpose machines
- But blurred as most systems interconnect (Internet)
- **Portals:** Provide web access to internal systems
- **Network computers (thin clients):** Like web terminals
- **Mobile computers:** Interconnect via wireless networks
- **Networking ubiquitous:** Even home systems use firewalls

**Business Example:** Traditional office desktop connected to company network and Internet.

#### 7.2 Client-Server Computing

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Client    │────→│   Server    │←────│   Client    │
│  (Smart PC) │     │             │     │  (Smart PC) │
└─────────────┘     └─────────────┘     └─────────────┘
                           ↕
                    ┌─────────────┐
                    │  Database   │
                    └─────────────┘
```

**Evolution:** Dumb terminals → Smart PCs

**Two Types:**
1. **Compute-server:** Provides interface for clients to request services (e.g., database queries)
2. **File-server:** Provides interface for clients to store and retrieve files

**Business Example:** 
- Web browser (client) requesting data from web server
- Email client connecting to email server

#### 7.3 Peer-to-Peer (P2P)

**Characteristics:**
- No distinction between clients and servers
- All nodes are **peers**
- Each node can act as client, server, or both

**How It Works:**
1. Node joins P2P network
2. Either:
   - Registers service with central lookup service, OR
   - Broadcasts request for service via discovery protocol

**Examples:**
- **Napster, Gnutella:** File sharing
- **Skype:** Voice over IP (VoIP)
- **BitTorrent:** Distributed file sharing

**Business Analogy:** Instead of hierarchical organization (manager → employees), it's a collaborative network where everyone contributes equally.

#### 7.4 Distributed Computing

**Definition:** Collection of separate, possibly heterogeneous, systems networked together.

**Network Types:**
- **LAN (Local Area Network):** Within building or campus
- **WAN (Wide Area Network):** Across cities or countries
- **MAN (Metropolitan Area Network):** Within a city
- **PAN (Personal Area Network):** Around a person (Bluetooth)

**Key Feature:** **Network Operating System** provides:
- Communication scheme for systems to exchange messages
- Illusion of a single system

**Most Common Protocol:** TCP/IP

**Business Example:** Company with offices in multiple cities, all connected and sharing resources.

#### 7.5 Virtualization

**Definition:** Allows operating systems to run applications within other OSes.

**Types:**

1. **Emulation:** Source CPU type different from target (e.g., PowerPC → Intel x86)
   - Generally slowest method

2. **Interpretation:** Computer language not compiled to native code
   - Moderate speed

3. **Virtualization:** OS natively compiled for CPU, running guest OSes also natively compiled
   - Example: VMware running Windows XP guests on Windows XP host
   - **VMM (Virtual Machine Manager):** Provides virtualization services

**Architecture:**
```
┌─────────────────────────────────────────────────────────┐
│  Application  │  Application  │  Application            │
├───────────────┼───────────────┼───────────────────────┤
│   Guest OS    │   Guest OS    │   Guest OS              │
│   (Linux)     │  (Windows)    │   (macOS)               │
├───────────────┴───────────────┴───────────────────────┤
│         Virtual Machine Manager (VMM)                   │
│         (VMware, VirtualBox, Hyper-V)                   │
├─────────────────────────────────────────────────────────┤
│              Host Operating System                      │
├─────────────────────────────────────────────────────────┤
│                   Hardware                              │
└─────────────────────────────────────────────────────────┘
```

**Use Cases:**
- Laptops/desktops running multiple OSes for exploration or compatibility
- Apple laptop running macOS host with Windows guest
- Developing apps for multiple OSes without multiple systems
- QA testing applications without multiple systems
- Managing compute environments within data centers

**Native VMMs (Type 1 Hypervisors):**
- VMM runs directly on hardware (no general-purpose host OS)
- Examples: VMware ESX, Citrix XenServer
- Better performance than hosted VMMs

**Business Value:** Consolidate multiple physical servers into one physical machine running multiple virtual machines. Saves hardware costs, power, cooling.

#### 7.6 Cloud Computing

**Definition:** Delivers computing, storage, even apps as a service across a network.

**Foundation:** Logical extension of virtualization (uses virtualization as base).

**Example:** Amazon EC2
- Thousands of servers
- Millions of virtual machines
- Petabytes of storage
- Available across Internet
- Pay based on usage

**Types of Cloud:**

1. **Public Cloud:** Available via Internet to anyone willing to pay
   - Examples: AWS, Azure, Google Cloud

2. **Private Cloud:** Run by a company for its own use
   - Examples: Company's internal data center

3. **Hybrid Cloud:** Combination of public and private
   - Examples: Sensitive data in private cloud, public-facing apps in public cloud

**Service Models:**

1. **SaaS (Software as a Service):** Applications via Internet
   - Examples: Gmail, Office 365, Salesforce
   - User just uses the application

2. **PaaS (Platform as a Service):** Software stack ready for application use
   - Examples: Heroku, Google App Engine
   - Developer deploys application, platform handles infrastructure

3. **IaaS (Infrastructure as a Service):** Servers or storage via Internet
   - Examples: AWS EC2, Azure VMs
   - User manages OS and applications, cloud provides hardware

**Cloud Architecture:**
```
┌─────────────────────────────────────────────────────────┐
│              Internet Connectivity                      │
│                  (Firewalls)                            │
├─────────────────────────────────────────────────────────┤
│              Load Balancers                             │
│        (Spread traffic across applications)             │
├─────────────────────────────────────────────────────────┤
│         Cloud Management Tools                          │
├─────────────────────────────────────────────────────────┤
│  Traditional OSes + VMMs + Applications                 │
└─────────────────────────────────────────────────────────┘
```

**CEO Decision Framework:**
- **Public Cloud:** Fast deployment, pay-as-you-go, no hardware management
- **Private Cloud:** More control, better security for sensitive data
- **Hybrid Cloud:** Best of both worlds, but more complex

#### 7.7 Mobile Computing

**Characteristics:**
- Handheld smartphones, tablets
- Extra features beyond traditional laptops:
  - **GPS:** Location services
  - **Gyroscope:** Motion sensing
  - **Accelerometer:** Orientation detection
  - **Fingerprint/Facial recognition:** Biometric security
  - **Cameras:** Front and back

**Connectivity:**
- **IEEE 802.11 wireless (Wi-Fi)**
- **Cellular data networks (4G, 5G)**

**Leading Platforms:**
- **Apple iOS**
- **Google Android**

**New Applications Enabled:**
- **Augmented Reality (AR):** Overlay digital info on real world
- **Location-based services:** Navigation, local search
- **Mobile payments:** Apple Pay, Google Pay
- **Health tracking:** Fitness apps, heart rate monitoring

**Business Impact:** Mobile-first design, responsive web apps, location-based marketing.

---

## Part B: System Calls (Complete)

### 1. System Call Basics

**Definition:** Programming interface to OS services.

**Key Points:**
- Typically written in C or C++
- Mostly accessed via **API (Application Programming Interface)** rather than direct system calls
- Three most common APIs:
  - **Win32 API:** Windows
  - **POSIX API:** Unix, Linux, macOS
  - **Java API:** Java Virtual Machine (JVM)

**Why Use APIs Instead of Direct System Calls?**
- **Portability:** Same API works across different OS versions
- **Ease of use:** API functions are easier to use than raw system calls
- **Abstraction:** Hide OS-specific details

---

### 2. Example of Standard API

**POSIX API Example:**

```c
// Read from file descriptor
ssize_t read(int fd, void *buf, size_t count);

Parameters:
- fd: File descriptor (integer)
- buf: Buffer to store data
- count: Number of bytes to read

Returns:
- Number of bytes read (success)
- -1 (error, check errno)
```

**Standard C Library Example:**

```c
// printf() is NOT a system call!
// It's a library function that eventually calls write()

printf("Hello, world!\n");
  ↓
vfprintf() (formatting)
  ↓
fwrite() (buffering)
  ↓
write() ← System call!
```

---

### 3. System Call Implementation

**How It Works:**

```
┌─────────────────────────────────────────────────────────┐
│  User Program                                           │
│  printf("Hello");                                       │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│  C Library (libc)                                       │
│  - Formats string                                       │
│  - Prepares syscall number                              │
│  - Loads arguments into registers                       │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│  System Call Interface                                  │
│  - Maintains table indexed by syscall number            │
│  - Number 1 = write()                                   │
│  - Number 2 = read()                                    │
│  - etc.                                                 │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│  Kernel                                                 │
│  sys_write() function executes                          │
│  - Validates parameters                                 │
│  - Writes to device/file                                │
│  - Returns status                                       │
└─────────────────────────────────────────────────────────┘
```

**Key Insight:** Each system call has a unique number. The system call interface maintains a table indexed by these numbers.

---

### 4. API – System Call – OS Relationship

**Abstraction Layers:**

```
User Program
     ↓ (calls)
API Function (e.g., printf)
     ↓ (calls)
System Call (e.g., write)
     ↓ (invokes)
Kernel Function (e.g., sys_write)
     ↓ (controls)
Hardware
```

**What the Programmer Needs to Know:**
- API specification (function name, parameters, return value)
- What the OS will do as a result

**What the Programmer Does NOT Need to Know:**
- How the system call is implemented
- Details of kernel internals
- Hardware-specific details

**Managed By:** Run-time support library (set of functions built into libraries included with compiler)

---

### 5. Types of System Calls

#### 5.1 Process Control

```c
// Create process
pid_t fork(void);

// Terminate process
void exit(int status);

// Load and execute program
int execve(const char *filename, char *const argv[], char *const envp[]);

// Wait for child process
pid_t wait(int *status);

// Get/set process attributes
pid_t getpid(void);
int nice(int inc);  // Change priority

// Allocate/free memory
void *malloc(size_t size);
void free(void *ptr);
```

**Additional Features:**
- **Dump memory if error:** Core dump for debugging
- **Debugger:** Single-step execution, breakpoints
- **Locks:** Manage access to shared data between processes

#### 5.2 File Management

```c
// Create/delete file
int creat(const char *pathname, mode_t mode);
int unlink(const char *pathname);

// Open/close file
int open(const char *pathname, int flags);
int close(int fd);

// Read/write file
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

// Reposition file pointer
off_t lseek(int fd, off_t offset, int whence);

// Get/set file attributes
int stat(const char *pathname, struct stat *buf);
int chmod(const char *pathname, mode_t mode);
```

#### 5.3 Device Management

```c
// Request/release device
int open("/dev/sda", O_RDONLY);  // Request disk device
close(fd);                        // Release device

// Read/write device
read(fd, buffer, size);
write(fd, buffer, size);

// Get/set device attributes
int ioctl(int fd, unsigned long request, ...);

// Logically attach/detach devices
int mount(const char *source, const char *target, ...);
int umount(const char *target);
```

**Key Insight:** In Unix, devices are treated as files! `/dev/sda` is a disk, `/dev/tty` is a terminal.

#### 5.4 Information Maintenance

```c
// Get/set time or date
time_t time(time_t *tloc);
int gettimeofday(struct timeval *tv, struct timezone *tz);

// Get system data
int uname(struct utsname *buf);  // System info

// Get/set process attributes
pid_t getpid(void);
uid_t getuid(void);
int getrusage(int who, struct rusage *usage);  // Resource usage
```

#### 5.5 Communications

**Two Models:**

1. **Message Passing:**
```c
// Create/delete communication connection
int socket(int domain, int type, int protocol);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
close(sockfd);

// Send/receive messages
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

2. **Shared Memory:**
```c
// Create and gain access to memory regions
int shmget(key_t key, size_t size, int shmflg);
void *shmat(int shmid, const void *shmaddr, int shmflg);

// Detach shared memory
int shmdt(const void *shmaddr);
```

**Additional:**
- Transfer status information
- Attach/detach remote devices

#### 5.6 Protection

```c
// Control access to resources
int chmod(const char *pathname, mode_t mode);
int chown(const char *pathname, uid_t owner, gid_t group);

// Get/set permissions
mode_t umask(mode_t mask);

// Allow/deny user access
int access(const char *pathname, int mode);
```

---

### 6. Examples of Windows and Unix System Calls

**Comparison Table:**

| Operation | Windows | Unix/Linux |
|-----------|---------|------------|
| **Process Control** |
| Create process | CreateProcess() | fork() |
| Exit process | ExitProcess() | exit() |
| Wait for event | WaitForSingleObject() | wait() |
| **File Manipulation** |
| Create file | CreateFile() | open() |
| Read file | ReadFile() | read() |
| Write file | WriteFile() | write() |
| Close file | CloseHandle() | close() |
| **Device Manipulation** |
| Set console mode | SetConsoleMode() | ioctl() |
| Read console | ReadConsole() | read() |
| Write console | WriteConsole() | write() |
| **Information Maintenance** |
| Get current process ID | GetCurrentProcessId() | getpid() |
| Set timer | SetTimer() | alarm() |
| Sleep | Sleep() | sleep() |
| **Communication** |
| Create pipe | CreatePipe() | pipe() |
| Create shared memory | CreateFileMapping() | shmget() |
| Map shared memory | MapViewOfFile() | shmat() |
| **Protection** |
| Set file security | SetFileSecurity() | chmod() |
| Get file security | GetFileSecurity() | stat() |

---

### 7. Standard C Library Example

**Complete Flow:**

```c
// User program
#include <stdio.h>

int main() {
    printf("Hello, world!\n");
    return 0;
}
```

**Execution:**

```
1. User Program:
   printf("Hello, world!\n");

2. C Library (printf):
   - Formats string
   - Adds newline
   - Calls write()

3. System Call (write):
   - syscall number = 1
   - fd = 1 (stdout)
   - buf = "Hello, world!\n"
   - count = 14

4. Kernel (sys_write):
   - Validates fd
   - Writes to terminal device
   - Returns 14 (bytes written)

5. Return to User:
   - printf() returns 14
   - Program continues
```

**Key Insight:** User never directly calls `write()`. The C library handles it automatically.

---

### 8. exec System Calls

**Problem:** After `fork()`, child process is a duplicate of parent. How do we run a different program in the child?

**Solution:** Use `exec()` family of system calls.

**exec Family:**

```c
// Execute with list of arguments
int execl(const char *path, const char *arg0, ..., NULL);

// Execute with list, search PATH
int execlp(const char *file, const char *arg0, ..., NULL);

// Execute with array of arguments
int execv(const char *path, char *const argv[]);

// Execute with array, search PATH
int execvp(const char *file, char *const argv[]);

// Execute with environment
int execve(const char *path, char *const argv[], char *const envp[]);
```

**Example:**

```c
#include <unistd.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        char *args[] = {"ls", "-l", NULL};
        execvp("ls", args);  // Replace child with ls
        
        // If execvp returns, it failed
        perror("execvp failed");
        exit(1);
    }
    else {
        // Parent process
        wait(NULL);  // Wait for child
    }
    
    return 0;
}
```

**What Happens:**

```
1. fork() creates child (duplicate of parent)

2. Child calls execvp("ls", args):
   - Kernel loads "ls" program from disk
   - Replaces child's memory with "ls" code
   - Resets stack, heap, data segments
   - Starts execution at "ls" main()
   
3. Child is now running "ls" program!
   - Original child code is gone
   - File descriptors remain open
   - Process ID unchanged

4. When "ls" finishes:
   - Child exits
   - Parent's wait() returns
```

**Key Insight:** `exec()` does NOT create a new process. It replaces the current process with a new program.

**Shell Example:**

```
User types: ls -l

Shell does:
1. fork()                    // Create child
2. Child: execvp("ls", ...)  // Replace with ls
3. Parent: wait()            // Wait for ls to finish
4. Shell prompt returns
```

---

## Summary: Complete Week 1 Coverage

### Operating System Concepts Covered
✅ Four components of computer system  
✅ What operating systems do (user view, system view)  
✅ Multiprogramming and time-sharing  
✅ Von Neumann architecture  
✅ Storage hierarchy (registers → disk)  
✅ OS services (program execution, I/O, file systems, etc.)  
✅ Computing environments (traditional, client-server, P2P, distributed, virtualization, cloud, mobile)  

### System Calls Covered
✅ System call basics and implementation  
✅ API vs. system call relationship  
✅ Types of system calls (process control, file management, device management, information maintenance, communications, protection)  
✅ Windows vs. Unix system call comparison  
✅ Standard C library example (printf → write)  
✅ exec family of system calls  

### Key Takeaways for Midterm

**Understand the Hierarchy:**
```
Hardware → OS → System Calls → API → User Program
```

**Know the Tradeoffs:**
- **Multiprogramming:** CPU utilization vs. complexity
- **Virtualization:** Flexibility vs. performance overhead
- **Cloud Computing:** Cost savings vs. security concerns

**Master System Calls:**
- Process control: fork(), exec(), wait(), exit()
- File management: open(), read(), write(), close()
- Communication: pipe(), socket(), shared memory

**Connect to Real World:**
- AWS EC2 = Cloud Computing (IaaS)
- Gmail = SaaS
- Your smartphone = Mobile computing environment
- Docker = Virtualization/containerization

This supplement now covers ALL slides from both PDFs with complete explanations, examples, and business context!
