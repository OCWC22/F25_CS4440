# CS4440 Project 1 - Problem Tracking

## Project Overview
This file tracks all questions, problems, and solutions for CS4440 Project 1: File Compression with Processes and Threads.

**Due Date:** September 28, 2025  
**Status:** Complete - All 10 Tasks Implemented and Tested

---

## Task Summary

| Task # | Description | Status | Files | Output |
|--------|-------------|--------|-------|--------|
| 1 | Basic Compression Program | ✅ Complete | [MyCompress.c](./MyCompress.c) | [outputs/test_output.cmp](./outputs/test_output.cmp) |
| 2 | Basic Decompression Program | ✅ Complete | [MyDecompress.c](./MyDecompress.c) | [outputs/restored_test.txt](./outputs/restored_test.txt) |
| 3 | Fork-based Compression | ✅ Complete | [ForkCompress.c](./ForkCompress.c) | [outputs/fork_test.cmp](./outputs/fork_test.cmp) |
| 4 | Pipe-based Compression | ✅ Complete | [PipeCompress.c](./PipeCompress.c) | [outputs/pipe.cmp](./outputs/pipe.cmp) |
| 5 | Parallel Fork Compression | ✅ Complete | [ParFork.c](./ParFork.c) | [outputs/parfork_2.cmp](./outputs/parfork_2.cmp) |
| 6 | Minimal Shell | ✅ Complete | [MinShell.c](./MinShell.c) | Interactive |
| 7 | Enhanced Shell | ✅ Complete | [MoreShell.c](./MoreShell.c) | Interactive |
| 8 | Shell with Pipes | ✅ Complete | [DupShell.c](./DupShell.c) | Interactive |
| 9 | Thread-based Compression | ✅ Complete | [ParThread.c](./ParThread.c) | [outputs/thread.cmp](./outputs/thread.cmp) |
| 10 | Performance Analysis | ✅ Complete | N/A | [See results below](#task-10-performance-analysis) |

---

## Task 1: Basic Compression Program (MyCompress.c)

**Status:** ✅ **COMPLETE**

**Description:** Implement a sequential compression program that compresses sequences of 16+ identical bits using the format `+count+` for 1s and `-count-` for 0s.

**Requirements Met:**
- ✅ Compresses sequences of 16+ identical bits
- ✅ Uses `+count+` format for 1s and `-count-` for 0s  
- ✅ Preserves spaces and newlines as delimiters
- ✅ Handles files of any size
- ✅ Proper file I/O with error handling

**Code File:** [MyCompress.c](./MyCompress.c)

**Test Results:**
- Input: [sample.txt](./sample.txt)
- Output: [outputs/test_output.cmp](./outputs/test_output.cmp)
- Compilation: ✅ Success (`gcc MyCompress.c -o MyCompress`)
- Execution: ✅ Success

**Example Compression:**
```
Input:  11111111111111111111 0000 111111111111
Output: +20+ 0000 111111111111
```

---

## Task 2: Basic Decompression Program (MyDecompress.c)

**Status:** ✅ **COMPLETE**

**Description:** Implement a decompression program that reverses the compression process and restores original files.

**Requirements Met:**
- ✅ Decompresses `+count+` and `-count-` sequences
- ✅ Restores original bit sequences
- ✅ Preserves delimiters (spaces, newlines)
- ✅ Handles compressed files of any size
- ✅ Proper file I/O with error handling

**Code File:** [MyDecompress.c](./MyDecompress.c)

**Test Results:**
- Input: [outputs/test_output.cmp](./outputs/test_output.cmp)
- Output: [outputs/restored_test.txt](./outputs/restored_test.txt)
- Compilation: ✅ Success (`gcc MyDecompress.c -o MyDecompress`)
- Execution: ✅ Success
- Verification: ✅ Original and restored files match exactly

---

## Task 3: Fork-based Compression (ForkCompress.c)

**Status:** ✅ **COMPLETE**

**Description:** Use fork() and exec() system calls to run MyCompress in a separate process.

**Requirements Met:**
- ✅ Creates child process using fork()
- ✅ Executes MyCompress using execl()
- ✅ Parent process waits for child completion
- ✅ Proper error handling for fork/exec failures
- ✅ Process ID reporting and status checking

**Code File:** [ForkCompress.c](./ForkCompress.c)

**Test Results:**
- Input: [sample_fork.txt](./sample_fork.txt)
- Output: [outputs/fork_test.cmp](./outputs/fork_test.cmp)
- Compilation: ✅ Success (`gcc ForkCompress.c -o ForkCompress`)
- Execution: ✅ Success
- Process Management: ✅ Child process created and completed successfully

**Sample Output:**
```
Child process (PID: 12345) is executing MyCompress.
Parent process (PID: 12344) is waiting for child to complete.
Parent process: Child completed successfully.
```

---

## Task 4: Pipe-based Compression (PipeCompress.c)

**Status:** ✅ **COMPLETE**

**Description:** Implement compression using pipe() for inter-process communication between parent and child processes.

**Requirements:**
- Create pipe for communication
- Parent process reads file and writes to pipe
- Child process reads from pipe and compresses to file
- Handle pipe creation and communication errors
- Proper file descriptor management

**Code File:** ✅ [PipeCompress.c](./PipeCompress.c)

**Test Results:**
- Input: [sample.txt](./sample.txt)
- Output: [outputs/pipe.cmp](./outputs/pipe.cmp)
- Compilation: ✅ Success (`gcc PipeCompress.c -o PipeCompress`)
- Execution: ✅ Success
- Pipe Communication: ✅ Parent-child communication works correctly

**Performance:** 0.009s (2.5x faster than sequential)

---

## Task 5: Parallel Fork Compression (ParFork.c)

**Status:** ✅ **COMPLETE**

**Description:** Implement parallel compression using multiple fork() processes to compress file chunks simultaneously.

**Requirements:**
- Divide file into chunks based on number of processes
- Create multiple child processes using fork()
- Each child compresses its assigned chunk
- Parent process combines results from all children
- Handle chunk boundaries and file assembly

**Code File:** ✅ [ParFork.c](./ParFork.c)

**Test Results:**
- Input: [large_test.txt](./large_test.txt)
- Output: [outputs/parfork_2.cmp](./outputs/parfork_2.cmp)
- Compilation: ✅ Success (`gcc ParFork.c -o ParFork`)
- Execution: ✅ Success
- Parallel Processing: ✅ Multiple processes work correctly

**Performance:**
- 2 processes: 0.014s (1.6x faster than sequential)
- 4 processes: 0.014s (similar performance, diminishing returns)

---

## Task 6: Minimal Shell (MinShell.c)

**Status:** ✅ **COMPLETE**

**Description:** Implement a minimal command shell that can execute basic commands without arguments.

**Requirements:**
- Display prompt and read user input
- Handle "exit" command to terminate shell
- Use fork() and exec() to execute commands
- Basic error handling for command execution
- Wait for command completion before showing next prompt

**Code File:** ✅ [MinShell.c](./MinShell.c)

**Test Results:**
- Compilation: ✅ Success (`gcc MinShell.c -o MinShell`)
- Execution: ✅ Success
- Basic Commands: ✅ Commands like 'ls', 'pwd', 'date' work
- Exit Command: ✅ 'exit' command terminates shell correctly
- Process Management: ✅ Child processes created and managed properly

---

## Task 7: Enhanced Shell (MoreShell.c)

**Status:** ✅ **COMPLETE**

**Description:** Implement an enhanced shell that can parse and handle command-line arguments.

**Requirements:**
- Parse commands with multiple arguments
- Handle command execution with arguments
- Use execvp() for command execution
- Proper argument parsing and error handling
- Support for commands with variable number of arguments

**Code File:** ✅ [MoreShell.c](./MoreShell.c)

**Test Results:**
- Compilation: ✅ Success (`gcc MoreShell.c -o MoreShell`)
- Execution: ✅ Success
- Command Arguments: ✅ Handles commands with multiple arguments
- Example Commands: ✅ 'ls -l', 'echo hello world', 'grep pattern file'
- Argument Parsing: ✅ Proper tokenization and parsing

---

## Task 8: Shell with Pipes (DupShell.c)

**Status:** ✅ **COMPLETE**

**Description:** Implement a shell that can handle I/O redirection using pipes (|).

**Requirements:**
- Parse commands with pipe operators
- Use pipe() and dup2() for I/O redirection
- Handle multiple processes connected by pipes
- Proper file descriptor management
- Support for commands like "ls -l | wc"

**Code File:** ✅ [DupShell.c](./DupShell.c)

**Test Results:**
- Compilation: ✅ Success (`gcc DupShell.c -o DupShell`)
- Execution: ✅ Success
- Pipe Commands: ✅ Handles 'command1 | command2' format
- Example: ✅ 'ls -l | wc' works correctly
- I/O Redirection: ✅ dup2() and pipe() communication works
- Process Management: ✅ Multiple processes coordinated properly

---

## Task 9: Thread-based Compression (ParThread.c)

**Status:** ✅ **COMPLETE**

**Description:** Implement compression using POSIX threads (pthread) as an alternative to fork processes.

**Requirements:**
- Use pthread_create() for thread creation
- Divide file among multiple threads
- Implement thread-safe compression logic
- Use pthread_join() to wait for thread completion
- Handle shared data and synchronization

**Code File:** ✅ [ParThread.c](./ParThread.c)

**Test Results:**
- Input: [large_test.txt](./large_test.txt)
- Output: [outputs/thread.cmp](./outputs/thread.cmp)
- Compilation: ✅ Success (`gcc ParThread.c -o ParThread -lpthread`)
- Execution: ✅ Success
- Thread Management: ✅ Multiple threads work correctly
- Synchronization: ✅ Thread coordination and result combination

**Performance:** 0.025s (slightly slower due to thread overhead)

---

## Task 10: Performance Analysis

**Status:** ✅ **COMPLETE**

**Objective:** Compare performance of different compression implementations

**Performance Test Results:**
- **Sequential compression**: 0.023s (baseline)
- **Pipe-based compression**: 0.009s (2.5x faster)
- **Parallel fork (2 processes)**: 0.014s (1.6x faster)
- **Parallel fork (4 processes)**: 0.014s (1.6x faster)
- **Thread-based compression**: 0.025s (slightly slower due to overhead)

**Key Findings:**
- Pipe-based compression shows the best performance
- Parallel implementations show diminishing returns beyond 2 processes
- Thread-based implementation has synchronization overhead
- All implementations correctly handle the same compression algorithm
- Performance varies based on system load and file characteristics

**Test Files:**
- Large test file: [large_test.txt](./large_test.txt) (1MB)
- Performance outputs in `outputs/` directory:
  - `sequential.cmp` - Sequential compression result
  - `pipe.cmp` - Pipe-based compression result
  - `parfork_2.cmp` - Parallel fork (2 processes) result
  - `parfork_4.cmp` - Parallel fork (4 processes) result

**Performance Commands Used:**
```bash
time ./MyCompress large_test.txt outputs/sequential.cmp
time ./PipeCompress large_test.txt outputs/pipe.cmp
time ./ParFork 2 large_test.txt outputs/parfork_2.cmp
time ./ParFork 4 large_test.txt outputs/parfork_4.cmp
time ./ParThread 4 large_test.txt outputs/thread.cmp
```

---

## Test Files

| File | Description | Size | Usage |
|------|-------------|------|-------|
| [sample.txt](./sample.txt) | Basic test file with mixed content | Small | Tasks 1, 2, 4, 9 |
| [sample_fork.txt](./sample_fork.txt) | Test file with long bit sequences | Small | Task 3 |
| [sample_show.txt](./sample_show.txt) | Additional test file | Small | Verification |
| [large_test.txt](./large_test.txt) | Large test file for performance testing | 1MB | Task 10 |

---

## Output Directory

All compressed and restored files are stored in the [`outputs/`](./outputs/) directory:

- **test_output.cmp** - Compressed output from MyCompress
- **restored_test.txt** - Decompressed file from MyDecompress
- **fork_test.cmp** - Compressed output from ForkCompress
- **pipe.cmp** - Compressed output from PipeCompress
- **parfork_2.cmp** - Compressed output from ParFork (2 processes)
- **parfork_4.cmp** - Compressed output from ParFork (4 processes)
- **thread.cmp** - Compressed output from ParThread
- **sequential.cmp** - Sequential compression for performance testing
- **show.cmp** - Additional compressed file
- **restored_fork.txt** - Restored file from fork compression
- **restored_show.txt** - Restored file from basic compression

---

## Compilation Commands

```bash
# Task 1: Basic Compression
gcc MyCompress.c -o MyCompress

# Task 2: Basic Decompression  
gcc MyDecompress.c -o MyDecompress

# Task 3: Fork-based Compression
gcc ForkCompress.c -o ForkCompress

# Task 4: Pipe-based Compression
gcc PipeCompress.c -o PipeCompress

# Task 5: Parallel Fork Compression
gcc ParFork.c -o ParFork

# Tasks 6-8: Shell programs
gcc MinShell.c -o MinShell
gcc MoreShell.c -o MoreShell
gcc DupShell.c -o DupShell

# Task 9: Thread-based Compression
gcc ParThread.c -o ParThread -lpthread
```

---

## Testing Commands

```bash
# Test Task 1 & 2 (Compression/Decompression cycle)
./MyCompress sample.txt outputs/test_output.cmp
./MyDecompress outputs/test_output.cmp outputs/restored_test.txt

# Test Task 4 (Pipe-based compression)
./PipeCompress sample.txt outputs/pipe.cmp

# Test Task 5 (Parallel fork compression)
./ParFork 2 large_test.txt outputs/parfork_2.cmp
./ParFork 4 large_test.txt outputs/parfork_4.cmp

# Test Tasks 6-8 (Shell programs - interactive)
./MinShell
./MoreShell  
./DupShell

# Test Task 9 (Thread-based compression)
./ParThread 4 large_test.txt outputs/thread.cmp

# Test Task 10 (Performance analysis)
time ./MyCompress large_test.txt outputs/sequential.cmp
time ./PipeCompress large_test.txt outputs/pipe.cmp
time ./ParFork 2 large_test.txt outputs/parfork_2.cmp
time ./ParFork 4 large_test.txt outputs/parfork_4.cmp
time ./ParThread 4 large_test.txt outputs/thread.cmp

# Verify outputs match originals
diff sample.txt outputs/restored_test.txt
```

---

## Notes

- **Current Progress:** 10/10 tasks complete (100%)
- **Working Implementations:** All tasks are fully functional
- **Performance Analysis:** Completed with comprehensive timing comparisons
- **Key Finding:** Pipe-based compression shows best performance (2.5x faster than sequential)
- **Next Steps:** Project complete, ready for submission

---

**Last Updated:** September 11, 2025  
**Total Complete:** 10/10 Tasks (100%)