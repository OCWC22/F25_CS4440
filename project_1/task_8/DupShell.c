/**
 * CS4440 Project 1 - Task 8: Shell with Pipes
 *
 * PURPOSE:
 * This program implements a command-line shell that specifically handles
 * piped commands (command1 | command2). It demonstrates pipe creation,
 * I/O redirection using dup2(), and coordination of multiple processes
 * in a pipeline where the output of one command becomes the input of another.
 *
 * CONCEPTS DEMONSTRATED:
 * - Pipe creation and management using pipe()
 * - I/O redirection using dup2() for stdin/stdout
 * - Multi-process coordination in command pipelines
 * - Command parsing for pipe operators
 * - Process synchronization using waitpid()
 * - Error handling for complex command execution
 *
 * PIPELINE EXECUTION:
 * 1. Parse command to identify pipe operator ("|")
 * 2. Split command into two separate command argument lists
 * 3. Create pipe for inter-process communication
 * 4. Fork first child process to execute first command:
 *    - Redirect stdout to write end of pipe
 *    - Execute command1
 * 5. Fork second child process to execute second command:
 *    - Redirect stdin to read end of pipe
 *    - Execute command2
 * 6. Parent waits for both children to complete
 *
 * USAGE:
 * Run the program and enter piped commands at the "dupshell> " prompt.
 * Only piped commands are supported (command1 | command2).
 * Type "exit" to quit the shell.
 *
 * EXAMPLE:
 * dupshell> echo "Hello World" | wc -c
 * 12
 * dupshell> ls -la | grep ".c"
 * -rw-r--r--  1 user  staff   3599 Sep 28 21:55 DupShell.c
 * dupshell> exit
 * Exiting dupshell.
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 DupShell.c -o DupShell
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define PROMPT "dupshell> "
#define MAX_CMD_LEN 256
#define MAX_ARGS 32

/**
 * Parses a command string to identify and split piped commands
 * Looks for pipe operator "|" and splits command into two argument lists
 * @param cmd Command string to parse (will be modified)
 * @param cmd1_args Array to store first command arguments
 * @param cmd2_args Array to store second command arguments
 * @return 1 if pipe found and parsed successfully, 0 otherwise
 */
int parse_pipe_command(char *cmd, char **cmd1_args, char **cmd2_args) {
    char *pipe_pos = strchr(cmd, '|');
    if (pipe_pos == NULL) {
        return 0; // No pipe found
    }

    *pipe_pos = '\0'; // Split the command at the pipe

    // Parse first command (before pipe)
    int i = 0;
    char *token = strtok(cmd, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        cmd1_args[i] = token;
        i++;
        token = strtok(NULL, " \t\n");
    }
    cmd1_args[i] = NULL;

    // Parse second command (after pipe)
    i = 0;
    token = strtok(pipe_pos + 1, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        cmd2_args[i] = token;
        i++;
        token = strtok(NULL, " \t\n");
    }
    cmd2_args[i] = NULL;

    return 1; // Pipe found and parsed successfully
}

/**
 * Main function - implements shell with pipe command support
 * Only accepts piped commands (command1 | command2) for execution
 * @return 0 on successful exit
 */
int main(void) {
    char cmd[MAX_CMD_LEN];              // Buffer for command input
    char *cmd1_args[MAX_ARGS];          // Arguments for first command
    char *cmd2_args[MAX_ARGS];          // Arguments for second command

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

        // Parse command for pipe operator
        if (!parse_pipe_command(cmd, cmd1_args, cmd2_args)) {
            printf("This shell only handles piped commands. Usage: command1 | command2\n");
            continue;
        }

        // Validate that both commands are present
        if (cmd1_args[0] == NULL || cmd2_args[0] == NULL) {
            printf("Invalid piped command format. Usage: command1 | command2\n");
            continue;
        }

        // Create pipe for inter-process communication
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            perror("pipe failed");
            continue;
        }

        // Create first child process for command1
        pid_t pid1 = fork();
        if (pid1 == 0) {
            // FIRST CHILD: Execute command1
            close(pipe_fd[0]); // Close read end of pipe

            // Redirect stdout to write end of pipe
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);

            // Execute first command
            execvp(cmd1_args[0], cmd1_args);
            fprintf(stderr, "dupshell: %s: command not found\n", cmd1_args[0]);
            exit(1);
        } else if (pid1 < 0) {
            perror("fork failed for first command");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            continue;
        }

        // Create second child process for command2
        pid_t pid2 = fork();
        if (pid2 == 0) {
            // SECOND CHILD: Execute command2
            close(pipe_fd[1]); // Close write end of pipe

            // Redirect stdin from read end of pipe
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);

            // Execute second command
            execvp(cmd2_args[0], cmd2_args);
            fprintf(stderr, "dupshell: %s: command not found\n", cmd2_args[0]);
            exit(1);
        } else if (pid2 < 0) {
            perror("fork failed for second command");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            continue;
        }

        // PARENT PROCESS: Close pipe ends and wait for children
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        // Wait for both children to complete
        int status1, status2;
        waitpid(pid1, &status1, 0);
        waitpid(pid2, &status2, 0);
    }

    printf("Exiting dupshell.\n");
    return 0;
}
