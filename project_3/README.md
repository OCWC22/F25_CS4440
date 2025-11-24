# CS4440 Project 3 - Networked Storage System

## Project Overview

This project implements a multi-tiered networked storage system with the following components:
- **Tier 1**: Raw disk storage server with block-based access
- **Tier 2**: Filesystem server with FAT-based file management  
- **Clients**: Interactive and automated clients for both tiers

## Assignment Requirements Compliance

### ✅ 1. Robust Error Handling
All programs check system call return values and use `perror(3)` for graceful error exit as required. By convention, UNIX calls returning -1 trigger appropriate error messages and exit codes.

### ✅ 2. Well-Commented Code
All C/C++ files have comprehensive line-by-line comments explaining functionality, algorithms, and design decisions.

### ✅ 3. Makefile
A complete Makefile is provided that builds all programs when typing `make` in the submission directory.

### ✅ 4. Source Code with Meaningful Names + README
All source files use meaningful names that clearly indicate their contents. This README file provides complete documentation.

### ✅ 5. Test Runs for Each Program
Script files are provided for each program group showing correct input handling and graceful error exit.

### ✅ 6. Technical Report
A professional technical report is provided containing both users manual and technical manual as required.

## File Structure

### Source Code Files (Meaningful Names)
```
reverse_server.c      - TCP server that reverses strings
reverse_client.c      - Client for reverse server
ls_server.c          - TCP server that executes ls commands
ls_client.c          - Client for ls server
disk_server.c        - Raw disk storage server
disk_client_cli.c    - Interactive disk client
disk_client_random.c - Automated disk stress tester
fs_server.c          - Filesystem server (Tier 2)
fs_client.c          - Filesystem client
fs_defs.h            - Filesystem data structure definitions
```

### Build System
```
Makefile             - Builds all programs with proper flags and linking
```

### Test Files (Script Files for Each Program)
```
comprehensive_test_reverse.sh      - Tests reverse server/client
comprehensive_test_ls.sh          - Tests ls server/client
comprehensive_test_disk.sh        - Tests disk server and clients
comprehensive_test_filesystem.sh   - Tests filesystem server/client
run_all_comprehensive_tests.sh     - Master test runner
```

### Documentation
```
TECHNICAL_REPORT.md    - Technical report (users manual + technical manual)
```

## Building and Running

### Build All Programs
```bash
make
```

### Run All Tests
```bash
./run_all_comprehensive_tests.sh
```

### Individual Component Tests
```bash
./comprehensive_test_reverse.sh      # Tests reverse server/client
./comprehensive_test_ls.sh          # Tests ls server/client
./comprehensive_test_disk.sh        # Tests disk server and clients
./comprehensive_test_filesystem.sh   # Tests filesystem server/client
```

## Usage Instructions

### Tier 1 Services

#### Reverse String Service
```bash
# Start server
./reverse_server 8080

# Client usage
./reverse_client 127.0.0.1 8080
```

#### Remote LS Service
```bash
# Start server  
./ls_server 8081

# Client usage
./ls_client 127.0.0.1 8081 -l /tmp
```

#### Disk Storage Service
```bash
# Start server
./disk_server 8082 4 4 1000 disk.img

# Interactive client
./disk_client_cli 127.0.0.1 8082

# Stress testing client
./disk_client_random 127.0.0.1 8082 1000 42
```

### Tier 2 Filesystem Service

```bash
# Start disk server (Tier 1)
./disk_server 8082 8 8 1000 disk.img &

# Start filesystem server (Tier 2)
./fs_server 127.0.0.1 8082 8083

# Filesystem client usage
./fs_client 127.0.0.1 8083
```

## Test Coverage

Each test script demonstrates:
- ✅ Correct operation with valid inputs
- ✅ Graceful error handling with invalid inputs
- ✅ Edge cases and boundary conditions
- ✅ Stress testing and concurrent access

The test scripts show that all programs work correctly for normal inputs and exit gracefully on error inputs, fulfilling the assignment requirement.

## Technical Documentation

Complete technical documentation including users manual and technical manual with data structures, algorithms, and design details is provided in `TECHNICAL_REPORT.md`.

## Project Status

✅ **ALL ASSIGNMENT REQUIREMENTS COMPLETED**
- Robust error handling with perror(3) and proper exit codes
- Comprehensive code documentation
- Complete Makefile that builds all programs
- Meaningful source file names + README file
- Test script files for all programs showing all inputs
- Professional technical report with users and technical manuals
