# Task 1: Basic Compression Program

## Description
Implement a sequential compression program that compresses sequences of 16+ identical bits using the format `+count+` for 1s and `-count-` for 0s.

## Files
- `MyCompress.c` - Source code for the compression program
- `MyCompress` - Compiled executable
- `sample.txt` - Test input file containing binary data
- `test_output.cmp` - Compressed output file

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 MyCompress.c -o MyCompress
```

## Usage
```bash
./MyCompress input_file output_file
```

## Example Run
```bash
./MyCompress sample.txt test_output.cmp
```

**Sample Input (sample.txt):**
```
111111111110000101010101010101010101010101010101010101010101010101
010101 1010101010000000000000000000000000000000000001
0111111111111111111111111
```

**Sample Output (test_output.cmp):**
```
111111111110000101010101010101010101010101010101010101010101010101
010101 101010101-36-1
0+24+
```

## Status
✅ Complete - All requirements met and tested