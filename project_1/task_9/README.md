# Task 9: Thread-based Compression

## Description
Implement compression using POSIX threads (pthread) as an alternative to fork processes.

## Files
- `ParThread.c` - Source code
- `ParThread` - Compiled executable
- `parthread_test.cmp` - Test output

## Compilation
```bash
gcc ParThread.c -o ParThread -lpthread
```

## Usage
```bash
./ParThread num_threads input_file output_file
```

## Example
```bash
./ParThread 4 ../task_10/large_test.txt parthread_test.cmp
```

## Thread Management
- Uses pthread_create() for thread creation
- Divides file among multiple threads
- Implements thread-safe compression logic
- Uses pthread_join() to wait for thread completion
- Handles shared data and synchronization

## Performance
- 0.025s execution time (slightly slower due to thread overhead)

## Status
✅ Complete - All requirements met and tested