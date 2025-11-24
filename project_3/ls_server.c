/*
 * ls_server.c
 * TCP server that executes 'ls' commands and streams output back to clients.
 * 
 * This program demonstrates:
 * 1. Process creation and management (fork, waitpid)
 * 2. Process execution (execvp)
 * 3. File descriptor redirection (dup2)
 * 4. TCP socket programming (server side)
 * 5. Command line parsing and tokenization
 * 6. Robust error handling for all system calls
 * 
 * Usage: ./ls_server <port>
 * Example: ./ls_server 8081
 */

#include <stdio.h>      // Standard I/O functions (printf, perror, fprintf)
#include <stdlib.h>     // Memory allocation and conversion functions (atoi, EXIT_FAILURE)
#include <string.h>     // String manipulation (strtok_r, memset)
#include <unistd.h>     // UNIX system calls (fork, dup2, close, recv, execvp, waitpid)
#include <errno.h>      // Error number definitions (used by perror, EINTR)
#include <arpa/inet.h>  // Internet address functions (htons)
#include <netinet/in.h> // Internet protocol address structures (sockaddr_in)
#include <sys/socket.h> // Socket API (socket, bind, listen, accept)
#include <sys/wait.h>   // Process wait functions (waitpid)

#define MAX_LINE 1024   // Maximum length of command line from client
#define MAX_ARGS 64     // Maximum number of arguments for ls command
#define BACKLOG 10      // Maximum number of pending connections in listen queue

#define LOG_ENABLED 1

#if LOG_ENABLED
#define LOGF(fmt, ...) \
    fprintf(stderr, "[ls_server][%d][%s] " fmt "\n", getpid(), __func__, ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) ((void)0)
#endif

/*
 * read_line - Read a complete line from a socket descriptor
 * @fd: Socket file descriptor to read from
 * @buf: Buffer to store the read line
 * @maxlen: Maximum number of bytes to read
 * 
 * TCP is stream-based, not message-based, so we need to read byte-by-byte
 * until we encounter a newline character or reach the buffer limit.
 * This handles the case where a client sends "ls -l /tmp" as a continuous stream.
 * 
 * Returns: Number of bytes read (excluding newline), -1 on error
 */
static ssize_t read_line(int fd, char *buf, size_t maxlen) {
    LOGF("read_line: fd=%d maxlen=%zu", fd, maxlen);
    size_t pos = 0;                     // Current position in buffer
    
    // Read one byte at a time until we hit newline or buffer limit
    while (pos < maxlen - 1) {
        char c;                         // Single character buffer
        ssize_t n = recv(fd, &c, 1, 0); // Read exactly one byte
        
        if (n == 0) break;              // EOF: Client closed connection
        if (n < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry read
            return -1;                  // Other error occurred
        }
        if (c == '\n') break;           // End of line reached
        
        buf[pos++] = c;                 // Store character and advance position
    }
    
    buf[pos] = '\0';                    // Null-terminate the string
    return (ssize_t)pos;                // Return number of characters read
}

/*
 * handle_client - Process a single client request
 * @clientfd: Socket file descriptor for the connected client
 * 
 * This function:
 * 1. Reads the ls command line from the client
 * 2. Parses it into arguments for execvp
 * 3. Forks a child process to execute ls
 * 4. Redirects child's stdout/stderr to the client socket
 * 5. Waits for child to complete and cleans up
 */
static void handle_client(int clientfd) {
    LOGF("handle_client: clientfd=%d", clientfd);
    char line[MAX_LINE];                // Buffer to store command line from client
    
    // Read the complete command line from the client
    ssize_t n = read_line(clientfd, line, sizeof(line));
    if (n <= 0) {
        close(clientfd);                // Close connection on error or EOF
        return;
    }
    
    // Parse the command line into argument array for execvp
    // execvp expects: argv[0] = program name, argv[1..n] = arguments, argv[n+1] = NULL
    char *argv_ls[MAX_ARGS];            // Argument array for ls command
    int argc_ls = 0;                    // Current argument count
    
    argv_ls[argc_ls++] = "ls";          // argv[0] is always "ls" (program name)
    
    // Tokenize the input line into separate arguments
    // strtok_r is thread-safe version of strtok
    char *saveptr = NULL;               // Save pointer for strtok_r thread safety
    char *tok = strtok_r(line, " \t", &saveptr); // Split on spaces and tabs
    
    // Process each token until we run out or hit argument limit
    while (tok && argc_ls < MAX_ARGS - 1) {
        argv_ls[argc_ls++] = tok;       // Store argument and increment count
        tok = strtok_r(NULL, " \t", &saveptr); // Get next token
    }
    
    argv_ls[argc_ls] = NULL;            // Null-terminate array (required by execvp)
    
    // Fork a child process to execute the ls command
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");                 // Fork failed
        close(clientfd);
        return;
    }
    
    if (pid == 0) {
        // CHILD PROCESS:
        // This process will execute the ls command
        
        // Redirect stdout (file descriptor 1) to client socket
        // This means anything written to stdout goes over the network
        dup2(clientfd, STDOUT_FILENO);
        
        // Redirect stderr (file descriptor 2) to client socket
        // This captures error messages and sends them to client
        dup2(clientfd, STDERR_FILENO);
        
        // Close the original socket descriptor (we have duplicates now)
        close(clientfd);
        
        // Execute the ls command with the parsed arguments
        // execvp searches PATH for "ls" and replaces the current process
        // If execvp succeeds, this line never executes
        execvp("ls", argv_ls);
        
        // We only reach here if execvp failed
        perror("execvp");
        _exit(1);                       // Exit child process with error code
    } else {
        // PARENT PROCESS:
        // This process waits for the child to complete
        
        int status;                     // Status variable for waitpid
        
        // Wait for child process to terminate
        // This prevents zombie processes and ensures we don't close connection too early
        waitpid(pid, &status, 0);
        
        // Close the client connection
        close(clientfd);
    }
}

/*
 * main - Entry point for the ls server
 * @argc: Number of command line arguments
 * @argv: Array of command line argument strings
 * 
 * Sets up TCP server, accepts connections, and handles ls command requests.
 * Implements robust error handling for all system calls as required.
 * 
 * Returns: EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char *argv[]) {
    LOGF("main: argc=%d", argc);
    // Validate command line arguments - we need exactly: program_name port
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Extract port number from command line and validate
    int port = atoi(argv[1]);
    
    // Validate port range (must be between 1024 and 65535 for non-root users)
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Port must be between 1 and 65535\n");
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Additional validation: ports below 1024 require root privileges
    if (port < 1024) {
        fprintf(stderr, "Error: Ports below 1024 require root privileges\n");
        fprintf(stderr, "Please use a port between 1024 and 65535\n");
        return EXIT_FAILURE;
    }
    
    // Step 1: Create TCP socket for listening
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    
    // Step 2: Set socket option to allow address reuse
    // This prevents "Address already in use" errors when restarting server
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listenfd);
        return EXIT_FAILURE;
    }
    
    // Step 3: Set up server address structure
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));     // Zero out the structure
    addr.sin_family      = AF_INET;     // IPv4 address family
    addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any interface
    addr.sin_port        = htons(port); // Convert port to network byte order
    
    // Step 4: Bind socket to the specified port
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listenfd);
        return EXIT_FAILURE;
    }
    
    // Step 5: Start listening for incoming connections
    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        close(listenfd);
        return EXIT_FAILURE;
    }
    
    // Inform user that server is ready
    printf("ls server listening on port %d\n", port);
    
    // Step 6: Main server loop - handle clients one at a time
    while (1) {
        struct sockaddr_in cliaddr;     // Structure to store client address
        socklen_t clilen = sizeof(cliaddr);
        
        // Block until a client connects
        int clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (clientfd < 0) {
            perror("accept");
            continue;                   // Continue to next iteration on error
        }
        
        // Handle the client request (blocking - handles clients sequentially)
        // Note: This simple implementation doesn't use threads like reverse_server
        handle_client(clientfd);
    }
    
    // This code is unreachable due to infinite loop above
    close(listenfd);
    return EXIT_SUCCESS;
}
