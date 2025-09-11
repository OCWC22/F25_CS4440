# Task 10: Performance Analysis

## Description
Compare performance of different compression implementations.

## Files
- `large_test.txt` - Large test file (1MB) for performance testing
- `sequential.cmp` - Sequential compression result

## Performance Test Results

| Implementation | Execution Time | Speedup |
|---------------|----------------|---------|
| Sequential compression | 0.023s | (baseline) |
| Pipe-based compression | 0.009s | 2.5x faster |
| Parallel fork (2 processes) | 0.014s | 1.6x faster |
| Parallel fork (4 processes) | 0.014s | 1.6x faster |
| Thread-based compression | 0.025s | slightly slower |

## Key Findings
- Pipe-based compression shows the best performance
- Parallel implementations show diminishing returns beyond 2 processes
- Thread-based implementation has synchronization overhead
- All implementations correctly handle the same compression algorithm
- Performance varies based on system load and file characteristics

## Testing Commands
```bash
# Sequential (Task 1)
time ../task_1/MyCompress large_test.txt sequential.cmp

# Pipe-based (Task 4)
time ../task_4/PipeCompress large_test.txt ../task_4/pipe.cmp

# Parallel fork (Task 5)
time ../task_5/ParFork 2 large_test.txt ../task_5/parfork_2.cmp
time ../task_5/ParFork 4 large_test.txt ../task_5/parfork_4.cmp

# Thread-based (Task 9)
time ../task_9/ParThread 4 large_test.txt ../task_9/parthread_test.cmp
```

## Status
✅ Complete - All performance analysis completed