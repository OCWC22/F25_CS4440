/*
 * reverse_client.c
 * TCP client that sends strings to reverse_server and prints the reversed results.
 * 
 * This program demonstrates:
 * 1. TCP socket programming (client side)
 * 2. Command line argument parsing and validation
 * 3. Network byte order conversion
 * 4. User input handling and network communication
 * 5. Robust error handling for all system calls
 * 
 * Usage: ./reverse_client <server-ip> <port>
 * Example: ./reverse_client 127.0.0.1 8080
 */

#include <stdio.h>      // Standard I/O functions (printf, perror, fprintf, fgets)
#include <stdlib.h>     // Memory allocation and conversion functions (atoi, EXIT_FAILURE)
#include <string.h>     // String manipulation (strlen, memset)
#include <unistd.h>     // UNIX system calls (close, recv, send)
#include <errno.h>      // Error number definitions (used by perror)
#include <arpa/inet.h>  // Internet address functions (inet_pton, htons)
#include <netinet/in.h> // Internet protocol address structures (sockaddr_in)
#include <sys/socket.h> // Socket API (socket, connect)

#define MAX_LINE 1024   // Maximum length of string we'll send to server

#define LOG_ENABLED 1

#if LOG_ENABLED
#define LOGF(fmt, ...) \
    fprintf(stderr, "[reverse_client][%d][%s] " fmt "\n", getpid(), __func__, ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) ((void)0)
#endif

/*
 * main - Entry point for the reverse string client
 * @argc: Number of command line arguments
 * @argv: Array of command line argument strings
 * 
 * Connects to server, gets user input, sends to server, receives and displays result.
 * Implements robust error handling for all system calls as required.
 * 
 * Returns: EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char *argv[]) {
    LOGF("main: argc=%d", argc);
    // Validate command line arguments - we need exactly: program_name server_ip port
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        return EXIT_FAILURE;          // Non-zero exit code indicates error
    }
    
    // Extract server IP and port from command line arguments
    const char *server_ip = argv[1];  // Server IP address (e.g., "127.0.0.1")
    int port = atoi(argv[2]);         // Server port number (e.g., 8080)
    
    // Validate port range
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Port must be between 1 and 65535\n");
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Create TCP socket for communication with server
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");            // Print system error message
        return EXIT_FAILURE;
    }
    
    // Set up server address structure
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr)); // Zero out the structure
    servaddr.sin_family = AF_INET;          // IPv4 address family
    servaddr.sin_port   = htons(port);      // Convert port to network byte order
    
    // Convert IP address from text to binary form
    // inet_pton returns 1 on success, 0 on invalid input, -1 on error
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);                  // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // Connect to the server
    // This blocks until the connection is established or fails
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);                  // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // Get input from user to send to server
    char buf[MAX_LINE];                 // Buffer to store user input
    printf("Enter a string to reverse: ");
    
    // Read line from standard input (includes newline if present)
    // fgets returns NULL on EOF or error
    if (!fgets(buf, sizeof(buf), stdin)) {
        fprintf(stderr, "No input.\n");
        close(sockfd);                  // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // Send the user's input to the server
    size_t len = strlen(buf);           // Length of string to send (includes newline)
    
    // send() returns number of bytes sent, or -1 on error
    // We check that all bytes were sent (TCP can send partial data)
    if (send(sockfd, buf, len, 0) != (ssize_t)len) {
        perror("send");
        close(sockfd);                  // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // Receive the reversed string from the server
    // sizeof(buf) - 1 leaves room for null terminator
    ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        perror("recv");
        close(sockfd);                  // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // Null-terminate the received data to make it a valid C string
    buf[n] = '\0';
    
    // Display the reversed string received from server
    printf("Reversed from server: %s", buf);
    
    // Close the connection to the server
    close(sockfd);
    
    // Return success
    return EXIT_SUCCESS;
}
