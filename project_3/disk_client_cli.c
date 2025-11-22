// disk_client_cli.c
// Command-line driven disk client.
// Lets user type commands like:
//   I
//   R 0 5
//   W 0 5 hello world
//
// For W, data is whatever remains on the line after the two numbers.

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BLOCK_SIZE 128
#define MAX_LINE   1024

static ssize_t recv_all(int fd, void *buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        ssize_t n = recv(fd, (char *)buf + done, len - done, 0);
        if (n == 0) return done;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        done += n;
    }
    return done;
}

int main(int argc, char *argv[]) {
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

    char line[MAX_LINE];

    printf("Connected to disk server. Commands: I | R c s | W c s data | q\n");
    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        // strip newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        if (line[0] == '\0') continue;

        char cmd[4];
        int c = 0, s = 0;

        if (sscanf(line, "%3s", cmd) != 1) continue;

        if (strcmp(cmd, "I") == 0 || strcmp(cmd, "i") == 0) {
            const char *msg = "I\n";
            if (send(sockfd, msg, strlen(msg), 0) < 0) {
                perror("send");
                break;
            }

            char reply[64];
            ssize_t n = recv(sockfd, reply, sizeof(reply) - 1, 0);
            if (n <= 0) {
                perror("recv");
                break;
            }
            reply[n] = '\0';
            printf("Disk geometry: %s", reply);

        } else if (cmd[0] == 'R' || cmd[0] == 'r') {
            if (sscanf(line, "%*s %d %d", &c, &s) != 2) {
                printf("Usage: R <cylinder> <sector>\n");
                continue;
            }

            char header[64];
            int hlen = snprintf(header, sizeof(header), "R %d %d\n", c, s);
            if (send(sockfd, header, hlen, 0) < 0) {
                perror("send");
                break;
            }

            char status;
            if (recv_all(sockfd, &status, 1) != 1) {
                perror("recv status");
                break;
            }
            if (status == '0') {
                printf("Invalid read (c or s out of range)\n");
                continue;
            }

            unsigned char buf[BLOCK_SIZE];
            if (recv_all(sockfd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
                perror("recv data");
                break;
            }

            printf("Read OK. First %d bytes (printable chars / dots):\n", BLOCK_SIZE);
            for (int i = 0; i < BLOCK_SIZE; i++) {
                unsigned char ch = buf[i];
                if (ch >= 32 && ch <= 126) putchar(ch);
                else putchar('.');
            }
            putchar('\n');

        } else if (cmd[0] == 'W' || cmd[0] == 'w') {
            // W c s data...
            char *p = line;

            // skip command
            while (*p && !isspace((unsigned char)*p)) p++;
            while (*p && isspace((unsigned char)*p)) p++;

            if (sscanf(p, "%d %d", &c, &s) != 2) {
                printf("Usage: W <cylinder> <sector> <data...>\n");
                continue;
            }

            // Skip cylinder
            while (*p && !isspace((unsigned char)*p)) p++;
            while (*p && isspace((unsigned char)*p)) p++;
            // Skip sector
            while (*p && !isspace((unsigned char)*p)) p++;
            while (*p && isspace((unsigned char)*p)) p++;

            char *data = p;
            size_t data_len = strlen(data);
            if (data_len > BLOCK_SIZE) {
                fprintf(stderr, "Data too long (%zu > %d)\n",
                        data_len, BLOCK_SIZE);
                continue;
            }

            char header[64];
            int hlen = snprintf(header, sizeof(header),
                                "W %d %d %zu ", c, s, data_len);
            if (send(sockfd, header, hlen, 0) < 0) {
                perror("send header");
                break;
            }

            if (data_len > 0) {
                if (send(sockfd, data, data_len, 0) < 0) {
                    perror("send data");
                    break;
                }
            }

            char status;
            if (recv_all(sockfd, &status, 1) != 1) {
                perror("recv status");
                break;
            }
            if (status == '1') printf("Write OK.\n");
            else printf("Write failed (invalid c/s/l).\n");

        } else if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
            break;
        } else {
            printf("Unknown command. Use I, R c s, W c s data, or q.\n");
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
