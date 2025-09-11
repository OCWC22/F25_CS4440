# CS4440 Project 1 - File Compression with Processes and Threads

## Overview
This project implements file compression using various techniques including sequential processing, parallel processing with fork(), inter-process communication with pipes, and multi-threading with pthreads.

## Project Structure
```
project_1/
├── task_1/          # Basic Compression Program
├── task_2/          # Basic Decompression Program
├── task_3/          # Fork-based Compression
├── task_4/          # Pipe-based Compression
├── task_5/          # Parallel Fork Compression
├── task_6/          # Minimal Shell
├── task_7/          # Enhanced Shell
├── task_8/          # Shell with Pipes
├── task_9/          # Thread-based Compression
├── task_10/         # Performance Analysis
├── README.md        # This file
├── PROJECT_TRACKING.md  # Comprehensive project tracking
└── project_1_due_9_28.pdf  # Original project requirements
```

## Quick Start

### Task 1: Basic Compression
```bash
cd task_1
gcc MyCompress.c -o MyCompress
./MyCompress sample.txt test_output.cmp
```

### Task 2: Basic Decompression
```bash
cd task_2
gcc MyDecompress.c -o MyDecompress
./MyDecompress ../task_1/test_output.cmp restored_test.txt
```

### Task 3: Fork-based Compression
```bash
cd task_3
gcc ForkCompress.c -o ForkCompress
./ForkCompress sample_fork.txt fork_test.cmp
```

### Task 4: Pipe-based Compression
```bash
cd task_4
gcc PipeCompress.c -o PipeCompress
./PipeCompress ../task_1/sample.txt pipe.cmp
```

### Task 5: Parallel Fork Compression
```bash
cd task_5
gcc ParFork.c -o ParFork
./ParFork 2 ../task_10/large_test.txt parfork_2.cmp
```

### Task 6: Minimal Shell
```bash
cd task_6
gcc MinShell.c -o MinShell
./MinShell
```

### Task 7: Enhanced Shell
```bash
cd task_7
gcc MoreShell.c -o MoreShell
./MoreShell
```

### Task 8: Shell with Pipes
```bash
cd task_8
gcc DupShell.c -o DupShell
./DupShell
```

### Task 9: Thread-based Compression
```bash
cd task_9
gcc ParThread.c -o ParThread -lpthread
./ParThread 4 ../task_10/large_test.txt parthread_test.cmp
```

### Task 10: Performance Analysis
```bash
cd task_10
# See README.md for detailed performance comparison
```

## Key Features

### Compression Algorithm
- Compresses sequences of 16+ identical bits
- Uses `+count+` format for 1s and `-count-` for 0s
- Preserves spaces and newlines as delimiters

### Process Management
- fork() and exec() system calls
- Inter-process communication with pipes
- Process synchronization and waiting

### Threading
- POSIX threads (pthread) for parallel processing
- Thread-safe compression implementation
- Thread synchronization with pthread_join()

### Shell Programming
- Command parsing and execution
- I/O redirection with dup2()
- Pipeline support with pipe()

## Performance Results

| Implementation | Execution Time | Speedup |
|---------------|----------------|---------|
| Sequential | 0.023s | (baseline) |
| Pipe-based | 0.009s | 2.5x faster |
| Parallel fork (2 processes) | 0.014s | 1.6x faster |
| Thread-based | 0.025s | slightly slower |

## Project Status
✅ **Complete** - All 10 tasks implemented and tested

## Files Removed (Cleaned Up)
- Dead code: Empty test.c, test.cpp, test.py files
- Duplicate files: Consolidated outputs directory
- Temporary files: Various test outputs and intermediate files

## Documentation
- Each task folder contains its own README.md
- PROJECT_TRACKING.md provides comprehensive project tracking
- Original requirements in project_1_due_9_28.pdf

## Due Date
September 28, 2025

## Completion Date
September 11, 2025