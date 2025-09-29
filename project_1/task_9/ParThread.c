#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

// Struct to pass arguments to our thread function
typedef struct {
    const char* in_file;
    char* out_buffer;
    long start;
    long size;
    long bytes_written;
    pthread_mutex_t* file_mutex;
    pthread_mutex_t* output_mutex;
} thread_args_t;

void* compress_chunk_thread(void *args) {
    thread_args_t *t_args = (thread_args_t*)args;

    // Use mutex to protect file access
    pthread_mutex_lock(t_args->file_mutex);
    FILE *source = fopen(t_args->in_file, "r");
    pthread_mutex_unlock(t_args->file_mutex);

    if (!source) {
        perror("Failed to open input file");
        t_args->out_buffer = NULL;
        t_args->bytes_written = 0;
        return NULL;
    }

    fseek(source, t_args->start, SEEK_SET);

    // Allocate a reasonable buffer for compressed output
    char* buffer = malloc(t_args->size * 2 + 100);
    if (!buffer) {
        fclose(source);
        t_args->out_buffer = NULL;
        t_args->bytes_written = 0;
        return NULL;
    }

    t_args->out_buffer = buffer;
    char* buffer_ptr = buffer; // Keep track of current position

    char prev_char = EOF;
    int count = 0;
    long bytes_read = 0;
    long total_bytes_written = 0;

    while (bytes_read < t_args->size) {
        char current_char = fgetc(source);
        if (current_char == EOF) break;
        bytes_read++;

        // Improved compression logic with proper boundary handling
        if (current_char == ' ' || current_char == '\n') {
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
            *buffer_ptr = current_char;
            buffer_ptr++;
            total_bytes_written++;
            count = 0;
            prev_char = EOF;
            continue;
        }

        if (prev_char == EOF) {
            prev_char = current_char;
            count = 1;
        } else if (current_char == prev_char) {
            count++;
        } else {
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

    // Handle any remaining sequence at the end of chunk
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

int main(int argc, char *argv[]) {
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

    struct stat st;
    if (stat(source_file, &st) != 0) {
        perror("Failed to get file size");
        return 1;
    }

    long file_size = st.st_size;
    long chunk_size = file_size / n_threads;

    // Initialize mutexes for thread synchronization
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_t threads[n_threads];
    thread_args_t args[n_threads];

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

    // Open destination file
    FILE *final_dest = fopen(dest_file, "w");
    if (!final_dest) {
        perror("Failed to open destination file");
        return 1;
    }

    // Wait for all threads to complete and write their results
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);

        if (args[i].out_buffer && args[i].bytes_written > 0) {
            pthread_mutex_lock(&output_mutex);
            fwrite(args[i].out_buffer, 1, args[i].bytes_written, final_dest);
            pthread_mutex_unlock(&output_mutex);
        }

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
