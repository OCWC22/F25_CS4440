# Task 8: Shell with Pipes

## Description
Implement a shell that can handle I/O redirection using pipes (|).

## Files
- `DupShell.c` - Source code
- `DupShell` - Compiled executable

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 DupShell.c -o DupShell
```

## Usage
```bash
./DupShell
```

## Features
- Displays prompt: `dupshell> `
- Parses commands with pipe operators
- Uses pipe() and dup2() for I/O redirection
- Handles multiple processes connected by pipes
- Proper file descriptor management
- Support for commands like "ls -l | wc"
- Supports 'exit' command to terminate shell

## Example Session
```bash
$ ./DupShell
dupshell> ls -l | wc -l
3
dupshell> echo "hello world" | wc -c
12
dupshell> cat /dev/null | wc -l
0
dupshell> exit
Exiting dupshell.
$
```

## Process Management
- Creates pipe for inter-process communication
- Forks processes for each command in pipeline
- Uses dup2() to redirect stdin/stdout appropriately
- Properly manages file descriptors and cleanup
- Waits for all processes to complete

## Status
✅ Complete - All requirements met and tested