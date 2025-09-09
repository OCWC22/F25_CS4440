#include <stdio.h>
#include <pthread.h>


void *calculate(void *) {
	int sum = 0;
    for(int i = 0; i <= 100; i++)
        sum += i;
    printf("Hi, this is pthread! Sum is %d.\n", sum);
    pthread_exit(NULL);
}

int main(void) {
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, calculate, NULL);
	pthread_join(thread_id, NULL);
	printf("This is main.\n");

	return 0;
}