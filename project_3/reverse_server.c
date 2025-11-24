// reverse_server.c
// Multi-threaded TCP server: accepts a string and sends back the reversed string.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>

#define MAX_LINE 1024
#define BACKLOG 10

static void reverse_string(char *s) {
    size_t i = 0, j = strlen(s);
    if (j == 0) return;
    j--; // last index
    while (i < j) {
        char tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
        i++;
        j--;
    }
}

static void *handle_client(void *arg) {
    int clientfd = *(int *)arg;
    free(arg);

    char buf[MAX_LINE];
    ssize_t n = recv(clientfd, buf, MAX_LINE - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        // strip trailing newline if present
        if (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
            buf[n - 1] = '\0';
        }

        reverse_string(buf);
        strcat(buf, "\n");
        send(clientfd, buf, strlen(buf), 0);
    }

    close(clientfd);
    return NULL;
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
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listenfd);
        return EXIT_FAILURE;
    }

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

    printf("Reverse server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int *clientfd = malloc(sizeof(int));
        if (!clientfd) {
            perror("malloc");
            continue;
        }

        *clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (*clientfd < 0) {
            perror("accept");
            free(clientfd);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, clientfd) != 0) {
            perror("pthread_create");
            close(*clientfd);
            free(clientfd);
            continue;
        }
        pthread_detach(tid);
    }

    close(listenfd);
    return EXIT_SUCCESS;
}
