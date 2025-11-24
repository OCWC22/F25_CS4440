// disk_server.c
// Simulated disk server using the protocol from the project spec.
//
// Usage:
//   ./disk_server <port> <cylinders> <sectors_per_cylinder> <track_time_us> <backing_file>
//
// Uses a fixed block size of 128 bytes and mmap()'d backing file.

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BLOCK_SIZE 128
#define BACKLOG    10

static int cylinders;
static int sectors_per_cyl;
static int track_time_us;
static unsigned char *disk_data;   // mmap'd disk
static long long total_blocks;

static ssize_t recv_all(int fd, void *buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        ssize_t n = recv(fd, (char *)buf + done, len - done, 0);
        if (n == 0) return done;        // EOF
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        done += n;
    }
    return done;
}

// Read next token (command or integer) separated by whitespace.
// Skips leading whitespace. Returns:
//   >0: length of token
//   0 : EOF
//  -1 : error
static ssize_t read_token(int fd, char *buf, size_t maxlen) {
    int started = 0;
    size_t pos = 0;
    while (1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 0) { // EOF
            if (started) break;
            return 0;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }

        if (!started) {
            if (isspace((unsigned char)c)) {
                continue; // skip leading whitespace
            }
            started = 1;
        }

        if (isspace((unsigned char)c)) {
            break; // end of token, delimiter eaten
        }

        if (pos < maxlen - 1) {
            buf[pos++] = c;
        } else {
            // token too long; truncate but still eat chars until whitespace
        }
    }
    buf[pos] = '\0';
    return (ssize_t)pos;
}

static void simulate_seek(int *current_cyl, int target_cyl) {
    int diff = target_cyl - *current_cyl;
    if (diff < 0) diff = -diff;
    if (diff > 0 && track_time_us > 0) {
        useconds_t delay = (useconds_t)((long long)diff * track_time_us);
        usleep(delay);
    }
    *current_cyl = target_cyl;
}

static void handle_connection(int clientfd) {
    char tok[64];
    int current_cyl = 0;

    while (1) {
        ssize_t tlen = read_token(clientfd, tok, sizeof(tok));
        if (tlen == 0) break;      // EOF
        if (tlen < 0) {
            perror("read_token");
            break;
        }

        if (strcmp(tok, "I") == 0) {
            char reply[64];
            int n = snprintf(reply, sizeof(reply), "%d %d\n",
                             cylinders, sectors_per_cyl);
            send(clientfd, reply, n, 0);

        } else if (strcmp(tok, "R") == 0) {
            // R c s
            if (read_token(clientfd, tok, sizeof(tok)) <= 0) break;
            int c = atoi(tok);
            if (read_token(clientfd, tok, sizeof(tok)) <= 0) break;
            int s = atoi(tok);

            if (c < 0 || c >= cylinders || s < 0 || s >= sectors_per_cyl) {
                char status = '0';
                send(clientfd, &status, 1, 0);
                continue;
            }

            simulate_seek(&current_cyl, c);

            long long block_index = (long long)c * sectors_per_cyl + s;
            unsigned char *block = disk_data + block_index * BLOCK_SIZE;

            char status = '1';
            if (send(clientfd, &status, 1, 0) != 1) break;
            if (send(clientfd, block, BLOCK_SIZE, 0) != BLOCK_SIZE) break;

        } else if (strcmp(tok, "W") == 0) {
            // W c s l data
            if (read_token(clientfd, tok, sizeof(tok)) <= 0) break;
            int c = atoi(tok);
            if (read_token(clientfd, tok, sizeof(tok)) <= 0) break;
            int s = atoi(tok);
            if (read_token(clientfd, tok, sizeof(tok)) <= 0) break;
            int l = atoi(tok);

            if (c < 0 || c >= cylinders || s < 0 || s >= sectors_per_cyl ||
                l < 0 || l > BLOCK_SIZE) {
                char status = '0';
                send(clientfd, &status, 1, 0);
                // You *could* also consume l bytes of data here if the client sent them,
                // but spec implies write requests will be valid.
                continue;
            }

            unsigned char buf[BLOCK_SIZE];
            if (l > 0) {
                if (recv_all(clientfd, buf, (size_t)l) != l) {
                    perror("recv_all");
                    break;
                }
            }

            simulate_seek(&current_cyl, c);

            long long block_index = (long long)c * sectors_per_cyl + s;
            unsigned char *block = disk_data + block_index * BLOCK_SIZE;

            memcpy(block, buf, (size_t)l);
            if (l < BLOCK_SIZE) {
                memset(block + l, 0, BLOCK_SIZE - l);  // zero-fill the rest
            }

            char status = '1';
            if (send(clientfd, &status, 1, 0) != 1) break;

        } else {
            // Unknown command; stop.
            fprintf(stderr, "Unknown command token: '%s'\n", tok);
            break;
        }
    }

    close(clientfd);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr,
                "Usage: %s <port> <cylinders> <sectors_per_cylinder> <track_time_us> <backing_file>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    cylinders       = atoi(argv[2]);
    sectors_per_cyl = atoi(argv[3]);
    track_time_us   = atoi(argv[4]);
    const char *filename = argv[5];

    if (cylinders <= 0 || sectors_per_cyl <= 0) {
        fprintf(stderr, "Invalid cylinders or sectors.\n");
        return EXIT_FAILURE;
    }

    total_blocks = (long long)cylinders * sectors_per_cyl;
    long long disk_bytes = total_blocks * BLOCK_SIZE;

    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("open backing file");
        return EXIT_FAILURE;
    }

    if (ftruncate(fd, (off_t)disk_bytes) < 0) {
        perror("ftruncate");
        close(fd);
        return EXIT_FAILURE;
    }

    disk_data = mmap(NULL, (size_t)disk_bytes, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (disk_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd); // mapping stays

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

    printf("Disk server listening on port %d (cyl=%d, sec/cyl=%d)\n",
           port, cylinders, sectors_per_cyl);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }
        handle_connection(clientfd);
    }

    // unreachable normally
    munmap(disk_data, (size_t)(total_blocks * BLOCK_SIZE));
    close(listenfd);
    return EXIT_SUCCESS;
}
