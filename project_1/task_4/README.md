# Task 4: Pipe-based Compression

## Description
Implement compression using pipe() for inter-process communication between parent and child processes.

## Files
- `PipeCompress.c` - Source code for pipe-based compression
- `PipeCompress` - Compiled executable
- `pipe_test.cmp` - Compressed output file
- `task4_output.cmp` - Additional test output

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 PipeCompress.c -o PipeCompress
```

## Usage
```bash
./PipeCompress input_file output_file
```

## Example
```bash
./PipeCompress ../task_1/sample.txt pipe_test.cmp
```

## Process Communication
- Parent process reads file and writes to pipe
- Child process reads from pipe and compresses to file
- Handles pipe creation and communication errors
- Proper file descriptor management

## Status
✅ Complete - All requirements met and tested