#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Global mutex for thread synchronization
pthread_mutex_t mutex;

/**
 * Thread function for printing A's (without synchronization)
 * This will interleave with Thread B to demonstrate the problem
 */
void* print_a_unsync(void* arg) {
    for (int i = 0; i < 20; i++) {
        printf("A");
        fflush(stdout);  // Force immediate output
        usleep(10000);   // Small delay to increase chance of interleaving
    }
    printf("\n");
    return NULL;
}

/**
 * Thread function for printing B's (without synchronization)
 * This will interleave with Thread A to demonstrate the problem
 */
void* print_b_unsync(void* arg) {
    for (int i = 0; i < 20; i++) {
        printf("B");
        fflush(stdout);  // Force immediate output
        usleep(10000);   // Small delay to increase chance of interleaving
    }
    printf("\n");
    return NULL;
}

/**
 * Thread function for printing A's (with synchronization)
 * Simple function - mutex control happens in main()
 */
void* print_a_sync(void* arg) {
    for (int i = 0; i < 20; i++) {
        printf("A");
        fflush(stdout);
        usleep(10000);
    }
    printf("\n");
    return NULL;
}

/**
 * Thread function for printing B's (with synchronization)
 * Simple function - mutex control happens in main()
 */
void* print_b_sync(void* arg) {
    for (int i = 0; i < 20; i++) {
        printf("B");
        fflush(stdout);
        usleep(10000);
    }
    printf("\n");
    return NULL;
}

int main() {
    pthread_t thread_a, thread_b;

    // PART 1: Demonstrate interleaving without synchronization
    // Shows what happens when threads run concurrently without coordination
    printf("=== Without synchronization (threads interleave) ===\n");
    pthread_create(&thread_a, NULL, print_a_unsync, NULL);
    pthread_create(&thread_b, NULL, print_b_unsync, NULL);
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
    printf("\n");

    // PART 2: Demonstrate sequential execution with mutex
    // Shows how mutex forces threads to execute one after another
    printf("=== With mutex synchronization (sequential execution) ===\n");

    // Initialize mutex before use
    pthread_mutex_init(&mutex, NULL);

    // Force sequential execution: Thread A completes, then Thread B starts
    pthread_mutex_lock(&mutex);           // Lock mutex
    pthread_create(&thread_a, NULL, print_a_sync, NULL);
    pthread_join(thread_a, NULL);          // Wait for Thread A to finish
    pthread_mutex_unlock(&mutex);         // Release mutex

    pthread_mutex_lock(&mutex);           // Lock mutex again
    pthread_create(&thread_b, NULL, print_b_sync, NULL);
    pthread_join(thread_b, NULL);          // Wait for Thread B to finish
    pthread_mutex_unlock(&mutex);         // Release mutex

    // Clean up resources
    pthread_mutex_destroy(&mutex);

    printf("=== Complete: Thread A finished all 20 A's before Thread B started ===\n");

    return 0;
}