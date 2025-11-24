/*
 * disk_server.c
 * Simulated disk storage server implementing the disk protocol from the project specification.
 * 
 * This program demonstrates:
 * 1. Memory-mapped file I/O (mmap, munmap)
 * 2. Disk geometry simulation (cylinders, sectors, tracks)
 * 3. Mechanical disk delay simulation (seek time)
 * 4. TCP socket programming with custom protocol
 * 5. Binary data handling and block-based storage
 * 6. Robust error handling for all system calls
 * 
 * Protocol Commands:
 *   I - Return disk geometry (cylinders sectors_per_cylinder)
 *   R c s - Read block at cylinder c, sector s (128 bytes)
 *   W c s data - Write data to block at cylinder c, sector s
 * 
 * Usage: ./disk_server <port> <cylinders> <sectors_per_cylinder> <track_time_us> <backing_file>
 * Example: ./disk_server 8082 4 4 1000 disk.img
 */

#define _POSIX_C_SOURCE 200809L         // Enable POSIX 2008 features for mmap

#include <stdio.h>      // Standard I/O functions (printf, perror, fprintf)
#include <stdlib.h>     // Memory allocation and conversion functions (atoi, EXIT_FAILURE)
#include <string.h>     // String manipulation (memset)
#include <unistd.h>     // UNIX system calls (close, recv, send, usleep)
#include <errno.h>      // Error number definitions (used by perror, EINTR)
#include <ctype.h>      // Character classification functions (isspace)
#include <fcntl.h>      // File control operations (open, O_CREAT)
#include <sys/mman.h>   // Memory management (mmap, munmap)
#include <sys/stat.h>   // File status (stat, S_IRUSR, S_IWUSR)
#include <sys/types.h>  // System data types (ssize_t, size_t)
#include <arpa/inet.h>  // Internet address functions (htons)
#include <netinet/in.h> // Internet protocol address structures (sockaddr_in)
#include <sys/socket.h> // Socket API (socket, bind, listen, accept)

#define BLOCK_SIZE 128   // Fixed block size for all disk operations (128 bytes)
#define BACKLOG    10    // Maximum number of pending connections in listen queue

#define LOG_ENABLED 1

#if LOG_ENABLED
#define LOGF(fmt, ...) \
    fprintf(stderr, "[disk_server][%d][%s] " fmt "\n", getpid(), __func__, ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) ((void)0)
#endif

// Global disk state variables
// These are global because they represent the physical disk characteristics
static int cylinders;              // Number of cylinders on the simulated disk
static int sectors_per_cyl;        // Number of sectors per cylinder
static int track_time_us;          // Time in microseconds to move between adjacent tracks
static unsigned char *disk_data;   // Memory-mapped disk storage (array of disk blocks)
static long long total_blocks;     // Total number of blocks on the disk

/*
 * recv_all - Receive exactly the specified number of bytes from a socket
 * @fd: Socket file descriptor to read from
 * @buf: Buffer to store the received data
 * @len: Exact number of bytes to receive
 * 
 * TCP is stream-based and can deliver partial data, so we need to loop
 * until we receive the complete message. This handles the case where
 * the network splits a large message into multiple packets.
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
        
        if (n == 0) return done;        // EOF: client closed connection
        if (n < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            return -1;                  // Other error occurred
        }
        
        done += n;                     // Update progress
    }
    
    return done;                      // Return total bytes received
}

/*
 * read_token - Read the next space-delimited token from socket stream
 * @fd: Socket file descriptor to read from
 * @buf: Buffer to store the token
 * @maxlen: Maximum length of token to read
 * 
 * The disk protocol uses space-separated tokens (e.g., "R 10 5").
 * This function reads characters until whitespace is encountered,
 * handling the case where tokens arrive across packet boundaries.
 * 
 * Returns: Number of characters in token, 0 on EOF, -1 on error
 */
static ssize_t read_token(int fd, char *buf, size_t maxlen) {
    LOGF("read_token: fd=%d maxlen=%zu", fd, maxlen);
    int started = 0;                   // Flag: have we started reading the token?
    size_t pos = 0;                    // Current position in buffer
    
    while (1) {
        char c;                        // Single character buffer
        ssize_t n = recv(fd, &c, 1, 0); // Read exactly one byte
        
        if (n == 0) {                  // EOF: client closed connection
            if (started) break;        // If we were reading a token, finish it
            return 0;                  // Otherwise, return EOF
        }
        if (n < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            return -1;                  // Other error occurred
        }
        
        if (!started) {
            // Skip leading whitespace before token starts
            if (isspace((unsigned char)c)) {
                continue;
            }
            started = 1;               // We've started reading the token
        }
        
        // If we hit whitespace, the token is complete
        if (isspace((unsigned char)c)) {
            break;
        }
        
        // Store character if we have room in buffer
        if (pos < maxlen - 1) {
            buf[pos++] = c;
        }
        // If buffer is full, we still consume characters until whitespace
        // but truncate the token (this is a safety measure)
    }
    
    buf[pos] = '\0';                   // Null-terminate the token
    return (ssize_t)pos;               // Return token length
}

/*
 * simulate_seek - Simulate mechanical disk head movement delay
 * @current_cyl: Pointer to current cylinder position
 * @target_cyl: Target cylinder to move to
 * 
 * Real mechanical disks have physical limitations - the read/write head
 * must physically move to the correct cylinder, which takes time.
 * This function simulates that delay using usleep based on the track time.
 * 
 * The delay is proportional to the distance between cylinders.
 */
static void simulate_seek(int *current_cyl, int target_cyl) {
    LOGF("simulate_seek: current=%d target=%d", *current_cyl, target_cyl);
    // Calculate absolute distance between current and target cylinders
    int diff = target_cyl - *current_cyl;
    if (diff < 0) diff = -diff;        // Make distance positive
    
    // Only simulate delay if we're actually moving and delay is configured
    if (diff > 0 && track_time_us > 0) {
        // Calculate total delay: distance × time per track
        useconds_t delay = (useconds_t)((long long)diff * track_time_us);
        usleep(delay);                 // Sleep to simulate mechanical movement
    }
    
    // Update current position to target
    *current_cyl = target_cyl;
}

/*
 * handle_connection - Process disk protocol commands from a single client
 * @clientfd: Socket file descriptor for the connected client
 * 
 * This function implements the disk protocol:
 * 1. Read command tokens (I, R, W)
 * 2. Parse arguments (cylinder, sector, data)
 * 3. Validate coordinates
 * 4. Simulate seek delays
 * 5. Perform read/write operations on memory-mapped disk
 * 6. Send responses back to client
 */
static void handle_connection(int clientfd) {
    LOGF("handle_connection: clientfd=%d", clientfd);
    char tok[64];                      // Buffer for reading command tokens
    int current_cyl = 0;               // Track current head position for seek simulation
    
    // Main command processing loop - continues until client disconnects
    while (1) {
        // Read the next command token (I, R, or W)
        ssize_t n = read_token(clientfd, tok, sizeof(tok));
        if (n <= 0) break;             // EOF or error, exit loop
        
        // Process INFO command - return disk geometry
        if (strcmp(tok, "I") == 0) {
            LOGF("command I (info)");
            char response[64];
            // Format: "cylinders sectors_per_cylinder\n"
            int len = snprintf(response, sizeof(response), "%d %d\n", 
                              cylinders, sectors_per_cyl);
            send(clientfd, response, len, 0);
            continue;
        }
        
        // Process READ command - read block at specified cylinder/sector
        if (strcmp(tok, "R") == 0) {
            // Read cylinder number
            n = read_token(clientfd, tok, sizeof(tok));
            if (n <= 0) break;
            int cyl = atoi(tok);
            LOGF("command R: cyl=%d", cyl);
            
            // Read sector number
            n = read_token(clientfd, tok, sizeof(tok));
            if (n <= 0) break;
            int sec = atoi(tok);
            LOGF("command R: sec=%d", sec);
            
            // Validate coordinates are within disk bounds
            if (cyl < 0 || cyl >= cylinders || sec < 0 || sec >= sectors_per_cyl) {
                const char *msg = "2 Invalid read (c or s out of range)\n";
                send(clientfd, msg, strlen(msg), 0);
                continue;
            }
            
            // Simulate mechanical seek delay
            simulate_seek(&current_cyl, cyl);
            
            // Calculate block index and send data
            long long block_idx = (long long)cyl * sectors_per_cyl + sec;
            unsigned char *block = disk_data + block_idx * BLOCK_SIZE;
            
            // Send success response followed by block data
            const char *msg = "0 Read OK. First 128 bytes (printable chars / dots):\n";
            send(clientfd, msg, strlen(msg), 0);
            send(clientfd, block, BLOCK_SIZE, 0);
            continue;
        }
        
        // Process WRITE command - write data to specified cylinder/sector
        if (strcmp(tok, "W") == 0) {
            // Read cylinder number
            n = read_token(clientfd, tok, sizeof(tok));
            if (n <= 0) break;
            int cyl = atoi(tok);
            LOGF("command W: cyl=%d", cyl);
            
            // Read sector number
            n = read_token(clientfd, tok, sizeof(tok));
            if (n <= 0) break;
            int sec = atoi(tok);
            LOGF("command W: sec=%d", sec);
            
            // Read data length
            n = read_token(clientfd, tok, sizeof(tok));
            if (n <= 0) break;
            int data_len = atoi(tok);
            LOGF("command W: data_len=%d", data_len);
            
            // Validate data length
            if (data_len < 0 || data_len > BLOCK_SIZE) {
                const char *msg = "3 Invalid write length\n";
                send(clientfd, msg, strlen(msg), 0);
                continue;
            }
            
            // Validate coordinates
            if (cyl < 0 || cyl >= cylinders || sec < 0 || sec >= sectors_per_cyl) {
                const char *msg = "2 Invalid write (c or s out of range)\n";
                send(clientfd, msg, strlen(msg), 0);
                continue;
            }
            
            // Simulate mechanical seek delay
            simulate_seek(&current_cyl, cyl);
            
            // Calculate block index
            long long block_idx = (long long)cyl * sectors_per_cyl + sec;
            unsigned char *block = disk_data + block_idx * BLOCK_SIZE;
            
            // Receive actual data from client
            if (data_len > 0) {
                if (recv_all(clientfd, block, data_len) < 0) break;
            }
            
            // Zero out any remaining bytes in the block
            if (data_len < BLOCK_SIZE) {
                memset(block + data_len, 0, BLOCK_SIZE - data_len);
            }
            
            // Send success response
            const char *msg = "0 Write OK.\n";
            send(clientfd, msg, strlen(msg), 0);
            continue;
        }
        
        // Unknown command - send error response
        const char *msg = "4 Unknown command\n";
        send(clientfd, msg, strlen(msg), 0);
    }
    
    close(clientfd);                   // Close connection when done
}

/*
 * main - Entry point for the disk server
 * @argc: Number of command line arguments
 * @argv: Array of command line argument strings
 * 
 * Sets up TCP server, creates and memory-maps disk file, accepts connections,
 * and handles disk protocol requests with mechanical delay simulation.
 * 
 * Returns: EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char *argv[]) {
    LOGF("main: argc=%d", argc);
    // Validate command line arguments
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <port> <cylinders> <sectors_per_cylinder> <track_time_us> <backing_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Parse command line arguments
    int port = atoi(argv[1]);
    cylinders = atoi(argv[2]);
    sectors_per_cyl = atoi(argv[3]);
    track_time_us = atoi(argv[4]);
    const char *filename = argv[5];
    LOGF("parsed args: port=%d cylinders=%d sectors_per_cyl=%d track_time_us=%d file=%s", port, cylinders, sectors_per_cyl, track_time_us, filename);
    
    // Validate port range
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Port must be between 1 and 65535\n");
        fprintf(stderr, "Usage: %s <port> <cylinders> <sectors_per_cylinder> <track_time_us> <backing_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Additional validation: ports below 1024 require root privileges
    if (port < 1024) {
        fprintf(stderr, "Error: Ports below 1024 require root privileges\n");
        fprintf(stderr, "Please use a port between 1024 and 65535\n");
        return EXIT_FAILURE;
    }
    
    // Validate disk geometry parameters
    if (cylinders <= 0 || sectors_per_cyl <= 0) {
        fprintf(stderr, "Invalid cylinders or sectors.\n");
        return EXIT_FAILURE;
    }
    
    // Calculate total number of blocks
    total_blocks = (long long)cylinders * sectors_per_cyl;
    
    // Create and initialize the disk file
    int fd = open(filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }
    
    // Ensure the file is large enough for the entire disk
    if (ftruncate(fd, total_blocks * BLOCK_SIZE) < 0) {
        perror("ftruncate");
        close(fd);
        return EXIT_FAILURE;
    }
    
    // Memory-map the disk file for direct array access
    disk_data = mmap(NULL, total_blocks * BLOCK_SIZE, PROT_READ | PROT_WRITE, 
                     MAP_SHARED, fd, 0);
    if (disk_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    
    // Close file descriptor - we don't need it after mmap
    close(fd);
    
    // Set up TCP server (standard socket setup code)
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        munmap(disk_data, total_blocks * BLOCK_SIZE);
        return EXIT_FAILURE;
    }
    
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listenfd);
        munmap(disk_data, total_blocks * BLOCK_SIZE);
        return EXIT_FAILURE;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listenfd);
        munmap(disk_data, total_blocks * BLOCK_SIZE);
        return EXIT_FAILURE;
    }
    
    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        close(listenfd);
        munmap(disk_data, total_blocks * BLOCK_SIZE);
        return EXIT_FAILURE;
    }
    
    printf("Disk server listening on port %d (cyl=%d, sec/cyl=%d)\n", 
           port, cylinders, sectors_per_cyl);
    
    // Main server loop - accept and handle clients
    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        
        int clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }
        
        // Handle client connection (blocking - one client at a time)
        handle_connection(clientfd);
    }
    
    // Cleanup (unreachable due to infinite loop, but included for completeness)
    close(listenfd);
    munmap(disk_data, total_blocks * BLOCK_SIZE);
    return EXIT_SUCCESS;
}
