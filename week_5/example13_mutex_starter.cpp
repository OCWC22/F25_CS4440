#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


void *print_a(void *) {
	for (int i=0; i<10; i++) {
        printf("A\n");
        usleep(10000);
	}
}

void *print_b(void *) {
	for (int i=0; i<10; i++) {
        printf("B\n");
        usleep(10000);
	}
}

int main(void) {
	pthread_t tid_1, tid_2;

	pthread_create(&tid_1, NULL, &print_a, NULL);
	pthread_create(&tid_2, NULL, &print_b, NULL);

	pthread_join(tid_1, NULL);
	pthread_join(tid_2, NULL);

	return 0;
}
