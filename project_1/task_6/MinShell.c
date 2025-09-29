/**
 * CS4440 Project 1 - Task 6: Minimal Shell
 *
 * PURPOSE:
 * This program implements a basic command-line shell that reads user commands
 * from stdin and executes them using the execlp() system call. It demonstrates
 * fundamental shell concepts including command input, process creation, and
 * command execution with PATH resolution.
 *
 * CONCEPTS DEMONSTRATED:
 * - Command-line interface design and user interaction
 * - Process creation and execution using fork() and execlp()
 * - Basic command parsing and validation
 * - Process synchronization using wait()
 * - Error handling for command execution failures
 * - Interactive shell loop with proper exit handling
 *
 * FEATURES:
 * - Interactive command prompt with "minishell> " display
 * - Basic command execution with PATH resolution
 * - Exit command support ("exit" or Ctrl+D)
 * - Empty command handling
 * - Error messages for command not found
 *
 * LIMITATIONS:
 * - Does not support command arguments
 * - Does not handle complex commands with pipes or redirection
 * - Simple command parsing (treats entire line as single command)
 *
 * USAGE:
 * Run the program and enter commands at the "minishell> " prompt.
 * Type "exit" or press Ctrl+D to quit the shell.
 *
 * EXAMPLE:
 * minishell> ls
 * [file listing]
 * minishell> pwd
 * /current/directory
 * minishell> exit
 * Exiting minishell.
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 MinShell.c -o MinShell
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define PROMPT "minishell> "
#define MAX_CMD_LEN 100

/**
 * Main function - implements the minimal shell with basic command execution
 * Provides interactive command prompt and executes single commands
 * @return 0 on successful exit
 */
int main(void) {
    char cmd[MAX_CMD_LEN];  // Buffer for command input

    while (1) {
        // Display prompt and get user input
        printf("%s", PROMPT);
        fflush(stdout);

        if (fgets(cmd, MAX_CMD_LEN, stdin) == NULL) {
            printf("\n");
            break; // Exit on EOF (Ctrl+D)
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

        // Create child process to execute command
        pid_t pid = fork();
        if (pid == 0) {
            // CHILD PROCESS: Execute the command
            // execlp() searches for command in PATH
            execlp(cmd, cmd, NULL);

            // If execlp returns, the command was not found
            fprintf(stderr, "minishell: %s: command not found\n", cmd);
            exit(1);
        } else if (pid > 0) {
            // PARENT PROCESS: Wait for child to complete
            int status;
            wait(&status);

            // Check if command failed (execlp already showed error message)
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                // Command failed, but error already displayed by child
            }
        } else {
            // FORK FAILED: Display error and continue
            perror("fork failed");
        }
    }

    printf("Exiting minishell.\n");
    return 0;
}
