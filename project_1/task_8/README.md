# Task 8: Shell with Pipes

## Description
Implement a shell that can handle I/O redirection using pipes (|).

## Files
- `DupShell.c` - Source code
- `DupShell` - Compiled executable

## Compilation
```bash
gcc DupShell.c -o DupShell
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

## Example Commands
```
dupshell> ls -l | wc
dupshell> cat file.txt | grep pattern
dupshell> ps aux | grep process
dupshell> exit
```

## Process Management
- Creates pipe for inter-process communication
- Forks processes for each command in pipeline
- Uses dup2() to redirect stdin/stdout
- Properly manages file descriptors
- Waits for all processes to complete

## Status
✅ Complete - All requirements met and tested