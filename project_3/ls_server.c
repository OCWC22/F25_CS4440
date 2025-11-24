// ls_server.c
// Server: receives a line of arguments, runs `ls` with those args,
// and streams the output back over the socket using fork + exec.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define BACKLOG 10

static ssize_t read_line(int fd, char *buf, size_t maxlen) {
    size_t pos = 0;
    while (pos < maxlen - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 0) break;      // EOF
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (c == '\n') break;
        buf[pos++] = c;
    }
    buf[pos] = '\0';
    return (ssize_t)pos;
}

static void handle_client(int clientfd) {
    char line[MAX_LINE];
    ssize_t n = read_line(clientfd, line, sizeof(line));
    if (n <= 0) {
        close(clientfd);
        return;
    }

    // Build argv for execvp: argv[0] = "ls", rest from tokens.
    char *argv_ls[MAX_ARGS];
    int argc_ls = 0;

    argv_ls[argc_ls++] = "ls";

    char *saveptr = NULL;
    char *tok = strtok_r(line, " \t", &saveptr);
    while (tok && argc_ls < MAX_ARGS - 1) {
        argv_ls[argc_ls++] = tok;
        tok = strtok_r(NULL, " \t", &saveptr);
    }
    argv_ls[argc_ls] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(clientfd);
        return;
    }

    if (pid == 0) {
        // Child: send ls output directly to socket.
        dup2(clientfd, STDOUT_FILENO);
        dup2(clientfd, STDERR_FILENO);
        close(clientfd);
        execvp("ls", argv_ls);
        perror("execvp");
        _exit(1);
    } else {
        // Parent: just wait and then close the socket.
        int status;
        waitpid(pid, &status, 0);
        close(clientfd);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listenfd);
        return EXIT_FAILURE;
    }

    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        close(listenfd);
        return EXIT_FAILURE;
    }

    printf("ls server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }
        handle_client(clientfd);
    }

    close(listenfd);
    return EXIT_SUCCESS;
}
