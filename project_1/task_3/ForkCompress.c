/**
 * CS4440 Project 1 - Task 3: Fork-based Compression
 *
 * PURPOSE:
 * This program demonstrates process creation and management using the fork-exec pattern.
 * It creates a child process that executes the MyCompress program, while the parent
 * process waits for the child to complete. This shows how to properly manage
 * parent-child process relationships and handle process exit status.
 *
 * CONCEPTS DEMONSTRATED:
 * - Process creation using fork()
 * - Program execution using execl()
 * - Process synchronization using waitpid()
 * - Exit status handling and error propagation
 * - Proper resource cleanup and error handling
 *
 * PROCESS FLOW:
 * 1. Parent process calls fork() to create a child process
 * 2. Child process uses execl() to replace its execution context with MyCompress
 * 3. Parent process waits for child completion using waitpid()
 * 4. Parent analyzes child's exit status and reports the result
 *
 * USAGE:
 * ./ForkCompress <source_file> <dest_file>
 *
 * EXAMPLE:
 * ./ForkCompress sample.txt compressed.cmp
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 ForkCompress.c -o ForkCompress
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>

/**
 * Main function - demonstrates fork-exec pattern for process management
 * Creates a child process to run MyCompress and waits for completion
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Child's exit status, or error code on failure
 */
int main(int argc, char **argv) {
    // Validate command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return 2;
    }

    const char *src = argv[1];
    const char *dst = argv[2];

    // Create child process using fork()
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // CHILD PROCESS: Execute MyCompress program
        // execl() replaces the child process's execution context
        execl("./MyCompress", "MyCompress", src, dst, (char *)NULL);

        // If we reach here, execl() failed
        perror("execl MyCompress");
        _exit(127);  // Exit with specific error code for exec failure
    }

    // PARENT PROCESS: Wait for child to complete
    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    if (w < 0) {
        perror("waitpid");
        return 1;
    }

    // Analyze child's exit status and report results
    if (WIFEXITED(status)) {
        // Child exited normally
        int code = WEXITSTATUS(status);
        if (code == 0) {
            fprintf(stderr, "MyCompress completed successfully.\n");
        } else {
            fprintf(stderr, "MyCompress exited with code %d.\n", code);
        }
        return code;
    } else if (WIFSIGNALED(status)) {
        // Child was terminated by a signal
        int sig = WTERMSIG(status);
        fprintf(stderr, "MyCompress terminated by signal %d.\n", sig);
        return 128 + sig;
    } else {
        // Child ended unexpectedly (should not happen)
        fprintf(stderr, "MyCompress ended unexpectedly.\n");
        return 1;
    }
}
