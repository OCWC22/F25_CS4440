// fs_client.c
// Simple command-line client for the Project 3 filesystem server.
// It sends textual commands (F, C, D, L, R, W, mkdir, cd, pwd, rmdir)
// and prints the server's responses. For R commands, it parses the
// header to determine the file length and then reads exactly that
// many bytes.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096

static ssize_t recv_all(int fd, void *buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        ssize_t n = recv(fd, (char *)buf + done, len - done, 0);
        if (n == 0) return (ssize_t)done; // EOF
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        done += (size_t)n;
    }
    return (ssize_t)done;
}

static void handle_read_command(int sockfd) {
    char header[64];
    size_t pos = 0;
    int spaces = 0;

    // Read until we've seen two spaces: "code len "
    while (pos < sizeof(header) - 1 && spaces < 2) {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);
        if (n <= 0) {
            if (n < 0) perror("recv");
            fprintf(stderr, "Connection closed while reading header.\n");
            return;
        }
        header[pos++] = c;
        if (c == ' ') spaces++;
    }
    header[pos] = '\0';

    int code = 0;
    int length = 0;
    int parsed = sscanf(header, "%d %d", &code, &length);

    if (parsed < 2) {
        // Malformed or error-only header; read rest of line for context.
        char msg[256];
        size_t mpos = 0;
        while (mpos < sizeof(msg) - 1) {
            char c;
            ssize_t n = recv(sockfd, &c, 1, 0);
            if (n <= 0 || c == '\n') break;
            msg[mpos++] = c;
        }
        msg[mpos] = '\0';
        if (code != 0) {
            fprintf(stderr, "Read error code %d%s%s\n",
                    code,
                    mpos > 0 ? ": " : "",
                    mpos > 0 ? msg : "");
        } else {
            fprintf(stderr, "Malformed read response header: '%s%s'\n",
                    header, msg);
        }
        return;
    }

    if (code != 0) {
        // Error with valid numeric length (often 0); read and discard rest of line.
        char c;
        while (1) {
            ssize_t n = recv(sockfd, &c, 1, 0);
            if (n <= 0 || c == '\n') break;
        }
        fprintf(stderr, "Read error code %d\n", code);
        return;
    }

    if (length < 0) {
        fprintf(stderr, "Negative length in read response: %d\n", length);
        return;
    }

    char *data = NULL;
    if (length > 0) {
        data = malloc((size_t)length);
        if (!data) {
            fprintf(stderr, "Out of memory for read of %d bytes\n", length);
            return;
        }
        if (recv_all(sockfd, data, (size_t)length) != length) {
            perror("recv_all");
            free(data);
            return;
        }
    }

    // Consume trailing newline if present (server sends one after data).
    char c;
    ssize_t n = recv(sockfd, &c, 1, 0);
    if (n > 0 && c != '\n') {
        // Extra byte beyond the expected newline; ignore it.
    }

    printf("0 %d ", length);
    if (length > 0) {
        fwrite(data, 1, (size_t)length, stdout);
        free(data);
    }
    printf("\n");
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

    printf("Connected to filesystem server at %s:%d\n", server_ip, port);
    printf("Commands: F | C f | D f | L 0|1 | R f | W f len data | mkdir d | cd d | pwd | rmdir d | q\n");

    char line[BUFFER_SIZE];
    while (1) {
        printf("fs> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break; // EOF on stdin
        }

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }
        if (len == 0) continue;

        if (strcmp(line, "q") == 0 || strcmp(line, "quit") == 0) {
            break;
        }

        char cmd[16];
        if (sscanf(line, "%15s", cmd) != 1) {
            continue;
        }

        // Send command + newline to server.
        char sendbuf[BUFFER_SIZE];
        int slen = snprintf(sendbuf, sizeof(sendbuf), "%s\n", line);
        if (slen < 0 || slen >= (int)sizeof(sendbuf)) {
            fprintf(stderr, "Input too long.\n");
            continue;
        }

        if (send(sockfd, sendbuf, (size_t)slen, 0) != slen) {
            perror("send");
            break;
        }

        if (strcmp(cmd, "R") == 0) {
            // Special handling for read responses: parse length and read exactly that many bytes.
            handle_read_command(sockfd);
        } else {
            // Generic response: small, newline-terminated message or listing.
            char resp[BUFFER_SIZE];
            ssize_t nr = recv(sockfd, resp, sizeof(resp), 0);
            if (nr < 0) {
                perror("recv");
                break;
            }
            if (nr == 0) {
                printf("Server closed connection.\n");
                break;
            }
            if (write(STDOUT_FILENO, resp, (size_t)nr) < 0) {
                perror("write");
                break;
            }
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
