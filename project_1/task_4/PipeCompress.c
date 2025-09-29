/**
 * CS4440 Project 1 - Task 4: Pipe-based Compression
 *
 * PURPOSE:
 * This program demonstrates inter-process communication using Unix pipes.
 * It creates a parent-child process pair where the parent reads data from
 * a source file and writes it to a pipe, while the child reads from the pipe
 * and performs compression. This shows how pipes enable efficient data
 * transfer between related processes without temporary files.
 *
 * CONCEPTS DEMONSTRATED:
 * - Pipe creation and management using pipe()
 * - Process creation and synchronization using fork() and wait()
 * - Inter-process communication through file descriptor passing
 * - I/O redirection using dup2() for stdin redirection
 * - Proper file descriptor cleanup and resource management
 *
 * PROCESS FLOW:
 * 1. Create a pipe for parent-child communication
 * 2. Parent process: reads source file and writes data to pipe
 * 3. Child process: reads from pipe and performs compression
 * 4. Parent closes write end to signal EOF to child
 * 5. Parent waits for child to complete
 *
 * USAGE:
 * ./PipeCompress <source_file> <dest_file>
 *
 * EXAMPLE:
 * ./PipeCompress sample.txt compressed.cmp
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 PipeCompress.c -o PipeCompress
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256

/**
 * Reads from pipe and performs compression, writing results to destination file
 * Implements the same RLE compression algorithm as MyCompress but reads from pipe
 * @param read_fd File descriptor to read compressed data from
 * @param dest Output file pointer to write compressed data to
 */
void compress_and_write(int read_fd, FILE *dest) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    char prev_char = EOF;  // Current character being tracked
    int count = 0;         // Count of consecutive identical characters

    while ((bytes_read = read(read_fd, buffer, BUFFER_SIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            char current_char = buffer[i];

            if (current_char == ' ' || current_char == '\n') {
                // Whitespace character - flush current run and output as-is
                if (count > 0) {
                    if (count >= 16) {
                        // Compress long runs
                        fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                    } else {
                        // Output short runs uncompressed
                        for (int j = 0; j < count; j++) {
                            fputc(prev_char, dest);
                        }
                    }
                }
                fputc(current_char, dest);
                count = 0;
                prev_char = EOF;
                continue;
            }

            if (prev_char == EOF) {
                // First character of new run
                prev_char = current_char;
                count = 1;
            } else if (current_char == prev_char) {
                // Same character - increment count
                count++;
            } else {
                // Different character - flush current run and start new one
                if (count > 0) {
                    if (count >= 16) {
                        fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                    } else {
                        for (int j = 0; j < count; j++) {
                            fputc(prev_char, dest);
                        }
                    }
                }
                prev_char = current_char;
                count = 1;
            }
        }
    }

    // Flush final run if any
    if (count > 0) {
        if (count >= 16) {
            fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
        } else {
            for (int j = 0; j < count; j++) {
                fputc(prev_char, dest);
            }
        }
    }
}

/**
 * Main function - demonstrates pipe-based inter-process communication
 * Creates parent-child processes with pipe for data transfer
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 on success, 1 on error
 */
int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <destination_file>\n", argv[0]);
        return 1;
    }

    // Create pipe for inter-process communication
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return 1;
    }

    // Create child process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // CHILD PROCESS: Compressor
        // Close write end of pipe (not needed by child)
        close(pipe_fd[1]);

        // Open destination file for writing
        FILE *dest = fopen(argv[2], "w");
        if (!dest) {
            perror("Child failed to open destination file");
            exit(1);
        }

        // Perform compression reading from pipe
        compress_and_write(pipe_fd[0], dest);

        fclose(dest);
        close(pipe_fd[0]);
        exit(0);
    } else {
        // PARENT PROCESS: File Reader
        // Close read end of pipe (not needed by parent)
        close(pipe_fd[0]);

        // Open source file for reading
        FILE *source = fopen(argv[1], "r");
        if (!source) {
            perror("Parent failed to open source file");
            exit(1);
        }

        // Read source file and write to pipe
        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source)) > 0) {
            write(pipe_fd[1], buffer, bytes_read);
        }

        fclose(source);
        close(pipe_fd[1]); // Signal EOF to child

        // Wait for child to complete
        wait(NULL);
        printf("Pipe compression complete.\n");
    }

    return 0;
} 
