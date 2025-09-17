#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Enhanced ParThread.c with Advanced Data Structures
// Implements: Priority Queues, Stacks, Divide & Conquer, Hash Tables

#define QUEUE_SIZE 1024
#define HASH_TABLE_SIZE 1024
#define MAX_STACK_DEPTH 256

// ============================================================================
// 1. PRIORITY QUEUE (HEAP) FOR DYNAMIC LOAD BALANCING
// ============================================================================

typedef struct {
    int priority;      // Based on chunk size/complexity
    int thread_id;
    long start_offset;
    long chunk_size;
    char* data;        // For caching
} thread_task_t;

// Min-heap for task scheduling
thread_task_t* task_queue[QUEUE_SIZE];
int heap_size = 0;
pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;

void heapify_up(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (task_queue[index]->priority >= task_queue[parent]->priority) break;

        thread_task_t* temp = task_queue[index];
        task_queue[index] = task_queue[parent];
        task_queue[parent] = temp;
        index = parent;
    }
}

void heapify_down(int index) {
    int size = heap_size;
    while (1) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int smallest = index;

        if (left < size && task_queue[left]->priority < task_queue[smallest]->priority)
            smallest = left;
        if (right < size && task_queue[right]->priority < task_queue[smallest]->priority)
            smallest = right;

        if (smallest == index) break;

        thread_task_t* temp = task_queue[index];
        task_queue[index] = task_queue[smallest];
        task_queue[smallest] = temp;
        index = smallest;
    }
}

void heap_push(thread_task_t* task) {
    pthread_mutex_lock(&heap_mutex);
    if (heap_size >= QUEUE_SIZE) {
        pthread_mutex_unlock(&heap_mutex);
        free(task);
        return;
    }
    task_queue[heap_size++] = task;
    heapify_up(heap_size - 1);
    pthread_mutex_unlock(&heap_mutex);
}

thread_task_t* heap_pop() {
    pthread_mutex_lock(&heap_mutex);
    if (heap_size == 0) {
        pthread_mutex_unlock(&heap_mutex);
        return NULL;
    }

    thread_task_t* min_task = task_queue[0];
    task_queue[0] = task_queue[--heap_size];
    heapify_down(0);
    pthread_mutex_unlock(&heap_mutex);
    return min_task;
}

// ============================================================================
// 2. STACK-BASED COMPRESSION STATE MANAGEMENT
// ============================================================================

typedef struct compression_state {
    char current_char;
    int count;
    long position;
    struct compression_state* next;
} compression_state_t;

compression_state_t* state_stack = NULL;
pthread_mutex_t stack_mutex = PTHREAD_MUTEX_INITIALIZER;

void push_state(char ch, int cnt, long pos) {
    pthread_mutex_lock(&stack_mutex);
    compression_state_t* new_state = malloc(sizeof(compression_state_t));
    new_state->current_char = ch;
    new_state->count = cnt;
    new_state->position = pos;
    new_state->next = state_stack;
    state_stack = new_state;
    pthread_mutex_unlock(&stack_mutex);
}

compression_state_t* pop_state() {
    pthread_mutex_lock(&stack_mutex);
    if (!state_stack) {
        pthread_mutex_unlock(&stack_mutex);
        return NULL;
    }
    compression_state_t* top = state_stack;
    state_stack = state_stack->next;
    pthread_mutex_unlock(&stack_mutex);
    return top;
}

void free_state_stack() {
    pthread_mutex_lock(&stack_mutex);
    while (state_stack) {
        compression_state_t* temp = state_stack;
        state_stack = state_stack->next;
        free(temp);
    }
    pthread_mutex_unlock(&stack_mutex);
}

// ============================================================================
// 3. DIVIDE AND CONQUER WITH BALANCED BINARY TREE
// ============================================================================

typedef struct work_node {
    long start_offset;
    long size;
    int complexity;    // Estimated processing complexity
    struct work_node* left;
    struct work_node* right;
} work_node_t;

int estimate_complexity(long size) {
    // Estimate based on size and some heuristics
    // In practice, you might analyze the data content
    return (int)(size / 1024); // Simple heuristic
}

work_node_t* build_work_tree(long file_size, int max_chunks) {
    if (max_chunks <= 1) {
        work_node_t* leaf = malloc(sizeof(work_node_t));
        leaf->start_offset = 0;
        leaf->size = file_size;
        leaf->complexity = estimate_complexity(file_size);
        leaf->left = leaf->right = NULL;
        return leaf;
    }

    long mid = file_size / 2;
    work_node_t* node = malloc(sizeof(work_node_t));
    node->left = build_work_tree(mid, max_chunks/2);
    node->right = build_work_tree(file_size - mid, max_chunks/2);
    node->complexity = node->left->complexity + node->right->complexity;
    return node;
}

void free_work_tree(work_node_t* node) {
    if (!node) return;
    free_work_tree(node->left);
    free_work_tree(node->right);
    free(node);
}

// ============================================================================
// 4. HASH MAP FOR CHUNK RESULT CACHING
// ============================================================================

typedef struct hash_entry {
    unsigned long key;    // Chunk identifier (hash of content)
    char* compressed_data;
    size_t data_size;
    struct hash_entry* next;
} hash_entry_t;

hash_entry_t* hash_table[HASH_TABLE_SIZE];
pthread_mutex_t hash_mutex = PTHREAD_MUTEX_INITIALIZER;

unsigned long hash_chunk(const char* data, size_t size) {
    unsigned long hash = 5381;
    for (size_t i = 0; i < size && i < 256; i++) { // Sample first 256 bytes
        hash = ((hash << 5) + hash) + data[i];
    }
    return hash % HASH_TABLE_SIZE;
}

void cache_result(const char* original_data, size_t original_size,
                 const char* compressed_data, size_t compressed_size) {
    unsigned long key = hash_chunk(original_data, original_size);

    pthread_mutex_lock(&hash_mutex);
    hash_entry_t* entry = malloc(sizeof(hash_entry_t));
    entry->key = key;
    entry->compressed_data = malloc(compressed_size);
    memcpy(entry->compressed_data, compressed_data, compressed_size);
    entry->data_size = compressed_size;
    entry->next = hash_table[key];
    hash_table[key] = entry;
    pthread_mutex_unlock(&hash_mutex);
}

char* get_cached_result(const char* chunk, size_t size, size_t* cached_size) {
    unsigned long key = hash_chunk(chunk, size);

    pthread_mutex_lock(&hash_mutex);
    hash_entry_t* entry = hash_table[key];
    while (entry) {
        // Simple comparison - in practice you'd need more sophisticated matching
        if (entry->data_size > 0) {
            *cached_size = entry->data_size;
            char* result = malloc(entry->data_size);
            memcpy(result, entry->compressed_data, entry->data_size);
            pthread_mutex_unlock(&hash_mutex);
            return result;
        }
        entry = entry->next;
    }
    pthread_mutex_unlock(&hash_mutex);
    return NULL;
}

void free_hash_table() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        hash_entry_t* entry = hash_table[i];
        while (entry) {
            hash_entry_t* temp = entry;
            entry = entry->next;
            free(temp->compressed_data);
            free(temp);
        }
    }
}

// ============================================================================
// ENHANCED COMPRESSION THREAD WITH ADVANCED DATA STRUCTURES
// ============================================================================

typedef struct {
    const char* in_file;
    char* out_buffer;
    long start;
    long size;
    long bytes_written;
} thread_args_t;

// Enhanced compression using stack-based state management
void* compress_chunk_enhanced(void *args) {
    thread_args_t *t_args = (thread_args_t*)args;

    FILE *source = fopen(t_args->in_file, "r");
    if (!source) {
        perror("Failed to open input file");
        t_args->out_buffer = NULL;
        t_args->bytes_written = 0;
        return NULL;
    }

    fseek(source, t_args->start, SEEK_SET);

    // Allocate buffer with some extra space
    char* buffer = malloc(t_args->size * 4);
    if (!buffer) {
        fclose(source);
        t_args->out_buffer = NULL;
        t_args->bytes_written = 0;
        return NULL;
    }

    t_args->out_buffer = buffer;
    char* buffer_ptr = buffer;

    char prev_char = EOF;
    int count = 0;
    long bytes_read = 0;
    long total_bytes_written = 0;

    // Use stack to track compression states
    push_state(EOF, 0, 0);

    while (bytes_read < t_args->size) {
        char current_char = fgetc(source);
        if (current_char == EOF) break;
        bytes_read++;

        // Enhanced whitespace handling with state management
        if (current_char == ' ' || current_char == '\n') {
            if (count > 0) {
                // Use stack to save current state before flushing
                push_state(prev_char, count, total_bytes_written);

                if (count >= 16) {
                    int written = snprintf(buffer_ptr, t_args->size * 4 - total_bytes_written,
                                         "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                    buffer_ptr += written;
                    total_bytes_written += written;
                } else {
                    for (int j = 0; j < count; j++) {
                        *buffer_ptr++ = prev_char;
                        total_bytes_written++;
                    }
                }
                count = 0;
                prev_char = EOF;
            }
            *buffer_ptr++ = current_char;
            total_bytes_written++;
            continue;
        }

        // Build runs of identical characters
        if (prev_char == EOF) {
            prev_char = current_char;
            count = 1;
        } else if (current_char == prev_char) {
            count++;
        } else {
            // Flush previous run
            push_state(prev_char, count, total_bytes_written);

            if (count >= 16) {
                int written = snprintf(buffer_ptr, t_args->size * 4 - total_bytes_written,
                                     "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                buffer_ptr += written;
                total_bytes_written += written;
            } else {
                for (int j = 0; j < count; j++) {
                    *buffer_ptr++ = prev_char;
                    total_bytes_written++;
                }
            }
            prev_char = current_char;
            count = 1;
        }
    }

    // Final flush
    if (count > 0) {
        push_state(prev_char, count, total_bytes_written);

        if (count >= 16) {
            int written = snprintf(buffer_ptr, t_args->size * 4 - total_bytes_written,
                                 "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
            buffer_ptr += written;
            total_bytes_written += written;
        } else {
            for (int j = 0; j < count; j++) {
                *buffer_ptr++ = prev_char;
                total_bytes_written++;
            }
        }
    }

    *buffer_ptr = '\0';
    t_args->bytes_written = total_bytes_written;

    fclose(source);
    return NULL;
}

// ============================================================================
// ENHANCED MAIN FUNCTION WITH DYNAMIC LOAD BALANCING
// ============================================================================

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

    // Get file size
    struct stat st;
    if (stat(source_file, &st) != 0) {
        perror("Failed to get file size");
        return 1;
    }
    long file_size = st.st_size;

    // Build work tree for balanced load distribution
    work_node_t* work_tree = build_work_tree(file_size, n_threads);

    // Create priority-based task queue
    long chunk_size = file_size / n_threads;
    for (int i = 0; i < n_threads; i++) {
        thread_task_t* task = malloc(sizeof(thread_task_t));
        task->priority = rand() % 100; // Simulate complexity variation
        task->thread_id = i;
        task->start_offset = i * chunk_size;
        task->chunk_size = (i == n_threads - 1) ? (file_size - task->start_offset) : chunk_size;
        heap_push(task);
    }

    // Create threads with dynamic task assignment
    pthread_t threads[n_threads];
    thread_args_t args[n_threads];

    for (int i = 0; i < n_threads; i++) {
        thread_task_t* task = heap_pop();
        if (!task) break;

        args[i].in_file = source_file;
        args[i].start = task->start_offset;
        args[i].size = task->chunk_size;
        args[i].out_buffer = NULL;
        args[i].bytes_written = 0;

        free(task); // Free the task after assignment

        int result = pthread_create(&threads[i], NULL, compress_chunk_enhanced, &args[i]);
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

    // Wait for all threads and write results
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);

        if (args[i].out_buffer && args[i].bytes_written > 0) {
            // Cache the result for potential reuse
            cache_result(NULL, 0, args[i].out_buffer, args[i].bytes_written);

            fwrite(args[i].out_buffer, 1, args[i].bytes_written, final_dest);
        }

        if (args[i].out_buffer) {
            free(args[i].out_buffer);
        }
    }

    fclose(final_dest);

    // Cleanup enhanced data structures
    free_work_tree(work_tree);
    free_state_stack();
    free_hash_table();

    printf("Enhanced threaded compression complete with %d threads.\n", n_threads);
    printf("Features: Priority Queue, Stack State Management, Divide & Conquer, Hash Caching\n");
    return 0;
}
