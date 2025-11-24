/*
 * reverse_server.c
 * Multi-threaded TCP server that reverses strings sent by clients.
 * 
 * This program demonstrates:
 * 1. TCP socket programming (server side)
 * 2. Multi-threading with pthreads (one thread per client)
 * 3. String manipulation algorithms
 * 4. Robust error handling for all system calls
 * 
 * Usage: ./reverse_server <port>
 * Example: ./reverse_server 8080
 */

#include <stdio.h>      // Standard I/O functions (printf, perror, fprintf)
#include <stdlib.h>     // Memory allocation and conversion functions (malloc, atoi, EXIT_FAILURE)
#include <string.h>     // String manipulation (strlen, memset, strcat)
#include <unistd.h>     // UNIX system calls (close, recv, send)
#include <errno.h>      // Error number definitions (used by perror)
#include <arpa/inet.h>  // Internet address functions (inet_pton, htons)
#include <netinet/in.h> // Internet protocol address structures (sockaddr_in)
#include <sys/socket.h> // Socket API (socket, bind, listen, accept)
#include <pthread.h>    // POSIX threads (pthread_create, pthread_detach)

#define MAX_LINE 1024   // Maximum length of string we'll accept from client
#define BACKLOG 10      // Maximum number of pending connections in listen queue

#define LOG_ENABLED 1

#if LOG_ENABLED
#define LOGF(fmt, ...) \
    fprintf(stderr, "[reverse_server][%d][%s] " fmt "\n", getpid(), __func__, ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) ((void)0)
#endif

/*
 * reverse_string - Reverses a C string in place using two-pointer technique
 * @s: The string to reverse (modified in place)
 * 
 * Algorithm: Start with pointers at beginning and end of string,
 * swap characters, move pointers inward until they meet.
 * Time Complexity: O(n) where n is string length
 * Space Complexity: O(1) - modifies string in place
 */
static void reverse_string(char *s) {
    LOGF("reverse_string: input_len=%zu", strlen(s));
    size_t i = 0;                    // Index from start of string
    size_t j = strlen(s);            // Index from end of string
    
    // Handle empty string case
    if (j == 0) return;
    
    j--;                             // Convert length to last valid index
    
    // Swap characters until pointers meet in middle
    while (i < j) {
        char tmp = s[i];             // Store character from start
        s[i] = s[j];                 // Move character from end to start
        s[j] = tmp;                  // Move stored character to end
        i++;                         // Move start pointer forward
        j--;                         // Move end pointer backward
    }
}

/*
 * handle_client - Thread function that handles one client connection
 * @arg: Pointer to client socket file descriptor (passed as void* for pthread compatibility)
 * 
 * This function:
 * 1. Receives a string from the client
 * 2. Strips newline characters
 * 3. Reverses the string
 * 4. Sends the reversed string back to client
 * 5. Cleans up and exits thread
 * 
 * Returns: NULL (required by pthread_create signature)
 */
static void *handle_client(void *arg) {
    LOGF("handle_client: thread started");
    // Extract client socket from void* argument
    int clientfd = *(int *)arg;
    free(arg);                       // Free the malloc'd socket descriptor storage
    
    char buf[MAX_LINE];              // Buffer to store received data
    
    // Receive data from client (max MAX_LINE-1 bytes to leave room for null terminator)
    ssize_t n = recv(clientfd, buf, MAX_LINE - 1, 0);
    if (n > 0) {
        // Null-terminate the received data to make it a valid C string
        buf[n] = '\0';
        
        // Remove trailing newline or carriage return characters
        // This makes the output cleaner when client sends "hello\n"
        if (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
            buf[n - 1] = '\0';
        }
        
        // Reverse the received string
        reverse_string(buf);
        
        // Add newline back so client output looks clean
        strcat(buf, "\n");
        
        // Send reversed string back to client
        // Note: strlen(buf) includes the newline we just added
        send(clientfd, buf, strlen(buf), 0);
    }
    
    // Close the client connection
    close(clientfd);
    
    // Thread must return void* pointer, NULL indicates successful completion
    return NULL;
}

/*
 * main - Entry point for the reverse string server
 * @argc: Number of command line arguments
 * @argv: Array of command line argument strings
 * 
 * Sets up TCP server, accepts connections, spawns threads to handle clients.
 * Implements robust error handling for all system calls as required.
 * 
 * Returns: EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char *argv[]) {
    LOGF("main: argc=%d", argc);
    // Validate command line arguments - we need exactly: program_name port
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;          // Non-zero exit code indicates error
    }
    
    // Convert port string to integer and validate
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
    
    // Create TCP socket for listening
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");            // Print system error message
        return EXIT_FAILURE;
    }
    
    // Set socket option to allow address reuse
    // This prevents "Address already in use" errors when restarting server quickly
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listenfd);              // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // Set up server address structure
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));   // Zero out the structure
    addr.sin_family      = AF_INET;   // IPv4 address family
    addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any network interface
    addr.sin_port        = htons(port); // Convert port to network byte order
    
    // Bind socket to the specified port
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listenfd);              // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // Start listening for incoming connections
    // BACKLOG = maximum number of pending connections
    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        close(listenfd);              // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // Inform user that server is ready
    printf("Reverse server listening on port %d\n", port);
    
    // Main server loop - accept connections forever
    while (1) {
        struct sockaddr_in cliaddr;   // Structure to store client address
        socklen_t clilen = sizeof(cliaddr);
        
        // Allocate memory for client socket descriptor to pass to thread
        // We malloc this because it needs to exist after this function returns
        int *clientfd = malloc(sizeof(int));
        if (!clientfd) {
            perror("malloc");
            continue;                 // Continue to next iteration instead of exiting
        }
        
        // Accept a new client connection
        // This blocks until a client connects
        *clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (*clientfd < 0) {
            perror("accept");
            free(clientfd);           // Clean up malloc'd memory
            continue;                 // Continue to next iteration
        }
        
        // Create a new thread to handle this client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, clientfd) != 0) {
            perror("pthread_create");
            close(*clientfd);         // Close client socket
            free(clientfd);           // Clean up malloc'd memory
            continue;                 // Continue to next iteration
        }
        
        // Detach the thread so it cleans up automatically when it finishes
        // This means we don't need to call pthread_join() later
        pthread_detach(tid);
    }
    
    // This code is unreachable due to infinite loop above
    // But included for completeness and good practice
    close(listenfd);
    return EXIT_SUCCESS;
}
