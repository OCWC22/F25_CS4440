# Task 4: Pipe-based Compression

## Description
Implement compression using pipe() for inter-process communication between parent and child processes.

## Files
- `PipeCompress.c` - Source code for pipe-based compression
- `PipeCompress` - Compiled executable
- `pipe_test.cmp` - Compressed output file

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 PipeCompress.c -o PipeCompress
```

## Usage
```bash
./PipeCompress input_file output_file
```

## Example Run
```bash
./PipeCompress ../task_1/sample.txt pipe_test.cmp
```

**Sample Input (from ../task_1/sample.txt):**
```
111111111110000101010101010101010101010101010101010101010101010101
010101 1010101010000000000000000000000000000000000001
0111111111111111111111111
```

**Sample Output (pipe_test.cmp):**
```
111111111110000101010101010101010101010101010101010101010101010101
010101 101010101-36-1
0+24+
```

## Process Communication
- Creates pipe for parent-child communication
- Parent process reads file and writes data to pipe
- Child process reads from pipe and performs compression
- Handles pipe creation and communication errors
- Proper file descriptor management and cleanup

## Status
✅ Complete - All requirements met and tested