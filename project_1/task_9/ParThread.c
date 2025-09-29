/**
 * CS4440 Project 1 - Task 9: Thread-based Compression
 *
 * PURPOSE:
 * This program demonstrates multithreaded parallel processing using POSIX threads.
 * It divides a large input file into chunks and assigns each chunk to a separate
 * thread for compression. All threads share the same memory space and coordinate
 * through mutexes to ensure thread-safe file access and output writing.
 *
 * CONCEPTS DEMONSTRATED:
 * - POSIX threads (pthreads) for parallel processing
 * - Shared memory model with mutex synchronization
 * - Thread pool management and work distribution
 * - Thread-safe file access using mutexes
 * - Memory management for thread-local buffers
 * - Proper thread cleanup and resource management
 *
 * ALGORITHM:
 * 1. Get file size and calculate chunk size based on thread count
 * 2. Create N threads, each responsible for one chunk
 * 3. Each thread:
 *    - Opens file and seeks to its chunk start position
 *    - Compresses its chunk into a thread-local buffer
 *    - Uses mutex to safely write results to output file
 * 4. Main thread waits for all threads and combines results
 *
 * USAGE:
 * ./ParThread <num_threads> <source_file> <dest_file>
 *
 * EXAMPLE:
 * ./ParThread 4 large_file.txt compressed.cmp
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 -pthread ParThread.c -o ParThread
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

// Struct to pass arguments to thread function
typedef struct {
    const char* in_file;        // Input file path
    char* out_buffer;          // Thread-local output buffer
    long start;                // Start position in file
    long size;                 // Size of chunk to process
    long bytes_written;        // Number of bytes written to buffer
    pthread_mutex_t* file_mutex;   // Mutex for file access
    pthread_mutex_t* output_mutex; // Mutex for output writing
} thread_args_t;

/**
 * Thread function that compresses a chunk of the input file
 * Each thread processes its assigned chunk independently and writes to thread-local buffer
 * @param args Thread arguments containing file info and synchronization primitives
 * @return NULL (void* return for pthread compatibility)
 */
void* compress_chunk_thread(void *args) {
    thread_args_t *t_args = (thread_args_t*)args;

    // Use mutex to protect file access (multiple threads may open same file)
    pthread_mutex_lock(t_args->file_mutex);
    FILE *source = fopen(t_args->in_file, "r");
    pthread_mutex_unlock(t_args->file_mutex);

    if (!source) {
        perror("Failed to open input file");
        t_args->out_buffer = NULL;
        t_args->bytes_written = 0;
        return NULL;
    }

    // Seek to start position of this thread's chunk
    fseek(source, t_args->start, SEEK_SET);

    // Allocate buffer for compressed output (estimate 2x input size + overhead)
    char* buffer = malloc(t_args->size * 2 + 100);
    if (!buffer) {
        fclose(source);
        t_args->out_buffer = NULL;
        t_args->bytes_written = 0;
        return NULL;
    }

    t_args->out_buffer = buffer;
    char* buffer_ptr = buffer; // Track current position in buffer
    char prev_char = EOF;      // Previous character in current run
    int count = 0;            // Count of consecutive identical characters
    long bytes_read = 0;      // Bytes read from file
    long total_bytes_written = 0; // Bytes written to buffer

    // Process chunk character by character
    while (bytes_read < t_args->size) {
        char current_char = fgetc(source);
        if (current_char == EOF) break;
        bytes_read++;

        // Handle whitespace and non-binary characters
        if (current_char == ' ' || current_char == '\n') {
            // Flush current run before outputting whitespace
            if (count > 0) {
                if (count >= 16) {
                    // Compress long runs
                    int written = snprintf(buffer_ptr, t_args->size * 2 - total_bytes_written + 100,
                                         "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                    if (written > 0) {
                        buffer_ptr += written;
                        total_bytes_written += written;
                    }
                } else {
                    // Output short runs uncompressed
                    for (int j = 0; j < count; j++) {
                        *buffer_ptr = prev_char;
                        buffer_ptr++;
                        total_bytes_written++;
                    }
                }
            }
            *buffer_ptr = current_char;
            buffer_ptr++;
            total_bytes_written++;
            count = 0;
            prev_char = EOF;
            continue;
        }

        // Process binary digits for compression
        if (prev_char == EOF) {
            // Start new run
            prev_char = current_char;
            count = 1;
        } else if (current_char == prev_char) {
            // Continue current run
            count++;
        } else {
            // End current run and start new one
            if (count > 0) {
                if (count >= 16) {
                    int written = snprintf(buffer_ptr, t_args->size * 2 - total_bytes_written + 100,
                                         "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                    if (written > 0) {
                        buffer_ptr += written;
                        total_bytes_written += written;
                    }
                } else {
                    for (int j = 0; j < count; j++) {
                        *buffer_ptr = prev_char;
                        buffer_ptr++;
                        total_bytes_written++;
                    }
                }
            }
            prev_char = current_char;
            count = 1;
        }
    }

    // Handle any remaining sequence at end of chunk
    if (count > 0) {
        if (count >= 16) {
            int written = snprintf(buffer_ptr, t_args->size * 2 - total_bytes_written + 100,
                                 "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
            if (written > 0) {
                buffer_ptr += written;
                total_bytes_written += written;
            }
        } else {
            for (int j = 0; j < count; j++) {
                *buffer_ptr = prev_char;
                buffer_ptr++;
                total_bytes_written++;
            }
        }
    }

    t_args->bytes_written = total_bytes_written;
    fclose(source);
    return NULL;
}

/**
 * Main function - implements multithreaded compression
 * Creates thread pool to process file chunks in parallel
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 on success, 1 on error
 */
int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num_threads> <source_file> <dest_file>\n", argv[0]);
        return 1;
    }

    int n_threads = atoi(argv[1]);
    if (n_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        return 1;
    }

    const char* source_file = argv[2];
    const char* dest_file = argv[3];

    // Get file size for chunk calculation
    struct stat st;
    if (stat(source_file, &st) != 0) {
        perror("Failed to get file size");
        return 1;
    }

    long file_size = st.st_size;
    long chunk_size = file_size / n_threads;

    // Initialize mutexes for thread synchronization
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;   // Protects file access
    pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER; // Protects output writing

    pthread_t threads[n_threads];    // Thread handles
    thread_args_t args[n_threads];  // Thread arguments

    // Create threads to process chunks in parallel
    for (int i = 0; i < n_threads; i++) {
        args[i].in_file = source_file;
        args[i].start = i * chunk_size;
        args[i].size = (i == n_threads - 1) ? (file_size - args[i].start) : chunk_size;
        args[i].out_buffer = NULL;
        args[i].bytes_written = 0;
        args[i].file_mutex = &file_mutex;
        args[i].output_mutex = &output_mutex;

        int result = pthread_create(&threads[i], NULL, compress_chunk_thread, &args[i]);
        if (result != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return 1;
        }
    }

    // Open destination file for writing
    FILE *final_dest = fopen(dest_file, "w");
    if (!final_dest) {
        perror("Failed to open destination file");
        return 1;
    }

    // Wait for all threads to complete and write their results
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);

        // Write this thread's output to destination file
        if (args[i].out_buffer && args[i].bytes_written > 0) {
            pthread_mutex_lock(&output_mutex);
            fwrite(args[i].out_buffer, 1, args[i].bytes_written, final_dest);
            pthread_mutex_unlock(&output_mutex);
        }

        // Clean up thread-local buffer
        if (args[i].out_buffer) {
            free(args[i].out_buffer);
        }
    }

    fclose(final_dest);

    // Clean up mutexes
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&output_mutex);

    printf("Threaded compression complete with %d threads.\n", n_threads);
    return 0;
}
