# Task 5: Parallel Fork Compression

## Description
Implement parallel compression using multiple fork() processes to compress file chunks simultaneously.

## Files
- `ParFork.c` - Source code for parallel fork compression
- `ParFork` - Compiled executable
- `parfork_test.cmp` - Compressed output file

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 ParFork.c -o ParFork
```

## Usage
```bash
./ParFork num_processes input_file output_file
```

## Example Run
```bash
./ParFork 2 ../task_1/sample.txt parfork_test.cmp
```

**Sample Input (from ../task_1/sample.txt):**
```
111111111110000101010101010101010101010101010101010101010101010101
010101 1010101010000000000000000000000000000000000001
0111111111111111111111111
```

**Sample Output (parfork_test.cmp):**
```
111111111110000101010101010101010101010101010101010101010101010101
010101 101010101-36-1
0+24+
```

## Parallel Processing
- Loads entire file into memory for efficient processing
- Divides data into chunks based on number of processes
- Creates multiple child processes using fork()
- Each child compresses its assigned chunk and writes to pipe
- Parent process combines results from all children
- Proper memory management and process cleanup

## Status
✅ Complete - All requirements met and tested