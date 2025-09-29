/**
 * CS4440 Project 1 - Task 7: Enhanced Shell
 *
 * PURPOSE:
 * This program implements an enhanced command-line shell that supports
 * command argument parsing and execution. It reads user commands from
 * stdin, parses them into separate arguments, and executes them using
 * the execvp() system call which searches for commands in the PATH.
 *
 * CONCEPTS DEMONSTRATED:
 * - Command-line interface design and user interaction
 * - String parsing and tokenization for command arguments
 * - Process creation and execution using fork() and execvp()
 * - Command execution with proper PATH resolution
 * - Error handling for command not found scenarios
 * - Process synchronization using wait()
 *
 * FEATURES:
 * - Interactive command prompt with "moreshell> " display
 * - Support for commands with multiple arguments
 * - Proper PATH resolution using execvp()
 * - Graceful handling of command execution failures
 * - Exit command to terminate the shell
 * - Empty command handling
 *
 * USAGE:
 * Run the program and enter commands at the "moreshell> " prompt.
 * Type "exit" to quit the shell.
 *
 * EXAMPLE:
 * moreshell> echo "Hello World"
 * Hello World
 * moreshell> ls -la
 * [file listing]
 * moreshell> exit
 * Exiting moreshell.
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 MoreShell.c -o MoreShell
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define PROMPT "moreshell> "
#define MAX_CMD_LEN 256
#define MAX_ARGS 32

/**
 * Parses a command string into individual arguments
 * Tokenizes the command string using spaces and tabs as delimiters
 * @param cmd Command string to parse (will be modified)
 * @param args Array to store parsed arguments (null-terminated)
 */
void parse_args(char *cmd, char **args) {
    int i = 0;
    char *token = strtok(cmd, " \t\n");

    while (token != NULL && i < MAX_ARGS - 1) {
        args[i] = token;
        i++;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL; // Null-terminate the argument list
}

/**
 * Main function - implements the enhanced shell with argument parsing
 * Provides interactive command prompt and executes user commands
 * @return 0 on successful exit
 */
int main(void) {
    char cmd[MAX_CMD_LEN];  // Buffer for command input
    char *args[MAX_ARGS];   // Array for command arguments

    while (1) {
        // Display prompt and get user input
        printf("%s", PROMPT);
        fflush(stdout);

        if (fgets(cmd, MAX_CMD_LEN, stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove trailing newline character
        cmd[strcspn(cmd, "\n")] = 0;

        // Skip empty commands
        if (strlen(cmd) == 0) {
            continue;
        }

        // Check for exit command
        if (strcmp(cmd, "exit") == 0) {
            break;
        }

        // Parse command into arguments
        parse_args(cmd, args);

        if (args[0] == NULL) {
            continue; // Empty command after parsing
        }

        // Create child process to execute command
        pid_t pid = fork();
        if (pid == 0) {
            // CHILD PROCESS: Execute the command
            // execvp() searches for command in PATH and passes arguments
            execvp(args[0], args);

            // If execvp returns, the command was not found
            fprintf(stderr, "moreshell: %s: command not found\n", args[0]);
            exit(1);
        } else if (pid > 0) {
            // PARENT PROCESS: Wait for child to complete
            int status;
            wait(&status);

            // Check if command failed (execvp already showed error message)
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                // Command failed, but error already displayed by child
            }
        } else {
            // FORK FAILED: Display error and continue
            perror("fork failed");
        }
    }

    printf("Exiting moreshell.\n");
    return 0;
}
