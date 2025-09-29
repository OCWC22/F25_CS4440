# Task 2: Basic Decompression Program

## Description
Implement a decompression program that reverses the compression process and restores original files.

## Files
- `MyDecompress.c` - Source code for the decompression program
- `MyDecompress` - Compiled executable
- `restored.txt` - Decompressed output file

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 MyDecompress.c -o MyDecompress
```

## Usage
```bash
./MyDecompress compressed_file output_file
```

## Example
```bash
./MyDecompress ../task_1/test_output.cmp restored.txt
```

## Verification
To verify the decompression works correctly:
```bash
diff ../task_1/sample.txt restored.txt
```

## Status
✅ Complete - All requirements met and tested