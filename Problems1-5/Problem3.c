#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        printf("fork error");
    }
    else if (pid == 0) {
        printf("child process %d \n", getpid()); 
    }
    else {
        wait(NULL);
        printf("else part: child process %d \n", pid);
        printf("else part: parent process %d \n", getpid());
    }

    return 0;
}
