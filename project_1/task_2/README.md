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

## Example Run
```bash
./MyDecompress ../task_1/test_output.cmp restored.txt
```

**Sample Input (compressed file):**
```
111111111110000101010101010101010101010101010101010101010101010101
010101 101010101-36-1
0+24+
```

**Sample Output (restored.txt):**
```
111111111110000101010101010101010101010101010101010101010101010101
010101 1010101010000000000000000000000000000000000001
0111111111111111111111111
```

## Verification
To verify the decompression works correctly:
```bash
diff ../task_1/sample.txt restored.txt
```

## Status
✅ Complete - All requirements met and tested