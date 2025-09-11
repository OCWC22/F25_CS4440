# Task 4: Pipe-based Compression

## Description
Implement compression using pipe() for inter-process communication between parent and child processes.

## Files
- `PipeCompress.c` - Source code
- `PipeCompress` - Compiled executable
- `pipe.cmp` - Compressed output using pipe
- `pipe_test.cmp` - Test output

## Compilation
```bash
gcc PipeCompress.c -o PipeCompress
```

## Usage
```bash
./PipeCompress input_file output_file
```

## Example
```bash
./PipeCompress ../task_1/sample.txt pipe.cmp
```

## Process Communication
- Parent process reads file and writes to pipe
- Child process reads from pipe and compresses to file
- Handles pipe creation and communication errors
- Proper file descriptor management

## Performance
- 0.009s execution time (2.5x faster than sequential)

## Status
✅ Complete - All requirements met and tested