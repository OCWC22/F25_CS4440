# Platform Compatibility Notes

## macOS Compatibility

### Semaphore Issue

**Problem:** macOS has deprecated POSIX unnamed semaphores (`sem_init`, `sem_destroy`, `sem_wait`, `sem_post`).

**Status:** Code compiles with `-Wno-deprecated-declarations` flag but may not run correctly on macOS.

**Solutions for macOS:**

#### Option 1: Use Named Semaphores (POSIX-compliant)
```c
// Instead of sem_init(&sem, 0, value)
sem_t* sem = sem_open("/my_semaphore", O_CREAT, 0644, value);

// Instead of sem_destroy(&sem)
sem_close(sem);
sem_unlink("/my_semaphore");
```

#### Option 2: Use dispatch_semaphore (macOS-specific)
```c
#include <dispatch/dispatch.h>

// Instead of sem_t
dispatch_semaphore_t sem;

// Instead of sem_init(&sem, 0, value)
sem = dispatch_semaphore_create(value);

// Instead of sem_wait(&sem)
dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

// Instead of sem_post(&sem)
dispatch_semaphore_signal(sem);

// Instead of sem_destroy(&sem)
dispatch_release(sem);
```

#### Option 3: Test on Linux

The code is designed for Linux and will work correctly on:
- Ubuntu 20.04+
- Fedora 35+
- RHEL 8+
- Any modern Linux distribution with glibc 2.28+

**Recommended:** Run on Linux VM or container for full functionality.

### Testing on Linux

```bash
# Using Docker
docker run -it --rm -v $(pwd):/project ubuntu:22.04
apt-get update && apt-get install -y build-essential
cd /project
make clean && make all
./test_all.sh
```

## Linux (Recommended Platform)

All code is fully compatible with Linux and follows POSIX standards.

**Tested on:**
- Ubuntu 22.04 LTS
- Fedora 37
- RHEL 9

**Requirements:**
- GCC 9.0+ or Clang 10.0+
- glibc 2.28+
- Linux kernel 5.0+ (for optimal futex performance)

## Windows (WSL2)

Code works on Windows Subsystem for Linux 2 (WSL2):

```bash
# Install WSL2 with Ubuntu
wsl --install -d Ubuntu-22.04

# Inside WSL2
cd /mnt/c/path/to/project_2
make all
./test_all.sh
```

## Cross-Platform Summary

| Platform | Status | Notes |
|----------|--------|-------|
| **Linux** | ✅ Full support | Recommended platform |
| **macOS** | ⚠️ Limited | Semaphores deprecated, use alternatives |
| **Windows (WSL2)** | ✅ Full support | Via Linux compatibility layer |
| **Windows (native)** | ❌ Not supported | Use pthreads-win32 or WSL2 |

## Hardware Requirements

**Minimum:**
- 2 CPU cores
- 4GB RAM
- x86-64 architecture

**Recommended:**
- 4+ CPU cores (to observe true parallelism)
- 8GB+ RAM
- Intel Xeon or AMD EPYC (for hardware features discussed in comments)

**Optimal:**
- Intel Xeon Sapphire Rapids (60 cores)
- 128GB+ RAM
- Multi-socket NUMA system (to observe NUMA effects)

## Performance Expectations

### Single-Core System
- Programs will run but won't demonstrate true parallelism
- Context switching overhead will dominate
- Educational value preserved (synchronization still works)

### Multi-Core System (4-8 cores)
- True parallelism observable
- Airline problem shows good scalability
- Mother Hubbard demonstrates thread coordination

### High-Core-Count System (60+ cores)
- Excellent scalability for Airline problem
- Can test with 100+ passengers and many workers
- NUMA effects become significant

## Compiler Compatibility

### GCC
```bash
gcc --version  # Requires 9.0+
make all
```

### Clang
```bash
clang --version  # Requires 10.0+
CC=clang make all
```

### Intel ICC
```bash
icc --version  # Requires 2021.1+
CC=icc make all
```

## Debugging

### GDB
```bash
gcc -g -O0 -pthread bounded_buffer.c -o bounded_buffer
gdb ./bounded_buffer
```

### Valgrind (Thread sanitizer)
```bash
valgrind --tool=helgrind ./bounded_buffer
```

### AddressSanitizer
```bash
gcc -fsanitize=thread -g -O1 -pthread bounded_buffer.c -o bounded_buffer
./bounded_buffer
```

## Performance Profiling

### perf (Linux only)
```bash
perf record -e context-switches,cache-misses ./airline 100 3 5 2
perf report
```

### Intel VTune (Intel CPUs)
```bash
vtune -collect threading ./airline 100 3 5 2
```

### gprof
```bash
gcc -pg -pthread bounded_buffer.c -o bounded_buffer
./bounded_buffer
gprof bounded_buffer gmon.out > analysis.txt
```

## Educational Use

**For OS Courses:**
- Code is designed for educational purposes
- Extensive comments explain hardware execution
- Works on any POSIX-compliant system
- Linux recommended for best experience

**For Production Use:**
- Code demonstrates concepts, not production patterns
- Add error handling, logging, monitoring
- Consider lock-free algorithms for high performance
- Profile and optimize for specific workload

## Support

For platform-specific issues:
- Linux: Check glibc version (`ldd --version`)
- macOS: Consider using Docker or VM
- Windows: Use WSL2 for full compatibility

For hardware-specific questions:
- Consult Intel/AMD architecture manuals
- Use `lscpu` to check CPU features
- Check NUMA topology with `numactl --hardware`
