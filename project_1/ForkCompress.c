// ForkCompress.c - Forks and execs MyCompress, then waits for completion.
// Usage: ForkCompress <source> <destination>

// fork) Write a C program that creates a new process to compress a file using the 
// MyCompress. This program should spawn a new process using fork system call. Then 
// use execl to execute MyCompress program. The source and destination file names 
// presented as command-line arguments should be passed to execl  as system call 
// arguments. The main process waits for completion of compress operation using wait 
// system call. (ForkCompress.c à ForkCompress) 

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return 2;
    }

    const char *src = argv[1];
    const char *dst = argv[2];

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child: exec MyCompress
        execl("./MyCompress", "MyCompress", src, dst, (char *)NULL);
        // If we reach here, exec failed
        perror("execl MyCompress");
        _exit(127);
    }

    // Parent: wait for child
    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    if (w < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code == 0) {
            fprintf(stderr, "MyCompress completed successfully.\n");
        } else {
            fprintf(stderr, "MyCompress exited with code %d.\n", code);
        }
        return code;
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        fprintf(stderr, "MyCompress terminated by signal %d.\n", sig);
        return 128 + sig;
    } else {
        fprintf(stderr, "MyCompress ended unexpectedly.\n");
        return 1;
    }
}