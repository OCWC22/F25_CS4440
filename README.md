# CS4440 Operating Systems

## Course Description 

The course introduces fundamental concepts and elements of operating systems. The content includes the basic operating system topics such as process management (e.g., process, threads, synchronization, CPU scheduling, deadlocks), memory management (e.g., main/virtual memory) and storage management (e.g., file systems, I/O systems). There are also a few advanced topics, such as security and protection, virtual machines, distributed systems and cloud computing. The programming environments focus on Linux, but also refer to various versions of UNIX (e.g., Mac OS) and Windows. The programming languages focus on C/C++.

 
## Pre-requisites

CS2013 is required, CS3035 is highly recommended. Students should be familiar with C/C++ programming language and Linux system.

## Course Goals 

At the end of this course, each student should be able to:


- Know the overall picture of operating systems in general
- Perform CPU scheduling, interprocess communication and processes/threads synchronization
- Manage memory system, segmentation, paging and virtual memory
- Design and implement a basic file system
- Start reading more advanced/research-oriented materials on distributed systems and cloud computing

## Learning Objectives 

- Interpret the general operating system structures
- Practice with using system calls
- Perform process scheduling and basic process operations
- Demonstrate interprocess communications
- Demonstrate simple client-server communications
- Demonstrate multithreads programming
- Implement basic processes synchronization, e.g., mutex and semaphores
- Describe classical CPU scheduling algorithms
- Identify deadlocks and describe methods to avoid, prevent, detect and recover deadlocks
- Perform basic memory management operations, e.g., segmentation and paging
- Describe virtual memory allocation and management methods
- Conduct disk scheduling and management
- Design and implement a basic file system
- Define I/O systems and analyze the performance of I/O systems
- Identify security threats to OS
- Describe the principles of security and protection
- Discuss and explore virtual machines and VM allocations
- Describe the communication structures and protocols for distributed systems
- Discuss the robustness design issues of distributed systems
- Explore advanced topics related to cloud computing

## Class Schedule  

Week 1: Introduction
Week 2: Processes
Week 3: Threads
Week 4: Process Synchronization
Week 5: Deadlocks
Week 6: CPU Scheduling
Week 7: Main Memory
Week 8: Virtual Memory
Week 9: File System
Week 10: File System Implementation
Week 11: I/O Systems
Week 12: Security and Protection
Week 13: Virtual Machines
Week 14: Distributed Systems
Week 15: Final Review

## Building and Running C/C++ Code

This repository contains various C/C++ programs for learning operating systems concepts.

### Project 1: File Compression

Navigate to project_1 and run:

```bash
make all
```

This compiles all C programs for the file compression tasks (Tasks 1-9).

#### Task 1: Sequential Compression
```bash
cd project_1/task_1
./MyCompress sample.txt test_output.cmp
```

#### Task 2: Decompression
```bash
cd project_1/task_2
./MyDecompress ../task_1/test_output.cmp restored_test.txt
```

#### Task 3: Fork-based Compression
```bash
cd project_1/task_3
./ForkCompress sample_fork.txt fork_test.cmp
```

#### Task 4: Pipe-based Compression
```bash
cd project_1/task_4
./PipeCompress ../task_1/sample.txt pipe.cmp
```

#### Task 5: Parallel Fork Compression
```bash
cd project_1/task_5
./ParFork 2 ../task_10/large_test.txt parfork_2.cmp
./ParFork 4 ../task_10/large_test.txt parfork_4.cmp
```

#### Task 6: Minimal Shell
```bash
cd project_1/task_6
./MinShell
# Type commands like 'ls', 'pwd', then 'exit' to quit
```

#### Task 7: Enhanced Shell
```bash
cd project_1/task_7
./MoreShell
# Supports arguments, e.g., 'ls -l', 'echo hello world'
```

#### Task 8: Shell with Pipes
```bash
cd project_1/task_8
./DupShell
# Supports pipes, e.g., 'ls -l | wc'
```

#### Task 9: Thread-based Compression
```bash
cd project_1/task_9
./ParThread 4 ../task_10/large_test.txt parthread_test.cmp
```

#### Task 10: Performance Analysis
```bash
cd project_1/task_10
time ../task_1/MyCompress large_test.txt sequential.cmp
time ../task_4/PipeCompress large_test.txt ../task_4/pipe.cmp
time ../task_5/ParFork 2 large_test.txt ../task_5/parfork_2.cmp
time ../task_9/ParThread 4 large_test.txt ../task_9/parthread_test.cmp
```

### Other Projects

For code in other directories (e.g., week_1, week_2, src/), refer to their respective directories and README.md files for build instructions.

To run specific programs, follow the usage examples in each task's README.md.