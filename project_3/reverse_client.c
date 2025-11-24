// reverse_client.c
// Simple TCP client that sends one line to reverse_server and prints the reversed result.
// Treat this as the caller-side of a tiny API: connect -> send payload -> read response.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAX_LINE 1024

int main(int argc, char *argv[]) {
    // Expect the server IP and TCP port. Fail fast if arguments are missing.
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    char buf[MAX_LINE];
    printf("Enter a string to reverse: ");
    if (!fgets(buf, sizeof(buf), stdin)) {
        fprintf(stderr, "No input.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    size_t len = strlen(buf);
    if (send(sockfd, buf, len, 0) != (ssize_t)len) {
        perror("send");
        close(sockfd);
        return EXIT_FAILURE;
    }

    ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        perror("recv");
        close(sockfd);
        return EXIT_FAILURE;
    }
    buf[n] = '\0';
    printf("Reversed from server: %s", buf);

    close(sockfd);
    return EXIT_SUCCESS;
}
