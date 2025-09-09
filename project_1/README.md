# CS4440 Project 1: File Compression with Processes and Threads

## Project Overview
This project implements a file compression utility that can compress and decompress files containing '0's and '1's using both process-based and thread-based approaches.

## Implemented Solutions

### 1. Basic Compression/Decompression
- **Files**: 
  - `MyCompress.c` - Compression program
  - `MyDecompress.c` - Decompression program
- **Output**: 
  - Compressed files have `.cmp` extension
  - Decompressed files restore original content

### 2. Fork-based Implementation
- **Files**:
  - `ForkCompress.c` - Uses `fork()` for parallel compression
  - `sample_fork.txt` - Sample input file for testing
- **Output**:
  - `outputs/fork_out.cmp` - Compressed output
  - `outputs/restored_fork.txt` - Restored output

### 3. Additional Implementations
- `compress.py` - Python implementation of the compression algorithm
- `test.c` - Test file for basic functionality

## How to Compile and Run

### Compilation
```bash
gcc MyCompress.c -o MyCompress
gcc MyDecompress.c -o MyDecompress
gcc ForkCompress.c -o ForkCompress
```

### Running the Programs
1. **Basic Compression**:
   ```bash
   ./MyCompress input.txt compressed.cmp
   ./MyDecompress compressed.cmp output.txt
   ```

2. **Fork-based Compression**:
   ```bash
   ./ForkCompress sample_fork.txt outputs/fork_out.cmp
   ```

## Test Files
- `sample.txt` - Basic test file
- `sample_show.txt` - Test file for verification
- `sample_fork.txt` - Test file for fork-based implementation

## Output Files
Output files are stored in the `outputs/` directory:
- `compressed.txt` - Basic compression output
- `fork_out.cmp` - Fork-based compression output
- `restored_fork.txt` - Restored file from fork-based compression
- `restored_show.txt` - Restored file from basic compression
- `show.cmp` - Compressed output from basic implementation

## Notes
- The project includes both C and Python implementations
- The `Makefile` can be used for easier compilation
- Test files are provided for verification

## Project Structure
```
project_1/
├── MyCompress.c      # Main compression program
├── MyDecompress.c    # Decompression program
├── ForkCompress.c    # Fork-based implementation
├── compress.py       # Python implementation
├── outputs/          # Directory for output files
├── sample*.txt      # Test input files
└── README.md        # This file
```
