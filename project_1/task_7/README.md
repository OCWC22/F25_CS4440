# Task 7: Enhanced Shell

## Description
Implement an enhanced shell that can parse and handle command-line arguments.

## Files
- `MoreShell.c` - Source code
- `MoreShell` - Compiled executable

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 MoreShell.c -o MoreShell
```

## Usage
```bash
./MoreShell
```

## Features
- Displays prompt: `moreshell> `
- Parses commands with multiple arguments
- Handles command execution with arguments
- Uses execvp() for command execution
- Proper argument parsing and error handling
- Support for commands with variable number of arguments
- Supports 'exit' command to terminate shell

## Example Session
```bash
$ ./MoreShell
moreshell> ls -l
total 16
-rwxr-xr-x 1 chen staff 34248 Sep 28 21:14 MoreShell
-rw-r--r-- 1 chen staff  1745 Sep 28 21:14 MoreShell.c
-rw-r--r-- 1 chen staff   810 Sep 28 21:14 README.md
moreshell> echo hello world
hello world
moreshell> wc -l MoreShell.c
18 MoreShell.c
moreshell> exit
Exiting moreshell.
$
```

## Status
✅ Complete - All requirements met and tested