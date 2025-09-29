# Task 6: Minimal Shell

## Description
Implement a minimal command shell that can execute basic commands without arguments.

## Files
- `MinShell.c` - Source code
- `MinShell` - Compiled executable

## Compilation
```bash
gcc -O2 -Wall -Wextra -std=c11 MinShell.c -o MinShell
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

## Example Session
```bash
$ ./MinShell
minshell> ls
MinShell  MinShell.c  README.md
minshell> pwd
/Users/chen/Projects/F25_CS4440/project_1/task_6
minshell> date
Mon Sep 28 21:14:18 PDT 2025
minshell> exit
Exiting minishell.
$
```

## Status
✅ Complete - All requirements met and tested