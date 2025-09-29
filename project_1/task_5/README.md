# Task 5: Parallel Fork Compression

## Description
Implement parallel compression using multiple fork() processes to compress file chunks simultaneously.

## Files
- `ParFork.c` - Source code for parallel fork compression
- `ParFork` - Compiled executable
- `MyCompress` - Copy of compression executable
- `task5_output.cmp` - Compressed output file

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 ParFork.c -o ParFork
```

## Usage
```bash
./ParFork num_processes input_file output_file
```

## Example
```bash
./ParFork 2 ../task_10/large_test.txt task5_output.cmp
```

## Parallel Processing
- Divides file into chunks based on number of processes
- Creates multiple child processes using fork()
- Each child compresses its assigned chunk
- Parent process combines results from all children
- Handles chunk boundaries and file assembly

## Status
✅ Complete - All requirements met and tested