# Task 3: Fork-based Compression

## Description
Use fork() and exec() system calls to run MyCompress in a separate process.

## Files
- `ForkCompress.c` - Source code for fork-based compression
- `ForkCompress` - Compiled executable
- `MyCompress` - Copy of the compression executable for execution
- `sample_fork.txt` - Test input file
- `fork_test.cmp` - Compressed output file

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 ForkCompress.c -o ForkCompress
```

## Usage
```bash
./ForkCompress input_file output_file
```

## Example
```bash
./ForkCompress sample_fork.txt fork_test.cmp
```

## Process Management
- Creates child process using fork()
- Executes MyCompress using execl()
- Parent process waits for child completion
- Proper error handling for fork/exec failures

## Status
✅ Complete - All requirements met and tested