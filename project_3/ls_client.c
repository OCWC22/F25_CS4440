// ls_client.c
// Sends ls arguments to ls_server and prints back directory listing.

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
    // REPOMARK:SCOPE: 1 - Validate CLI arguments
    // Expects at least IP and Port. Any arguments after that are passed to ls.
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port> [ls-args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    // REPOMARK:SCOPE: 2 - Create TCP Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);
    // REPOMARK:SCOPE: 3 - Convert IP string to binary and connect
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

    // REPOMARK:SCOPE: 4 - Build command string from arguments
    // Concatenates optional args (e.g. "-l /tmp") into one line separated by spaces
    char line[MAX_LINE] = {0};
    for (int i = 3; i < argc; i++) {
        if (strlen(line) + strlen(argv[i]) + 2 >= sizeof(line)) {
            fprintf(stderr, "Too many/long arguments.\n");
            close(sockfd);
            return EXIT_FAILURE;
        }
        strcat(line, argv[i]);
        if (i != argc - 1) strcat(line, " ");
    }
    strcat(line, "\n");

    // REPOMARK:SCOPE: 5 - Send command to server
    size_t len = strlen(line);
    if (send(sockfd, line, len, 0) != (ssize_t)len) {
        perror("send");
        close(sockfd);
        return EXIT_FAILURE;
    }

    // REPOMARK:SCOPE: 6 - Read server response and print to stdout
    // Loop continues until server closes connection (recv returns 0)
    char buf[MAX_LINE];
    ssize_t n;
    while ((n = recv(sockfd, buf, sizeof(buf), 0)) > 0) {
        if (write(STDOUT_FILENO, buf, n) < 0) {
            perror("write");
            break;
        }
    }

    if (n < 0) perror("recv");

    close(sockfd);
    return EXIT_SUCCESS;
}
