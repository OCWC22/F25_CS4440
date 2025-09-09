#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


int main() {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(pipefd[1]);

        FILE *output = fopen("destination.txt", "w");

        char ch2;
        // read from pipe and convert to lowercase
        while (read(pipefd[0], &ch2, 1) > 0) {
            if (ch2 >= 'A' && ch2 <= 'Z') {
                ch2 = ch2 + 32;
            }
            fputc(ch2, output);
        }

        fclose(output);
        close(pipefd[0]);

    } else {
        close(pipefd[0]); 

        FILE *input = fopen("source.txt", "r");

        char ch1;
        while ((ch1 = fgetc(input)) != EOF) {
            // write to pipe
            write(pipefd[1], &ch1, 1);
        }

        fclose(input);
        close(pipefd[1]);

        wait(NULL);
    }

    return 0;
}
