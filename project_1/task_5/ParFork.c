/**
 * CS4440 Project 1 - Task 5: Parallel Fork Compression
 *
 * PURPOSE:
 * This program demonstrates parallel processing using multiple child processes.
 * It divides a large input file into chunks and assigns each chunk to a separate
 * child process for compression. Each child compresses its chunk independently
 * and sends the result back through a pipe to the parent, which combines all
 * results into the final output file.
 *
 * CONCEPTS DEMONSTRATED:
 * - Parallel processing with multiple child processes
 * - Work distribution and load balancing across processes
 * - Multiple pipe management for inter-process communication
 * - Memory-mapped file processing for efficiency
 * - Process pool management and synchronization
 *
 * ALGORITHM:
 * 1. Read entire input file into memory buffer
 * 2. Calculate chunk size based on number of processes
 * 3. Create N child processes, each with its own pipe
 * 4. Each child compresses its assigned chunk and writes to pipe
 * 5. Parent reads from all child pipes and combines results
 * 6. Parent waits for all children to complete
 *
 * USAGE:
 * ./ParFork <source_file> <dest_file> <num_processes>
 *
 * EXAMPLE:
 * ./ParFork large_file.txt compressed.cmp 4
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 ParFork.c -o ParFork
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/**
 * Compresses a buffer of data and writes compressed output to file
 * Uses the same RLE algorithm as the sequential compressor
 * @param s Pointer to the data buffer to compress
 * @param len Length of the data to compress
 * @param w Output file pointer to write compressed data to
 */
void comp_buf(char *s, long len, FILE *w) {
    char prev = 0;    // Previous character in current run
    long count = 0;   // Count of consecutive identical characters

    for (long i = 0; i < len; i++) {
        char curr = s[i];

        if (curr == '0' || curr == '1') {
            // Binary digit - process for compression
            if (count == 0) {
                // Start new run
                prev = curr;
                count = 1;
            } else if (curr == prev) {
                // Continue current run
                count++;
            } else {
                // End current run and start new one
                if (count >= 16) {
                    // Compress long runs
                    fprintf(w, "%c%ld%c", prev == '1' ? '+' : '-', count, prev == '1' ? '+' : '-');
                } else {
                    // Output short runs uncompressed
                    for (long j = 0; j < count; j++) {
                        fputc(prev, w);
                    }
                }
                prev = curr;
                count = 1;
            }
        } else {
            // Non-binary character - flush current run and output as-is
            if (count >= 16) {
                fprintf(w, "%c%ld%c", prev == '1' ? '+' : '-', count, prev == '1' ? '+' : '-');
            } else {
                for (long j = 0; j < count; j++) {
                    fputc(prev, w);
                }
            }
            fputc(curr, w);
            prev = 0;
            count = 0;
        }
    }

    // Flush final run if any
    if (count >= 16) {
        fprintf(w, "%c%ld%c", prev == '1' ? '+' : '-', count, prev == '1' ? '+' : '-');
    } else {
        for (long j = 0; j < count; j++) {
            fputc(prev, w);
        }
    }
}

/**
 * Main function - implements parallel compression using multiple processes
 * Divides work among N child processes and combines their outputs
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 on success, 1 on error
 */
int main(int argc, char** argv) {
    // Validate command line arguments
    if (argc != 4) {
        fprintf(stderr, "usage: %s src dst nproc\n", argv[0]);
        return 1;
    }

    // Open input file and read entire contents into memory
    FILE* in = fopen(argv[1], "r");
    if (!in) {
        perror(argv[1]);
        return 1;
    }
    fseek(in, 0, SEEK_END);
    long N = ftell(in);  // Get file size
    fseek(in, 0, SEEK_SET);

    char *buf = malloc(N);  // Allocate buffer for entire file
    if (!buf) {
        perror("malloc");
        return 1;
    }
    fread(buf, 1, N, in);
    fclose(in);

    // Parse number of processes, with minimum of 1
    int n = atoi(argv[3]);
    if (n < 1) n = 1;

    // Calculate chunk size for equal distribution
    long chunk = (N + n - 1) / n;

    // Allocate arrays for pipes and process IDs
    int (*pp)[2] = malloc(sizeof(int[2]) * n);
    pid_t *pids = malloc(sizeof(pid_t) * n);

    // Create N child processes
    for (int i = 0; i < n; i++) {
        pipe(pp[i]);  // Create pipe for this child

        if ((pids[i] = fork()) == 0) {
            // CHILD PROCESS: Compress assigned chunk
            close(pp[i][0]);  // Close read end

            // Open write end of pipe as FILE stream
            FILE* w = fdopen(pp[i][1], "w");

            // Calculate chunk boundaries for this process
            long s = (long)i * chunk, e = s + chunk;
            if (e > N) e = N;  // Handle last chunk

            // Compress chunk if it has data
            if (s < N) comp_buf(buf + s, e - s, w);

            fclose(w);
            _exit(0);  // Exit child process
        }

        close(pp[i][1]);  // Parent closes write end
    }

    // PARENT PROCESS: Collect results from all children
    FILE* out = fopen(argv[2], "w");
    for (int i = 0; i < n; i++) {
        // Open read end of pipe as FILE stream
        FILE* r = fdopen(pp[i][0], "r");

        // Read all data from this child's pipe
        int c;
        while ((c = fgetc(r)) != EOF) fputc(c, out);

        fclose(r);
        waitpid(pids[i], NULL, 0);  // Wait for this child
    }
    fclose(out);

    // Clean up allocated memory
    free(buf);
    free(pp);
    free(pids);
    return 0;
}
