#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include <semaphore.h>
#include <fcntl.h> 
#include <sys/stat.h>

sem_t *go = sem_open("/mysem", O_CREAT, 0644, 0);

void *print_a(void *) {
	sem_wait(go);
	for (int i=0; i<10; i++) {
        printf("A\n");
        usleep(10000);
	}
    pthread_exit(NULL);
}

void *print_b(void *) {
	for (int i=0; i<10; i++) {
        printf("B\n");
        usleep(10000);
	}
	sem_post(go);
    pthread_exit(NULL);
}

int main(void) {
	pthread_t tid_1, tid_2;

	pthread_create(&tid_1, NULL, &print_a, NULL);
	pthread_create(&tid_2, NULL, &print_b, NULL);

	pthread_join(tid_1, NULL);
	pthread_join(tid_2, NULL);

	sem_close(go);

	return 0;
}
