#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>

#define MAX_SIZE 5


char buffer[MAX_SIZE];
int count = 0;

char get_char(void) {
    static char c = 'A';
    if (c > 'Z') c = 'A';
    return c++;
}

void use_char(char c) {
    printf("Consumed: %c\n", c);
    fflush(stdout);
}

void *producer(void *arg) {
    for (;;) {
        char item = get_char();
        printf("Produced: %c\n", item);
        buffer[count] = item;
        count++;
    }
    return NULL;
}

void *consumer(void *arg) {
    for (;;) {
        char item = buffer[count - 1];
        count--;
        use_char(item);
    }
    return NULL;
}

int main(void) {
    pthread_t prod, cons;

    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    return 0;
}
