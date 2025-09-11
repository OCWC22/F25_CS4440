# Task 6: Minimal Shell

## Description
Implement a minimal command shell that can execute basic commands without arguments.

## Files
- `MinShell.c` - Source code
- `MinShell` - Compiled executable

## Compilation
```bash
gcc MinShell.c -o MinShell
```

## Usage
```bash
./MinShell
```

## Features
- Displays prompt: `minshell> `
- Handles basic commands without arguments
- Supports 'exit' command to terminate shell
- Uses fork() and execlp() for command execution
- Basic error handling for command execution
- Waits for command completion before showing next prompt

## Example Commands
```
minshell> ls
minshell> pwd
minshell> date
minshell> exit
```

## Status
✅ Complete - All requirements met and tested