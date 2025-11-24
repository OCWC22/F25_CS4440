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
    // Helper: Read a line from the socket byte-by-byte until newline or max length.
    // This is necessary because TCP is a stream, not packet-based.
    size_t pos = 0;
    while (pos < maxlen - 1) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 0) break;      // EOF: Client closed connection
        if (n < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
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

    // Parse the command line into an argument array for execvp
    // argv[0] is forced to "ls", subsequent tokens are arguments (e.g. "-l", "-a")
    char *argv_ls[MAX_ARGS];
    int argc_ls = 0;

    argv_ls[argc_ls++] = "ls";

    char *saveptr = NULL;
    char *tok = strtok_r(line, " \t", &saveptr);
    while (tok && argc_ls < MAX_ARGS - 1) {
        argv_ls[argc_ls++] = tok;
        tok = strtok_r(NULL, " \t", &saveptr);
    }
    argv_ls[argc_ls] = NULL; // Null-terminate the array for execvp

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(clientfd);
        return;
    }

    if (pid == 0) {
        // Child process:
        // Redirect stdout (1) and stderr (2) to the client socket.
        // This ensures the output of 'ls' is sent over the network.
        dup2(clientfd, STDOUT_FILENO);
        dup2(clientfd, STDERR_FILENO);
        close(clientfd); // Close the copy of the socket descriptor

        // Execute the ls command; this replaces the child process image
        execvp("ls", argv_ls);
        perror("execvp"); // Only reached if execvp fails
        _exit(1);
    } else {
        // Parent process:
        // Wait for the child to finish to avoid zombie processes
        int status;
        waitpid(pid, &status, 0);
        close(clientfd); // Close connection to client
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);

    // 1. Create a TCP socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // 2. Set SO_REUSEADDR to allow immediate restart of server on the same port
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    // 3. Bind the socket to the port
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listenfd);
        return EXIT_FAILURE;
    }

    // 4. Start listening for incoming connections
    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        close(listenfd);
        return EXIT_FAILURE;
    }

    printf("ls server listening on port %d\n", port);

    // 5. Main server loop
    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);

        // Block until a client connects
        int clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }

        // Handle the client request
        // Note: This simple implementation handles clients sequentially (blocking).
        handle_client(clientfd);
    }

    close(listenfd);
    return EXIT_SUCCESS;
}
