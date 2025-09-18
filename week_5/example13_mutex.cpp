#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


pthread_mutex_t lock;

void *print_a(void *) {
	pthread_mutex_lock(&lock);
	for (int i=0; i<10; i++) {
        printf("A\n");
        usleep(10000);
	}
	pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

void *print_b(void *) {
	pthread_mutex_lock(&lock);
	for (int i=0; i<10; i++) {
        printf("B\n");
        usleep(10000);
	}
	pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

int main(void) {
	pthread_t tid_1, tid_2;

	pthread_mutex_init(&lock, NULL);

	pthread_create(&tid_1, NULL, &print_a, NULL);
	pthread_create(&tid_2, NULL, &print_b, NULL);

	pthread_join(tid_1, NULL);
	pthread_join(tid_2, NULL);

	return 0;
}
