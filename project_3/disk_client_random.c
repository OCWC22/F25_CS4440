/*
 * disk_client_random.c
 * Automated stress-testing client for the disk storage server.
 * 
 * This program demonstrates:
 * 1. TCP socket programming (client side)
 * 2. Random number generation for reproducible testing
 * 3. Automated protocol communication with disk server
 * 4. Stress testing patterns for storage systems
 * 5. Binary data handling for disk operations
 * 6. Robust error handling for all system calls
 * 
 * Testing Strategy:
 * - Connects to disk server and queries geometry
 * - Generates N random read/write operations
 * - Uses deterministic seed for reproducible tests
 * - Progress indicators for monitoring test execution
 * 
 * Usage: ./disk_client_random <server-ip> <port> <N> <seed>
 * Example: ./disk_client_random 127.0.0.1 8082 1000 42
 */

#define _POSIX_C_SOURCE 200809L         // Enable POSIX 2008 features

#include <stdio.h>      // Standard I/O functions (printf, perror, fprintf)
#include <stdlib.h>     // Memory allocation and conversion functions (atoi, atoll, strtoul, srand, rand)
#include <string.h>     // String manipulation (memset)
#include <unistd.h>     // UNIX system calls (close, recv, send)
#include <errno.h>      // Error number definitions (used by perror, EINTR)
#include <time.h>       // Time functions (used for random seed generation)
#include <arpa/inet.h>  // Internet address functions (inet_pton, htons)
#include <netinet/in.h> // Internet protocol address structures (sockaddr_in)
#include <sys/socket.h> // Socket API (socket, connect)

#define BLOCK_SIZE 128   // Fixed block size for disk operations (matches server)
#define MAX_LINE   128  // Maximum length of server response lines

#define LOG_ENABLED 1

#if LOG_ENABLED
#define LOGF(fmt, ...) \
    fprintf(stderr, "[disk_client_random][%d][%s] " fmt "\n", getpid(), __func__, ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) ((void)0)
#endif

/*
 * recv_all - Receive exactly the specified number of bytes from a socket
 * @fd: Socket file descriptor to read from
 * @buf: Buffer to store the received data
 * @len: Exact number of bytes to receive
 * 
 * TCP is stream-based and can deliver partial data, so we need to loop
 * until we receive the complete message. This is critical for receiving
 * the exact 128-byte blocks from the disk server during stress testing.
 * 
 * Returns: Number of bytes received on success, -1 on error
 */
static ssize_t recv_all(int fd, void *buf, size_t len) {
    LOGF("recv_all: fd=%d len=%zu", fd, len);
    size_t done = 0;                    // Number of bytes received so far
    
    // Keep receiving until we have all requested bytes
    while (done < len) {
        // Receive remaining bytes into the next available buffer position
        ssize_t n = recv(fd, (char *)buf + done, len - done, 0);
        
        if (n == 0) return done;        // EOF: server closed connection
        if (n < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            return -1;                  // Other error occurred
        }
        
        done += n;                     // Update progress
    }
    
    return done;                      // Return total bytes received
}

/*
 * recv_line - Read a complete line from a socket descriptor
 * @fd: Socket file descriptor to read from
 * @buf: Buffer to store the read line
 * @maxlen: Maximum number of bytes to read
 * 
 * Reads characters one at a time until newline is encountered.
 * This is necessary for parsing variable-length text responses
 * like the geometry information from the disk server.
 * 
 * Returns: Number of characters read (excluding newline), -1 on error
 */
static ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    LOGF("recv_line: fd=%d maxlen=%zu", fd, maxlen);
    size_t pos = 0;                     // Current position in buffer
    
    // Read one byte at a time until we hit newline or buffer limit
    while (pos < maxlen - 1) {
        char c;                         // Single character buffer
        ssize_t n = recv(fd, &c, 1, 0); // Read exactly one byte
        
        if (n == 0) break;              // EOF: server closed connection
        if (n < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            return -1;                  // Other error occurred
        }
        if (c == '\n') break;           // End of line reached
        
        buf[pos++] = c;                 // Store character and advance position
    }
    
    buf[pos] = '\0';                    // Null-terminate the string
    return (ssize_t)pos;                // Return number of characters read
}

/*
 * main - Entry point for the automated disk stress tester
 * @argc: Number of command line arguments
 * @argv: Array of command line argument strings
 * 
 * Connects to disk server and performs N random read/write operations
 * for stress testing. Uses deterministic seed for reproducible results.
 * Implements robust error handling for all system calls as required.
 * 
 * Returns: EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char *argv[]) {
    LOGF("main: argc=%d", argc);
    // STEP 1: Validate and parse command line arguments
    // We need exactly: program_name server_ip port N seed
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server-ip> <port> <N> <seed>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Extract and parse command line arguments
    const char *server_ip = argv[1];    // Server IP address (e.g., "127.0.0.1")
    int port = atoi(argv[2]);           // Server port number (e.g., 8082)
    long long N = atoll(argv[3]);       // Number of random operations to perform
    unsigned int seed = (unsigned int)strtoul(argv[4], NULL, 10); // Random seed
    LOGF("args: server_ip=%s port=%d N=%lld seed=%u", server_ip, port, N, seed);
    
    // Validate port range
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Port must be between 1 and 65535\n");
        fprintf(stderr, "Usage: %s <server-ip> <port> <N> <seed>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Validate the number of operations
    if (N <= 0) {
        fprintf(stderr, "N must be positive.\n");
        return EXIT_FAILURE;
    }
    
    // Initialize random number generator with provided seed
    // This ensures reproducible test results across multiple runs
    srand(seed);
    
    // STEP 2: Create TCP socket and connect to server
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");              // Print system error message
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
    
    // Connect to the disk server
    // This blocks until the connection is established or fails
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);                  // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // STEP 3: Query disk geometry from server
    printf("Querying disk geometry...\n");
    
    // Send "I" command to get disk geometry information
    if (send(sockfd, "I", 1, 0) != 1) {
        perror("send geometry request");
        close(sockfd);
        return EXIT_FAILURE;
    }
    
    // Receive geometry response from server
    char response[MAX_LINE];
    ssize_t n = recv_line(sockfd, response, sizeof(response));
    if (n <= 0) {
        perror("recv geometry response");
        close(sockfd);
        return EXIT_FAILURE;
    }
    
    // Parse geometry: "cylinders sectors_per_cylinder"
    int cylinders, sectors_per_cyl;
    if (sscanf(response, "%d %d", &cylinders, &sectors_per_cyl) != 2) {
        fprintf(stderr, "Invalid geometry response: %s\n", response);
        close(sockfd);
        return EXIT_FAILURE;
    }
    
    printf("Disk geometry: %d cylinders, %d sectors/cylinder\n", 
           cylinders, sectors_per_cyl);
    
    // Validate geometry parameters
    if (cylinders <= 0 || sectors_per_cyl <= 0) {
        fprintf(stderr, "Invalid disk geometry.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }
    
    // STEP 4: Perform N random disk operations
    printf("Performing %lld random operations...\n", N);
    
    // Generate test data for write operations
    unsigned char test_data[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++) {
        test_data[i] = (unsigned char)(rand() % 256); // Random byte values
    }
    
    // Main stress test loop
    for (long long i = 0; i < N; i++) {
        // Randomly choose between read (50%) and write (50%) operations
        int is_write = (rand() % 2) == 1;
        
        // Generate random cylinder and sector coordinates
        int cylinder = rand() % cylinders;
        int sector = rand() % sectors_per_cyl;
        
        if (is_write) {
            // --- PERFORM WRITE OPERATION ---
            // Send write command: "W cylinder sector length"
            char cmd[64];
            int cmd_len = snprintf(cmd, sizeof(cmd), "W %d %d %d", 
                                 cylinder, sector, BLOCK_SIZE);
            
            if (send(sockfd, cmd, cmd_len, 0) != cmd_len) {
                perror("send write command");
                break;
            }
            
            // Send test data for the write operation
            if (send(sockfd, test_data, BLOCK_SIZE, 0) != BLOCK_SIZE) {
                perror("send write data");
                break;
            }
            
            // Receive write confirmation from server
            char confirm[64];
            n = recv_line(sockfd, confirm, sizeof(confirm));
            if (n <= 0) {
                perror("recv write confirmation");
                break;
            }
            
            putchar('W');               // Show progress indicator
            fflush(stdout);             // Ensure immediate display
            
        } else {
            // --- PERFORM READ OPERATION ---
            // Send read command: "R cylinder sector"
            char cmd[64];
            int cmd_len = snprintf(cmd, sizeof(cmd), "R %d %d", 
                                 cylinder, sector);
            
            if (send(sockfd, cmd, cmd_len, 0) != cmd_len) {
                perror("send read command");
                break;
            }
            
            // Receive read response from server
            char response[256];
            n = recv_line(sockfd, response, sizeof(response));
            if (n <= 0) {
                perror("recv read response");
                break;
            }
            
            // If response indicates success, receive the 128-byte block data
            if (response[0] == '0') {
                unsigned char block[BLOCK_SIZE];
                if (recv_all(sockfd, block, BLOCK_SIZE) < 0) {
                    perror("recv block data");
                    break;
                }
                // Note: We ignore the actual data in stress testing
            }
            
            putchar('R');               // Show progress indicator
            fflush(stdout);             // Ensure immediate display
        }
        
        // Optional: Add small delay to avoid overwhelming the server
        // usleep(1000); // Uncomment if needed for testing
    }
    
    putchar('\n');                     // New line after progress indicators
    printf("Completed %lld random operations.\n", N);
    
    // STEP 5: Clean up and exit
    close(sockfd);
    return EXIT_SUCCESS;
}
