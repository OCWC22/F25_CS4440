# Task 2: Basic Decompression Program

## Description
Implement a decompression program that reverses the compression process and restores original files.

## Files
- `MyDecompress.c` - Source code
- `MyDecompress` - Compiled executable
- `restored_test.txt` - Decompressed output from Task 1
- `restored_show.txt` - Decompressed output from additional test

## Compilation
```bash
gcc MyDecompress.c -o MyDecompress
```

## Usage
```bash
./MyDecompress compressed_file output_file
```

## Example
```bash
./MyDecompress ../task_1/test_output.cmp restored_test.txt
```

## Verification
To verify the decompression works correctly:
```bash
diff original_file restored_file
```

## Status
✅ Complete - All requirements met and tested