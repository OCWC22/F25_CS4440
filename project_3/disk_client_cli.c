/*
 * disk_client_cli.c
 * Interactive command-line client for the disk storage server.
 * 
 * This program demonstrates:
 * 1. TCP socket programming (client side)
 * 2. Interactive command-line interface (REPL pattern)
 * 3. Protocol communication with custom disk server
 * 4. Command parsing and validation
 * 5. Binary data handling for disk operations
 * 6. Robust error handling for all system calls
 * 
 * Supported Commands:
 *   I - Get disk geometry information
 *   R c s - Read block at cylinder c, sector s
 *   W c s data - Write data to block at cylinder c, sector s
 *   q - Quit the client
 * 
 * Usage: ./disk_client_cli <server-ip> <port>
 * Example: ./disk_client_cli 127.0.0.1 8082
 */

#define _POSIX_C_SOURCE 200809L         // Enable POSIX 2008 features

#include <stdio.h>      // Standard I/O functions (printf, perror, fprintf, fgets, fflush)
#include <stdlib.h>     // Memory allocation and conversion functions (atoi, EXIT_FAILURE)
#include <string.h>     // String manipulation (strlen, memset, strcmp, sscanf)
#include <unistd.h>     // UNIX system calls (close, recv, send)
#include <errno.h>      // Error number definitions (used by perror, EINTR)
#include <ctype.h>      // Character classification functions (isspace)
#include <arpa/inet.h>  // Internet address functions (inet_pton, htons)
#include <netinet/in.h> // Internet protocol address structures (sockaddr_in)
#include <sys/socket.h> // Socket API (socket, connect)

#define BLOCK_SIZE 128   // Fixed block size for disk operations (matches server)
#define MAX_LINE   1024  // Maximum length of user input line

#define LOG_ENABLED 1

#if LOG_ENABLED
#define LOGF(fmt, ...) \
    fprintf(stderr, "[disk_client_cli][%d][%s] " fmt "\n", getpid(), __func__, ##__VA_ARGS__)
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
 * the exact 128-byte blocks from the disk server.
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
 * main - Entry point for the interactive disk client
 * @argc: Number of command line arguments
 * @argv: Array of command line argument strings
 * 
 * Connects to disk server and provides interactive REPL for disk operations.
 * Implements robust error handling for all system calls as required.
 * 
 * Returns: EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char *argv[]) {
    LOGF("main: argc=%d", argc);
    // STEP 1: Validate command line arguments
    // We need exactly: program_name server_ip port
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Extract server IP and port from command line
    const char *server_ip = argv[1];  // Server IP address (e.g., "127.0.0.1")
    int port = atoi(argv[2]);         // Server port number (e.g., 8082)
    LOGF("server_ip=%s port=%d", server_ip, port);
    
    // Validate port range
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Error: Port must be between 1 and 65535\n");
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
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
    
    // Connect to the disk server
    // This blocks until the connection is established or fails
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);                  // Clean up socket before exit
        return EXIT_FAILURE;
    }
    
    // STEP 4: Start interactive REPL (Read-Eval-Print Loop)
    char line[MAX_LINE];                // Buffer for user input
    
    // Check if stdin is a terminal (interactive) or pipe/file
    int is_interactive = isatty(STDIN_FILENO);
    
    // Display available commands (only once for interactive sessions)
    if (is_interactive) {
        printf("Connected to disk server. Commands: I | R c s | W c s data | q\n");
    }
    
    // Main command loop - continues until user quits
    while (1) {
        // Display prompt only if interactive
        if (is_interactive) {
            printf("> ");                   // Display prompt
            fflush(stdout);                 // Ensure prompt is displayed immediately
        }
        
        // Read a line of input from the user
        // fgets returns NULL on EOF or error
        if (!fgets(line, sizeof(line), stdin)) {
            break;                      // Exit on EOF (Ctrl+D)
        }
        
        // Remove trailing newline character
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Skip empty lines
        if (line[0] == '\0') {
            continue;
        }
        
        // Variables for parsing commands
        char cmd[4];                    // Buffer for command token (I, R, W, q)
        int c = 0, s = 0;               // Cylinder and sector variables
        
        // Extract the first token (command) from the input line
        // %3s reads up to 3 characters to prevent buffer overflow
        if (sscanf(line, "%3s", cmd) != 1) {
            continue;                   // Skip if no command found
        }
        
        // --- COMMAND: INFO (Get disk geometry) ---
        if (strcmp(cmd, "I") == 0 || strcmp(cmd, "i") == 0) {
            LOGF("command I (info)");
            // Send "I" command to server to get disk geometry
            const char *info_cmd = "I\n";
            if (send(sockfd, info_cmd, strlen(info_cmd), 0) != (ssize_t)strlen(info_cmd)) {
                perror("send");
                break;
            }
            
            // Receive and display server response
            char response[64];
            ssize_t n = recv(sockfd, response, sizeof(response) - 1, 0);
            
            if (n <= 0) {
                perror("recv");
                break;
            }
            
            response[n] = '\0';          // Null-terminate the response
            
            printf("> Disk geometry: %s", response);
            continue;
        }
        
        // --- COMMAND: READ (Read block from disk) ---
        if (strcmp(cmd, "R") == 0 || strcmp(cmd, "r") == 0) {
            // Parse cylinder and sector numbers from input
            if (sscanf(line, "%*s %d %d", &c, &s) != 2) {
                printf("> Usage: R <cylinder> <sector>\n");
                continue;
            }
            
            LOGF("command R: c=%d s=%d", c, s);
            // Send read command to server: "R c s"
            char cmd_str[64];
            int cmd_len = snprintf(cmd_str, sizeof(cmd_str), "R %d %d\n", c, s);
            if (send(sockfd, cmd_str, cmd_len, 0) != cmd_len) {
                perror("send");
                break;
            }
            
            // Receive server response
            char response[256];
            ssize_t n = recv(sockfd, response, sizeof(response) - 1, 0);
            if (n <= 0) {
                perror("recv");
                break;
            }
            
            response[n] = '\0';          // Null-terminate the response
            printf("> %s", response);
            
            // If response indicates success, receive the 128-byte block data
            if (response[0] == '0') {
                unsigned char block[BLOCK_SIZE];
                ssize_t bytes = recv_all(sockfd, block, BLOCK_SIZE);
                if (bytes < 0) {
                    perror("recv_all");
                    break;
                }
                if (bytes != BLOCK_SIZE) {
                    fprintf(stderr,
                            "Error: expected %d bytes but received %zd bytes from server.\n",
                            BLOCK_SIZE, bytes);
                    break;
                }
                
                // Display block data as printable characters with dots for non-printable
                for (int i = 0; i < BLOCK_SIZE; i++) {
                    if (block[i] >= 32 && block[i] <= 126) {
                        putchar(block[i]); // Printable character
                    } else {
                        putchar('.');     // Non-printable character shown as dot
                    }
                }
                putchar('\n');
            }
            continue;
        }
        
        // --- COMMAND: WRITE (Write block to disk) ---
        if (strcmp(cmd, "W") == 0 || strcmp(cmd, "w") == 0) {
            // Parse cylinder, sector, and find start of data
            char *data_start = strchr(line + 2, ' '); // Find first space after "W"
            if (!data_start) {
                printf("> Usage: W <cylinder> <sector> <data>\n");
                continue;
            }
            
            data_start++;                 // Move past the space
            char *sector_start = strchr(data_start, ' ');
            if (!sector_start) {
                printf("> Usage: W <cylinder> <sector> <data>\n");
                continue;
            }
            
            *sector_start = '\0';         // Temporarily null-terminate cylinder string
            c = atoi(data_start);         // Parse cylinder
            s = atoi(sector_start + 1);   // Parse sector
            
            char *write_data = sector_start + 1;
            // Skip to actual data (after sector number)
            while (*write_data && *write_data != ' ') write_data++;
            if (*write_data) write_data++; // Skip the space
            
            int data_len = strlen(write_data);
            LOGF("command W: c=%d s=%d len=%d", c, s, data_len);
            
            // Send write command to server: "W c s len"
            char cmd_str[64];
            int cmd_len = snprintf(cmd_str, sizeof(cmd_str), "W %d %d %d\n", c, s, data_len);
            if (send(sockfd, cmd_str, cmd_len, 0) != cmd_len) {
                perror("send");
                break;
            }
            
            // Send the actual data
            if (data_len > 0) {
                if (send(sockfd, write_data, data_len, 0) != data_len) {
                    perror("send data");
                    break;
                }
            }
            
            // Receive server response
            char response[64];
            ssize_t n = recv(sockfd, response, sizeof(response) - 1, 0);
            if (n <= 0) {
                perror("recv");
                break;
            }
            
            response[n] = '\0';          // Null-terminate the response
            printf("> %s", response);
            continue;
        }
        
        // --- COMMAND: QUIT (Exit client) ---
        if (strcmp(cmd, "q") == 0 || strcmp(cmd, "Q") == 0) {
            LOGF("command q (quit)");
            break;                      // Exit the loop
        }
        
        // --- UNKNOWN COMMAND ---
        printf("> Unknown command. Use: I, R c s, W c s data, or q\n");
    }
    
    // Clean up and exit
    close(sockfd);
    return EXIT_SUCCESS;
}
