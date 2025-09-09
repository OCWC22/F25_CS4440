#include <stdio.h>
#include <pthread.h>

void *printMsg(void* args) {
	char *msg = (char *)args;
	printf("%s\n", msg);
	pthread_exit(NULL);
}

int main(int argc, char** argv) {
	pthread_t tid;

	printf("creating a new thread\n");
	pthread_create(&tid, NULL, printMsg, argv[1]);
	printf("created thread %d\n", tid);
	pthread_join(tid, NULL);
	printf("done\n");

	return 0;
}
