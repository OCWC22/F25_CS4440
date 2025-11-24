// disk_client_random.c
// Random tester client for the disk server.
//
// Usage:
//   ./disk_client_random <server-ip> <port> <N> <seed>
//
// Steps:
//   1. Send "I" to get cylinders + sectors.
//   2. Generate N random requests; each is randomly R or W.
//   3. For each write, send 128 bytes; for reads, ignore returned data.
//   4. Print a single char per request to show progress: 'R' or 'W'.

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BLOCK_SIZE 128
#define MAX_LINE   128

// REPOMARK:SCOPE: 1 - Helper Functions for Reliable Network I/O
// recv_all: Ensures exactly 'len' bytes are read from the socket.
// TCP streams can fragmant data, so we must loop until the buffer is full.
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

// recv_line: Reads byte-by-byte until a newline is encountered.
// Necessary for parsing variable-length text responses (like Geometry info).
static ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    size_t pos = 0;
    while (pos < maxlen - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 0) break;
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

int main(int argc, char *argv[]) {
    // REPOMARK:SCOPE: 2 - Argument Parsing and Setup
    // Requires Server IP, Port, N (number of requests), and Seed.
    // The Seed allows us to run reproducible stress tests.
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server-ip> <port> <N> <seed>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port     = atoi(argv[2]);
    long long N  = atoll(argv[3]);
    unsigned int seed = (unsigned int)strtoul(argv[4], NULL, 10);

    // Initialize Random Number Generator for deterministic behavior
    srand(seed);

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

    // Step 1: get disk geometry via I
    // REPOMARK:SCOPE: 3 - Fetch Disk Geometry
    // We must know the disk dimensions to generate valid Cylinder/Sector coordinates.
    const char *msg = "I\n";
    if (send(sockfd, msg, strlen(msg), 0) < 0) {
        perror("send(I)");
        close(sockfd);
        return EXIT_FAILURE;
    }

    char line[MAX_LINE];
    if (recv_line(sockfd, line, sizeof(line)) <= 0) {
        perror("recv geometry");
        close(sockfd);
        return EXIT_FAILURE;
    }

    int cylinders = 0, sectors = 0;
    if (sscanf(line, "%d %d", &cylinders, &sectors) != 2 ||
        cylinders <= 0 || sectors <= 0) {
        fprintf(stderr, "Bad geometry response: '%s'\n", line);
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Disk geometry: %d cylinders, %d sectors/cylinder\n",
           cylinders, sectors);

    unsigned char block[BLOCK_SIZE];

    // REPOMARK:SCOPE: 4 - Main Load Testing Loop
    // Execute N random operations to stress the server.
    for (long long i = 0; i < N; i++) {
        // Pick random target location within valid range
        int c = rand() % cylinders;
        int s = rand() % sectors;

        // Randomly decide: 50% chance Write, 50% chance Read
        int is_write = rand() & 1;

        if (is_write) {
            // --- WRITE PATH ---
            // 1. Fill block with random data
            for (int j = 0; j < BLOCK_SIZE; j++) {
                block[j] = (unsigned char)(rand() % 256);
            }
            // 2. Send Write Header: W <cyl> <sec> <size>
            char header[64];
            int hlen = snprintf(header, sizeof(header),
                                "W %d %d %d ", c, s, BLOCK_SIZE);
            if (send(sockfd, header, hlen, 0) < 0) {
                perror("send header");
                break;
            }
            // 3. Send Data Payload
            if (send(sockfd, block, BLOCK_SIZE, 0) < 0) {
                perror("send block");
                break;
            }

            // 4. Wait for Acknowledgment
            char status;
            if (recv_all(sockfd, &status, 1) != 1) {
                perror("recv status");
                break;
            }
            putchar('W');

        } else {
            // --- READ PATH ---
            // 1. Send Read Request: R <cyl> <sec>
            char header[64];
            int hlen = snprintf(header, sizeof(header), "R %d %d\n", c, s);
            if (send(sockfd, header, hlen, 0) < 0) {
                perror("send header");
                break;
            }

            // 2. Receive Status Byte
            char status;
            if (recv_all(sockfd, &status, 1) != 1) {
                perror("recv status");
                break;
            }
            // 3. If valid, drain the incoming data block
            if (status == '1') {
                if (recv_all(sockfd, block, BLOCK_SIZE) != BLOCK_SIZE) {
                    perror("recv block");
                    break;
                }
            }
            putchar('R');
        }

        fflush(stdout);
    }

    putchar('\n');
    close(sockfd);
    return EXIT_SUCCESS;
}
