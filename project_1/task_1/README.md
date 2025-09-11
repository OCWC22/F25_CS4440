# Task 1: Basic Compression Program

## Description
Implement a sequential compression program that compresses sequences of 16+ identical bits using the format `+count+` for 1s and `-count-` for 0s.

## Files
- `MyCompress.c` - Source code
- `MyCompress` - Compiled executable
- `sample.txt` - Test input file
- `sample_show.txt` - Additional test file
- `test_output.cmp` - Compressed output
- `show.cmp` - Additional compressed output
- `compressed.txt` - Test compression result

## Compilation
```bash
gcc MyCompress.c -o MyCompress
```

## Usage
```bash
./MyCompress input_file output_file
```

## Example
```bash
./MyCompress sample.txt test_output.cmp
```

## Status
✅ Complete - All requirements met and tested