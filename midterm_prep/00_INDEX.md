# Operating Systems Crash Course: Complete Midterm Guide
## From Silicon to Software - A CEO's Technical Bridge

**Purpose:** Comprehensive OS understanding for midterm + AWS infrastructure decisions  
**Audience:** CTOs, Engineers, OS Students preparing for exams  
**Approach:** Hardware → OS → Real Code → Business Decisions

---

## Course Structure

### Part 0: Week 1 Complete Slide Coverage
📄 **[SUPPLEMENT_WEEK1_SLIDES.md](SUPPLEMENT_WEEK1_SLIDES.md)** ⭐ NEW!
- ALL slides from intro_os.pdf and system_calls.pdf
- Four components of computer system
- Multiprogramming and time-sharing
- Computing environments (traditional, client-server, P2P, distributed, virtualization, cloud, mobile)
- Complete system call types and examples
- exec() family explained
- Windows vs. Unix system call comparison

### Part 1: Hardware Foundation
📄 **[01_HARDWARE_FUNDAMENTALS.md](01_HARDWARE_FUNDAMENTALS.md)**
- Transistors → Logic Gates → CPU Architecture
- Intel Xeon Sapphire Rapids deep dive
- Memory hierarchy (L1/L2/L3/RAM)
- Cache coherency (MESI protocol)
- Why this matters for AWS EC2 instance selection

### Part 2: Operating System Core
📄 **[02_OS_FUNDAMENTALS.md](02_OS_FUNDAMENTALS.md)**
- What is an OS? (Resource manager analogy)
- System calls (user space ↔ kernel space)
- Processes vs. Threads
- Process lifecycle and scheduling
- IPC mechanisms (pipes, shared memory)

### Part 3: Concurrency & Synchronization
📄 **[03_SYNCHRONIZATION_DEEP_DIVE.md](03_SYNCHRONIZATION_DEEP_DIVE.md)**
- Race conditions and why they happen
- Mutex implementation (software → hardware)
- Semaphores and counting synchronization
- Producer-Consumer problem (complete solution)
- Deadlock prevention strategies
- Hardware atomic operations (LOCK CMPXCHG)

### Part 4: Real-World Applications
📄 **[04_REAL_WORLD_APPLICATIONS.md](04_REAL_WORLD_APPLICATIONS.md)**
- AWS EC2 instance selection framework
- Performance optimization strategies
- NUMA-aware programming
- Lock-free algorithms
- Complete code walkthrough with hardware execution

### Part 5: Code Examples
📄 **[05_CODE_EXAMPLES_ANNOTATED.md](05_CODE_EXAMPLES_ANNOTATED.md)**
- All course code with line-by-line hardware explanations
- fork() example with kernel internals
- pipe() example with IPC flow
- mutex example with cache coherency
- Bounded buffer with complete synchronization

---

## Quick Reference Cards

### CPU Architecture Quick Facts
```
Intel Xeon Sapphire Rapids:
- 60 cores, 120 threads (SMT)
- L1: 80KB/core, L2: 2MB/core, L3: 112.5MB shared
- Cache line: 64 bytes
- Memory: DDR5, up to 4TB
- TDP: 225-350W
```

### System Call Costs
```
Operation                    Cost
────────────────────────────────────
Register access              1 cycle
L1 cache hit                 4 cycles
L2 cache hit                 12 cycles
L3 cache hit                 40 cycles
RAM access                   100-200 cycles
System call overhead         100-1000 cycles
Context switch               1-10 microseconds
```

### Synchronization Primitives
```
Primitive       Use Case                    Cost (uncontended)
────────────────────────────────────────────────────────────────
Mutex           Mutual exclusion            20-50 cycles
Semaphore       Counting/signaling          20-50 cycles
Spinlock        Very short critical section 10-20 cycles
Atomic ops      Lock-free algorithms        5-10 cycles
```

---

## Study Strategy for Midterm

### Week 1-2 Topics
- ✅ OS introduction and services
- ✅ Four components of computer system
- ✅ Multiprogramming, time-sharing, von Neumann architecture
- ✅ Storage hierarchy (registers → cache → RAM → disk)
- ✅ Computing environments (client-server, P2P, distributed, virtualization, cloud, mobile)
- ✅ System calls (open, read, write, fork, exec)
- ✅ Process concept and lifecycle
- ✅ C programming fundamentals

**Key Files to Review:**
- `SUPPLEMENT_WEEK1_SLIDES.md` - Complete slide coverage ⭐
- `week_1/example1-ls.c` - execvp() usage
- `week_1/example2-copy.c` - File I/O system calls
- `week_2/example3-fork.c` - Process creation

### Week 3-4 Topics
- ✅ Inter-Process Communication (IPC)
- ✅ Pipes and FIFOs
- ✅ Threads introduction
- ✅ pthread library basics

**Key Files to Review:**
- `week_2/example4-pipe.c` - Pipe communication
- `week_3/example7_thread_1.cpp` - Thread creation
- `week_3/example8_thread_2.cpp` - Thread synchronization

### Week 5-6 Topics
- ✅ Mutex locks
- ✅ Semaphores
- ✅ Producer-Consumer problem
- ✅ Deadlock

**Key Files to Review:**
- `week_5/example13_mutex.cpp` - Mutex usage
- `week_6/example14_semaphore.cpp` - Semaphore usage
- `project_2/bounded_buffer.c` - Complete solution

### Week 7 Topics
- ✅ Deadlock detection and prevention
- ✅ Classical synchronization problems

---

## Business Decision Framework

### When to Choose Intel Xeon (vs AMD/ARM)
**Choose Intel Xeon Sapphire Rapids when:**
- Need maximum single-thread performance
- AVX-512 workloads (AI/ML, scientific computing)
- Established software ecosystem compatibility
- Advanced features (AMX, DSA, IAA)

**Choose AMD EPYC when:**
- Need more cores per socket (up to 96)
- Cost-sensitive workloads
- Memory bandwidth critical

**Choose ARM Graviton when:**
- Cost optimization (40% cheaper)
- Energy efficiency matters
- Cloud-native workloads

### Scaling Strategy
```
Vertical Scaling (Scale Up):
- Single large instance (e.g., c7i.48xlarge)
- Pros: Simple, low latency
- Cons: Single point of failure, expensive

Horizontal Scaling (Scale Out):
- Multiple smaller instances (e.g., 10× c7i.4xlarge)
- Pros: Fault-tolerant, flexible
- Cons: More complex, network overhead

CEO Decision: Start vertical, scale horizontal
```

---

## Exam Preparation Checklist

### Concepts You Must Know
- [ ] Process vs. Thread (memory layout, creation cost)
- [ ] System call execution flow (user → kernel → hardware)
- [ ] fork() behavior (return values, memory copying)
- [ ] Pipe communication (read/write ends, blocking)
- [ ] Mutex vs. Semaphore (when to use each)
- [ ] Deadlock conditions (4 necessary conditions)
- [ ] Cache coherency (MESI states)
- [ ] Context switch cost (direct + indirect)

### Code You Must Be Able to Write
- [ ] fork() to create child process
- [ ] pipe() for parent-child communication
- [ ] pthread_create() for thread creation
- [ ] pthread_mutex_lock/unlock for critical sections
- [ ] sem_wait/sem_post for synchronization
- [ ] Producer-Consumer with bounded buffer

### Problems You Must Be Able to Solve
- [ ] Identify race conditions in code
- [ ] Fix synchronization bugs
- [ ] Detect potential deadlocks
- [ ] Calculate context switch overhead
- [ ] Optimize cache usage

---

## Additional Resources

### From Your Course Materials
- `Abraham Silberschatz-Operating System Concepts (9th,2012_12).pdf` - Textbook
- `week_1/EssentialC.pdf` - C programming reference
- `project_1/` - Process and IPC implementations
- `project_2/` - Synchronization implementations

### Hardware Architecture
- `project_2/hardware_architecture_guide.md` - Intel Xeon deep dive
- `project_2/synchronization_primitives_explained.md` - Mutex/semaphore internals

### Performance Analysis
- `project_1/task_10/README.md` - Performance comparison results
- `project_2/PLATFORM_NOTES.md` - macOS vs Linux differences

---

## Contact & Questions

**Study Tips:**
1. **Understand, don't memorize:** Focus on "why" not "what"
2. **Draw diagrams:** Visualize process states, memory layouts
3. **Run code:** Modify examples, observe behavior
4. **Think in analogies:** Connect to business/real-world scenarios
5. **Practice problems:** Write code from scratch

**Common Pitfalls:**
- Confusing process and thread
- Forgetting to close pipe ends
- Incorrect mutex lock ordering (deadlock!)
- Not checking system call return values
- Ignoring cache effects in performance analysis

---

**Next Steps:** Start with Part 1 (Hardware Fundamentals) to build foundation, then progress through each part sequentially.
