#include <stdio.h>
#include <pthread.h>


void *calculate(void *args) {
	int *p = (int *)args; 
    int start = p[0];
    int end   = p[1];

    // add a return value
    int *sum = new int;
    *sum = 0;
    for (int i = start; i <= end; i++) {
        *sum += i;
    }

    // exit and return
    pthread_exit(sum);
}

int main(void) {
	pthread_t tid1, tid2;

	int range1[2] = {1, 50};
	int range2[2] = {51, 100};

	pthread_create(&tid1, NULL, calculate, range1);
	pthread_create(&tid2, NULL, calculate, range2);

	int *sum1, *sum2;
	pthread_join(tid1, (void **)&sum1);
    pthread_join(tid2, (void **)&sum2);

    int total = *sum1 + *sum2;
    printf("Total sum = %d\n", total);

	return 0;
}