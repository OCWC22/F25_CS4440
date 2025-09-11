# Task 7: Enhanced Shell

## Description
Implement an enhanced shell that can parse and handle command-line arguments.

## Files
- `MoreShell.c` - Source code
- `MoreShell` - Compiled executable

## Compilation
```bash
gcc MoreShell.c -o MoreShell
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

## Example Commands
```
moreshell> ls -l
moreshell> echo hello world
moreshell> grep pattern file
moreshell> cat file.txt | wc -l
moreshell> exit
```

## Status
✅ Complete - All requirements met and tested