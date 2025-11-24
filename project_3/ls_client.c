/*
 * ls_client.c
 * TCP client that sends ls command arguments to ls_server and displays directory listing.
 * 
 * This program demonstrates:
 * 1. TCP socket programming (client side)
 * 2. Command line argument parsing and validation
 * 3. String manipulation and concatenation
 * 4. Network communication and response handling
 * 5. Robust error handling for all system calls
 * 
 * Usage: ./ls_client <server-ip> <port> [ls-args...]
 * Examples: ./ls_client 127.0.0.1 8081 -l /tmp
 *           ./ls_client 127.0.0.1 8081 -a /home
 */

#include <stdio.h>      // Standard I/O functions (printf, perror, fprintf)
#include <stdlib.h>     // Memory allocation and conversion functions (atoi, EXIT_FAILURE)
#include <string.h>     // String manipulation (strlen, memset, strcat)
#include <unistd.h>     // UNIX system calls (close, recv, write)
#include <errno.h>      // Error number definitions (used by perror)
#include <arpa/inet.h>  // Internet address functions (inet_pton, htons)
#include <netinet/in.h> // Internet protocol address structures (sockaddr_in)
#include <sys/socket.h> // Socket API (socket, connect)

#define MAX_LINE 1024   // Maximum length of command line to send to server

#define LOG_ENABLED 1

#if LOG_ENABLED
#define LOGF(fmt, ...) \
    fprintf(stderr, "[ls_client][%d][%s] " fmt "\n", getpid(), __func__, ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) ((void)0)
#endif

/*
 * main - Entry point for the ls client
 * @argc: Number of command line arguments
 * @argv: Array of command line argument strings
 * 
 * Connects to server, sends ls command arguments, receives and displays directory listing.
 * Implements robust error handling for all system calls as required.
 * 
 * Returns: EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char *argv[]) {
    LOGF("main: argc=%d", argc);
    // STEP 1: Validate command line arguments
    // We need at least: program_name server_ip port
    // Additional arguments are passed directly to the ls command
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port> [ls-args...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Extract server IP and port from command line
    const char *server_ip = argv[1];  // Server IP address (e.g., "127.0.0.1")
    int port = atoi(argv[2]);         // Server port number (e.g., 8081)
    
    // Validate port range
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Port must be between 1 and 65535\n");
        fprintf(stderr, "Usage: %s <server-ip> <port> [ls-args...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // STEP 2: Create TCP socket for communication with server
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");            // Print system error message
        return EXIT_FAILURE;
    }
    
    // STEP 3: Set up server address structure and connect
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
    
    // STEP 4: Build command string from command line arguments
    // Concatenate all arguments after the first two (IP and port) into one line
    // Example: "-l /tmp" becomes "-l /tmp\n" for the server
    char line[MAX_LINE] = {0};           // Initialize empty command string
    
    // Process each argument starting from index 3 (first ls argument)
    for (int i = 3; i < argc; i++) {
        // Check if adding this argument would overflow our buffer
        // +2 accounts for space between arguments and null terminator
        if (strlen(line) + strlen(argv[i]) + 2 >= sizeof(line)) {
            fprintf(stderr, "Too many/long arguments.\n");
            close(sockfd);              // Clean up socket before exit
            return EXIT_FAILURE;
        }
        
        // Add the argument to our command string
        strcat(line, argv[i]);
        
        // Add space between arguments (but not after the last one)
        if (i != argc - 1) {
            strcat(line, " ");
        }
    }
    
    // Add newline to mark end of command for the server
    strcat(line, "\n");
    
    // STEP 5: Send the command string to the server
    size_t len = strlen(line);           // Length of command to send
    
    // send() returns number of bytes sent, or -1 on error
    // We check that all bytes were sent (TCP can send partial data)
    if (send(sockfd, line, len, 0) != (ssize_t)len) {
        perror("send");
        close(sockfd);                  // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // STEP 6: Read the server's response and display it
    // The server sends the ls output, which we display until the connection closes
    char buf[MAX_LINE];                 // Buffer for receiving data
    ssize_t n;                          // Number of bytes received
    
    // Keep receiving data until server closes connection (recv returns 0)
    while ((n = recv(sockfd, buf, sizeof(buf), 0)) > 0) {
        // Write received data directly to stdout
        // This preserves the exact formatting of the ls output
        if (write(STDOUT_FILENO, buf, n) < 0) {
            perror("write");
            break;                      // Exit loop on write error
        }
    }
    
    // Check if recv failed with an error (as opposed to normal EOF)
    if (n < 0) {
        perror("recv");
    }
    
    // Close the connection to the server
    close(sockfd);
    
    // Return success
    return EXIT_SUCCESS;
}
