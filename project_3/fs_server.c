/*
 * fs_server.c
 * Filesystem server implementing a FAT-based filesystem on top of disk storage.
 * 
 * This program demonstrates:
 * 1. Multi-threaded TCP server architecture with pthreads
 * 2. FAT (File Allocation Table) filesystem implementation
 * 3. Directory and file management operations
 * 4. Client-server protocol for filesystem operations
 * 5. Thread synchronization with mutexes
 * 6. Integration with underlying disk storage server
 * 7. Robust error handling for all system calls
 * 
 * Architecture:
 * - Tier 2 filesystem server built on top of Tier 1 disk server
 * - Maintains FAT in memory for efficient block allocation
 * - Provides hierarchical directory structure
 * - Supports concurrent client operations with proper synchronization
 * 
 * Protocol Commands:
 *   MK filename - Create new file
 *   RM filename - Remove file
 *   LS - List directory contents  
 *   READ filename offset length - Read file data
 *   WRITE filename offset data - Write file data
 *   MKDIR dirname - Create directory
 *   RMDIR dirname - Remove directory
 * 
 * Usage: ./fs_server <disk-ip> <disk-port> <fs-port>
 * Example: ./fs_server 127.0.0.1 8082 8083
 */

#include <stdio.h>      // Standard I/O functions (printf, perror, fprintf)
#include <stdlib.h>     // Memory allocation and conversion functions (atoi, EXIT_FAILURE)
#include <string.h>     // String manipulation (strlen, memset, strcpy, strcmp)
#include <unistd.h>     // UNIX system calls (close, recv, send)
#include <pthread.h>    // POSIX threads (pthread_mutex, pthread_create)
#include <arpa/inet.h>  // Internet address functions (inet_pton, htons)
#include <netinet/in.h> // Internet protocol address structures (sockaddr_in)
#include <sys/socket.h> // Socket API (socket, bind, listen, accept)
#include <errno.h>      // Error number definitions (used by perror)
#include "fs_defs.h"    // Filesystem data structures and constants

#define MAX_CLIENTS 20   // Maximum number of concurrent client connections
#define BUFFER_SIZE 4096 // Buffer size for network communication

#define LOG_ENABLED 1

#if LOG_ENABLED
#define LOGF(fmt, ...) \
    fprintf(stderr, "[fs_server][%d][%s] " fmt "\n", getpid(), __func__, ##__VA_ARGS__)
#else
#define LOGF(fmt, ...) ((void)0)
#endif

/*
 * ====================================================================
 * GLOBAL FILESYSTEM STATE
 * ====================================================================
 * 
 * These global variables represent the filesystem state that is shared
 * across all client threads. Proper synchronization is required to
 * maintain consistency in a multi-threaded environment.
 */

// Connection to underlying disk server (Tier 1)
int disk_sock = -1;                      // Socket descriptor for disk server connection

// Mutex for disk server operations
// Serializes all disk I/O to prevent protocol interleaving issues
// Only one thread can communicate with disk server at a time
pthread_mutex_t disk_lock = PTHREAD_MUTEX_INITIALIZER;

// Mutex for filesystem metadata operations
// Protects FAT and directory structures from concurrent modifications
// Ensures filesystem consistency during parallel operations
pthread_mutex_t fs_lock = PTHREAD_MUTEX_INITIALIZER;

// Disk geometry parameters (from disk server)
int cylinders;                           // Number of cylinders on underlying disk
int sectors_per_cyl;                     // Number of sectors per cylinder
int total_blocks;                        // Total number of blocks available

// Filesystem layout parameters
int fat_blocks_count;                    // Number of blocks used for FAT storage
int root_dir_block;                      // Block index where root directory starts

// File Allocation Table (in memory)
// Maps block indices to next block in file chain
// FAT_FREE (0) = free block, FAT_EOF (0xFFFF) = end of file
uint16_t *FAT = NULL;

/*
 * ====================================================================
 * DISK SERVER INTERFACE LAYER
 * ====================================================================
 * 
 * These functions provide an abstraction layer over the raw disk server.
 * They handle the protocol communication and convert logical block numbers
 * to physical cylinder/sector coordinates.
 */

/*
 * connect_to_disk - Establish connection to disk server and initialize filesystem
 * @ip: IP address of the disk server
 * @port: Port number of the disk server
 * 
 * This function:
 * 1. Creates TCP connection to disk server
 * 2. Queries disk geometry using 'I' command
 * 3. Calculates filesystem layout parameters
 * 4. Initializes global filesystem state
 * 
 * The function exits on failure since the filesystem cannot operate
 * without a working disk connection.
 */
void connect_to_disk(const char *ip, int port) {
    struct sockaddr_in serv_addr;         // Server address structure
    
    // Create TCP socket for disk server communication
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    if ((disk_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Disk socket creation error");
        exit(EXIT_FAILURE);
    }
    
    // Set up server address structure
    serv_addr.sin_family = AF_INET;       // IPv4 address family
    serv_addr.sin_port = htons(port);     // Convert port to network byte order
    
    // Convert IP address from text to binary form
    // inet_pton returns 1 on success, 0 on invalid input, -1 on error
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid disk address");
        exit(EXIT_FAILURE);
    }
    
    // Connect to the disk server
    // This blocks until the connection is established or fails
    if (connect(disk_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection to Disk Server failed");
        exit(EXIT_FAILURE);
    }
    
    // Query disk geometry to determine filesystem capacity
    // Send 'I' command to get cylinders and sectors per cylinder
    send(disk_sock, "I", 1, 0);
    
    // Receive geometry response from disk server
    char buf[64] = {0};                  // Buffer for geometry response
    recv(disk_sock, buf, 64, 0);
    
    // Parse geometry: "cylinders sectors_per_cylinder"
    if (sscanf(buf, "%d %d", &cylinders, &sectors_per_cyl) != 2) {
        fprintf(stderr, "Failed to parse geometry: %s\n", buf);
        exit(EXIT_FAILURE);
    }
    
    // Calculate total available blocks
    total_blocks = cylinders * sectors_per_cyl;
    
    // Display connection information
    printf("[FS] Connected to Disk: %d Cyls, %d Sec/Cyl, Total Blocks: %d\n",
           cylinders, sectors_per_cyl, total_blocks);
}

/*
 * disk_read - Read a logical block from the disk server
 * @block_idx: Logical block index to read (0 to total_blocks-1)
 * @buffer: Buffer to store the read block data (must be BLOCK_SIZE bytes)
 * 
 * This function:
 * 1. Converts logical block index to cylinder/sector coordinates
 * 2. Sends read command to disk server
 * 3. Receives status response
 * 4. Reads block data if successful, zeros buffer on error
 * 5. Uses disk_lock to ensure thread safety
 * 
 * Thread-safe due to disk_lock mutex protection.
 */
void disk_read(int block_idx, char *buffer) {
    // Acquire disk lock to prevent concurrent disk operations
    pthread_mutex_lock(&disk_lock);
    
    // Convert logical block index to physical coordinates
    int cylinder = block_idx / sectors_per_cyl;  // Which cylinder contains this block
    int sector = block_idx % sectors_per_cyl;    // Which sector within the cylinder
    
    // Format read command: "R cylinder sector"
    char cmd[64];
    sprintf(cmd, "R %d %d\n", cylinder, sector);
    
    // Send read command to disk server
    send(disk_sock, cmd, strlen(cmd), 0);
    
    // Receive status response from disk server
    char status;
    recv(disk_sock, &status, 1, 0);
    
    if (status == '1') {
        // Success: read the actual block data
        // TCP may deliver partial data, so we loop until complete
        int total_received = 0;
        while (total_received < BLOCK_SIZE) {
            int bytes = recv(disk_sock, buffer + total_received, 
                           BLOCK_SIZE - total_received, 0);
            if (bytes <= 0) break;          // Error or connection closed
            total_received += bytes;
        }
    } else {
        // Error: zero out the buffer
        memset(buffer, 0, BLOCK_SIZE);
    }
    
    // Release disk lock to allow other threads to use disk
    pthread_mutex_unlock(&disk_lock);
}

/*
 * disk_write - Write a logical block to the disk server
 * @block_idx: Logical block index to write (0 to total_blocks-1)
 * @data: Buffer containing data to write (must be BLOCK_SIZE bytes)
 * 
 * This function:
 * 1. Converts logical block index to cylinder/sector coordinates
 * 2. Sends write command to disk server
 * 3. Sends block data to disk server
 * 4. Receives confirmation status
 * 5. Uses disk_lock to ensure thread safety
 * 
 * Thread-safe due to disk_lock mutex protection.
 */
void disk_write(int block_idx, const char *data) {
    pthread_mutex_lock(&disk_lock);
    int c = block_idx / sectors_per_cyl;
    int s = block_idx % sectors_per_cyl;
    char cmd[64];
    sprintf(cmd, "W %d %d %d ", c, s, BLOCK_SIZE);
    send(disk_sock, cmd, strlen(cmd), 0);
    send(disk_sock, data, BLOCK_SIZE, 0);
    char status;
    recv(disk_sock, &status, 1, 0);
    pthread_mutex_unlock(&disk_lock);
}

// --- FS LOGIC ---

// Calculates how many blocks constitute the FAT based on total disk size.
// Loads the existing FAT from the disk into RAM for fast access.
void init_fs_meta(void) {
    int fat_size_bytes = total_blocks * sizeof(uint16_t);
    fat_blocks_count = (fat_size_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    root_dir_block = fat_blocks_count;
    
    FAT = malloc(fat_size_bytes);
    if (!FAT) exit(1);
    
    // Load FAT from disk
    char buf[BLOCK_SIZE];
    for (int i = 0; i < fat_blocks_count; i++) {
        disk_read(i, buf);
        int bytes_to_copy = BLOCK_SIZE;
        if ((i+1) * BLOCK_SIZE > fat_size_bytes)
            bytes_to_copy = fat_size_bytes - (i*BLOCK_SIZE);
        memcpy((char*)FAT + (i * BLOCK_SIZE), buf, bytes_to_copy);
    }
    printf("[FS] FAT loaded. Reserved: %d blocks\n", fat_blocks_count);
}

// Persists the in-memory FAT back to the physical disk.
// Must be called after any file allocation/deletion to ensure consistency.
void save_fat(void) {
    char buf[BLOCK_SIZE];
    int fat_size_bytes = total_blocks * sizeof(uint16_t);
    for (int i = 0; i < fat_blocks_count; i++) {
        memset(buf, 0, BLOCK_SIZE);
        int bytes_to_copy = BLOCK_SIZE;
        if ((i+1) * BLOCK_SIZE > fat_size_bytes)
            bytes_to_copy = fat_size_bytes - (i*BLOCK_SIZE);
        memcpy(buf, (char*)FAT + (i * BLOCK_SIZE), bytes_to_copy);
        disk_write(i, buf);
    }
}

// Linear search for a block marked FAT_FREE (0x0000).
// Returns the block index or -1 if disk is full.
int find_free_block(void) {
    for(int i = root_dir_block + 1; i < total_blocks; i++) {
        if(FAT[i] == FAT_FREE) return i;
    }
    return -1;
}

// --- COMMANDS ---

// Wipes the filesystem by resetting the FAT and clearing the Root Directory.
void cmd_format(int client_sock) {
    pthread_mutex_lock(&fs_lock);
    // 1. Reset FAT in memory
    memset(FAT, 0, total_blocks * sizeof(uint16_t));
    // 2. Mark FAT blocks themselves as reserved (EOF) so they aren't overwritten
    for (int i = 0; i <= root_dir_block; i++) FAT[i] = FAT_EOF;
    save_fat();
    // 3. Clear the root directory block
    char zeros[BLOCK_SIZE];
    memset(zeros, 0, BLOCK_SIZE);
    disk_write(root_dir_block, zeros);
    pthread_mutex_unlock(&fs_lock);
    send(client_sock, "0 Format Complete\n", 18, 0);
}

// Creates a File or Directory.
// 1. Checks if name exists in current directory.
// 2. Finds a free slot in the directory block.
// 3. Allocates a new block on disk for the file data (or new folder).
void cmd_create(int client_sock, char *name, int current_dir_block, int is_dir) {
    if (strlen(name) > MAX_FILENAME) {
        send(client_sock, "2 Name Too Long\n", 16, 0);
        return;
    }
    pthread_mutex_lock(&fs_lock);
    char buf[BLOCK_SIZE];
    disk_read(current_dir_block, buf);
    DirEntry *entries = (DirEntry*)buf;
    int max = BLOCK_SIZE / sizeof(DirEntry);
    int free_slot = -1;
    
    for(int i=0; i<max; i++) {
        if(entries[i].valid && strcmp(entries[i].name, name) == 0) {
            pthread_mutex_unlock(&fs_lock);
            send(client_sock, "1 Already Exists\n", 17, 0);
            return;
        }
        if(!entries[i].valid && free_slot == -1) free_slot = i;
    }
    
    if(free_slot == -1) {
        pthread_mutex_unlock(&fs_lock);
        send(client_sock, "2 Directory Full\n", 17, 0);
        return;
    }
    
    int new_block = find_free_block();
    if(new_block == -1) {
        pthread_mutex_unlock(&fs_lock);
        send(client_sock, "2 Disk Full\n", 12, 0);
        return;
    }
    FAT[new_block] = FAT_EOF;
    save_fat();
    
    char zeros[BLOCK_SIZE];
    memset(zeros, 0, BLOCK_SIZE);
    disk_write(new_block, zeros);
    
    strncpy(entries[free_slot].name, name, MAX_FILENAME);
    entries[free_slot].name[MAX_FILENAME] = '\0';
    entries[free_slot].size = 0;
    entries[free_slot].head_block = new_block;
    entries[free_slot].type = is_dir ? TYPE_DIR : TYPE_FILE;
    entries[free_slot].valid = 1;
    
    disk_write(current_dir_block, buf);
    pthread_mutex_unlock(&fs_lock);
    send(client_sock, "0 Created\n", 10, 0);
}

// Reads the current directory block and formats the list of entries.
void cmd_list(int client_sock, int current_dir_block, int detail) {
    pthread_mutex_lock(&fs_lock);
    char buf[BLOCK_SIZE];
    disk_read(current_dir_block, buf);
    pthread_mutex_unlock(&fs_lock);
    
    DirEntry *entries = (DirEntry*)buf;
    char resp[BUFFER_SIZE] = "";
    int count = 0;
    int max = BLOCK_SIZE / (int)sizeof(DirEntry);
    
    for(int i = 0; i < max; i++) {
        if(entries[i].valid) {
            char line[128];
            if (detail) {
                sprintf(line, "%s\t%d bytes\t%s\n",
                    entries[i].name, entries[i].size,
                    entries[i].type == TYPE_DIR ? "<DIR>" : "");
            } else {
                sprintf(line, "%s\n", entries[i].name);
            }
            strcat(resp, line);
            count++;
        }
    }
    if(count == 0) strcat(resp, "(Empty)\n");
    send(client_sock, resp, strlen(resp), 0);
}

// Overwrites a file.
// Strategy:
// 1. Locate file in directory.
// 2. Free the entire old chain of blocks (simple approach).
// 3. Allocate new blocks as needed while writing data.
// 4. Update file size in directory entry.
void cmd_write(int client_sock, char *name, int len, char *data, int current_dir_block) {
    pthread_mutex_lock(&fs_lock);
    char buf[BLOCK_SIZE];
    disk_read(current_dir_block, buf);
    DirEntry *entries = (DirEntry*)buf;
    int idx = -1;
    int max = BLOCK_SIZE / (int)sizeof(DirEntry);
    
    for(int i = 0; i < max; i++) {
        if(entries[i].valid && strcmp(entries[i].name, name) == 0) {
            if(entries[i].type == TYPE_DIR) {
                pthread_mutex_unlock(&fs_lock);
                send(client_sock, "2 Is Directory\n", 15, 0);
                return;
            }
            idx = i; break;
        }
    }
    
    if(idx == -1) {
        pthread_mutex_unlock(&fs_lock);
        send(client_sock, "1 Not Found\n", 12, 0);
        return;
    }
    
    // Simple implementation: Rewrite entire chain
    // Free old chain except head
    int curr = FAT[entries[idx].head_block];
    while(curr != FAT_EOF && curr != FAT_FREE) {
        int next = FAT[curr];
        FAT[curr] = FAT_FREE;
        curr = next;
    }
    FAT[entries[idx].head_block] = FAT_EOF;
    
    // Write new data
    curr = entries[idx].head_block;
    int written = 0;
    while(written < len) {
        char chunk[BLOCK_SIZE];
        memset(chunk, 0, BLOCK_SIZE);
        int to_write = (len - written > BLOCK_SIZE) ? BLOCK_SIZE : (len - written);
        memcpy(chunk, data + written, to_write);
        disk_write(curr, chunk);
        written += to_write;
        
        if(written < len) {
            int new_b = find_free_block();
            if(new_b == -1) break; // Out of space, truncate
            FAT[curr] = new_b;
            FAT[new_b] = FAT_EOF;
            curr = new_b;
        }
    }
    save_fat();
    entries[idx].size = written;
    disk_write(current_dir_block, buf);
    pthread_mutex_unlock(&fs_lock);
    send(client_sock, "0 Written\n", 10, 0);
}

// Traverses the linked list in the FAT to read all blocks of a file.
// Sends data back to client in chunks matching BLOCK_SIZE.
void cmd_read(int client_sock, char *name, int current_dir_block) {
    pthread_mutex_lock(&fs_lock);
    char buf[BLOCK_SIZE];
    disk_read(current_dir_block, buf);
    DirEntry *entries = (DirEntry*)buf;
    int idx = -1;
    int max = BLOCK_SIZE / (int)sizeof(DirEntry);
    
    for(int i = 0; i < max; i++) {
        if(entries[i].valid && strcmp(entries[i].name, name) == 0) {
            idx = i; break;
        }
    }
    
    if(idx == -1 || entries[idx].type == TYPE_DIR) {
        pthread_mutex_unlock(&fs_lock);
        send(client_sock, "1 Not Found\n", 12, 0);
        return;
    }
    
    char header[64];
    sprintf(header, "0 %d ", entries[idx].size);
    send(client_sock, header, strlen(header), 0);
    
    uint32_t size = entries[idx].size;
    int curr = entries[idx].head_block;
    uint32_t sent = 0;
    while(curr != FAT_EOF && sent < size) {
        char chunk[BLOCK_SIZE];
        disk_read(curr, chunk);
        uint32_t remaining = size - sent;
        int to_send = (remaining > (uint32_t)BLOCK_SIZE) ? BLOCK_SIZE : (int)remaining;
        send(client_sock, chunk, to_send, 0);
        sent += (uint32_t)to_send;
        curr = FAT[curr];
    }
    send(client_sock, "\n", 1, 0);
    pthread_mutex_unlock(&fs_lock);
}

// Deletes a file or directory.
// 1. Check if file/dir exists.
// 2. If directory, ensure it is empty.
// 3. Walk the FAT chain and mark blocks as FREE.
// 4. Mark directory entry as invalid.
void cmd_delete(int client_sock, char *name, int current_dir_block, int is_rmdir) {
    pthread_mutex_lock(&fs_lock);
    char buf[BLOCK_SIZE];
    disk_read(current_dir_block, buf);
    DirEntry *entries = (DirEntry*)buf;
    int idx = -1;
    int max = BLOCK_SIZE / (int)sizeof(DirEntry);
    
    for(int i = 0; i < max; i++) {
        if(entries[i].valid && strcmp(entries[i].name, name) == 0) {
            idx = i; break;
        }
    }
    
    if(idx == -1) {
        pthread_mutex_unlock(&fs_lock);
        send(client_sock, "1 Not Found\n", 12, 0);
        return;
    }
    
    if(is_rmdir) {
        if(entries[idx].type != TYPE_DIR) {
            pthread_mutex_unlock(&fs_lock);
            send(client_sock, "2 Not Directory\n", 16, 0);
            return;
        }
        // Check empty
        char sub[BLOCK_SIZE];
        disk_read(entries[idx].head_block, sub);
        DirEntry *sub_ent = (DirEntry*)sub;
        int max_sub = BLOCK_SIZE / (int)sizeof(DirEntry);
        for(int k = 0; k < max_sub; k++) {
            if(sub_ent[k].valid) {
                pthread_mutex_unlock(&fs_lock);
                send(client_sock, "2 Not Empty\n", 12, 0);
                return;
            }
        }
    } else {
        if(entries[idx].type == TYPE_DIR) {
            pthread_mutex_unlock(&fs_lock);
            send(client_sock, "2 Is Directory\n", 15, 0);
            return;
        }
    }
    
    // Free chain
    int curr = entries[idx].head_block;
    while(curr != FAT_EOF && curr != FAT_FREE) {
        int next = FAT[curr];
        FAT[curr] = FAT_FREE;
        curr = next;
    }
    save_fat();
    
    entries[idx].valid = 0;
    disk_write(current_dir_block, buf);
    pthread_mutex_unlock(&fs_lock);
    send(client_sock, "0 Deleted\n", 10, 0);
}

// --- THREAD ---

// Per-client session state.
// Stores the 'current working directory' (block index) and logical path string.
typedef struct {
    int sock;
    int current_dir;
    char pwd[256];
} Session;

// Worker thread logic.
// Parses ASCII commands and invokes appropriate FS functions.
void *handle_client(void *arg) {
    Session *s = (Session*)arg;
    char buf[BUFFER_SIZE];
    int n;
    
    s->current_dir = root_dir_block;
    strcpy(s->pwd, "/");
    
    while((n = recv(s->sock, buf, sizeof(buf)-1, 0)) > 0) {
        buf[n] = 0;
        char *nl = strchr(buf, '\n');
        if(nl) *nl = 0;
        if(strlen(buf) == 0) continue;
        
        char cmd[32], arg1[128], arg2[128];
        int count = sscanf(buf, "%s %s %s", cmd, arg1, arg2);
        
        if(strcmp(cmd, "F") == 0) {
            cmd_format(s->sock);
            s->current_dir = root_dir_block;
            strcpy(s->pwd, "/");
        }
        else if(strcmp(cmd, "L") == 0) cmd_list(s->sock, s->current_dir, (count > 1 && strcmp(arg1, "1") == 0));
        else if(strcmp(cmd, "C") == 0) cmd_create(s->sock, arg1, s->current_dir, 0);
        else if(strcmp(cmd, "mkdir") == 0) cmd_create(s->sock, arg1, s->current_dir, 1);
        else if(strcmp(cmd, "rmdir") == 0) cmd_delete(s->sock, arg1, s->current_dir, 1);
        else if(strcmp(cmd, "D") == 0) cmd_delete(s->sock, arg1, s->current_dir, 0);
        else if(strcmp(cmd, "R") == 0) cmd_read(s->sock, arg1, s->current_dir);
        else if(strcmp(cmd, "pwd") == 0) {
            char r[300]; sprintf(r, "%s\n", s->pwd);
            send(s->sock, r, strlen(r), 0);
        }
        else if(strcmp(cmd, "W") == 0) {
            // W file len data
            // Re-parse to handle spaces in data
            char *p1 = strchr(buf, ' ');
            if(!p1) { send(s->sock, "2 Bad Args\n", 11, 0); continue; }
            p1++; // Start of filename
            char *p2 = strchr(p1, ' ');
            if(!p2) { send(s->sock, "2 Bad Args\n", 11, 0); continue; }
            *p2 = 0; // Terminate filename
            char *filename = p1;
            p2++; // Start of len
            char *p3 = strchr(p2, ' ');
            if(!p3) { send(s->sock, "2 Bad Args\n", 11, 0); continue; }
            *p3 = 0;
            int len = atoi(p2);
            char *data = p3 + 1;
            cmd_write(s->sock, filename, len, data, s->current_dir);
        }
        else if(strcmp(cmd, "cd") == 0) {
            if (count < 2) {
                send(s->sock, "2 Missing Arg\n", 14, 0);
            } else if(strcmp(arg1, "/") == 0) {
                s->current_dir = root_dir_block;
                strcpy(s->pwd, "/");
                send(s->sock, "0\n", 2, 0);
            } else {
                pthread_mutex_lock(&fs_lock);
                char dbuf[BLOCK_SIZE];
                disk_read(s->current_dir, dbuf);
                DirEntry *entries = (DirEntry *)dbuf;
                int idx = -1;
                for (int i = 0; i < BLOCK_SIZE / (int)sizeof(DirEntry); i++) {
                    if (entries[i].valid &&
                        entries[i].type == TYPE_DIR &&
                        strcmp(entries[i].name, arg1) == 0) {
                        idx = i;
                        break;
                    }
                }

                if (idx == -1) {
                    pthread_mutex_unlock(&fs_lock);
                    send(s->sock, "1 Not Found\n", 12, 0);
                } else {
                    s->current_dir = entries[idx].head_block;
                    if (strcmp(s->pwd, "/") == 0) {
                        snprintf(s->pwd, sizeof(s->pwd), "/%s", entries[idx].name);
                    } else {
                        size_t plen = strlen(s->pwd);
                        size_t nlen = strlen(entries[idx].name);
                        if (plen + 1 + nlen >= sizeof(s->pwd)) {
                            pthread_mutex_unlock(&fs_lock);
                            send(s->sock, "2 Path Too Long\n", 16, 0);
                            continue;
                        }
                        strcat(s->pwd, "/");
                        strcat(s->pwd, entries[idx].name);
                    }
                    pthread_mutex_unlock(&fs_lock);
                    send(s->sock, "0\n", 2, 0);
                }
            }
        }
        else {
            send(s->sock, "Unknown command\n", 16, 0);
        }
    }

    close(s->sock);
    free(s);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <listen_port> <disk_ip> <disk_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int listen_port = atoi(argv[1]);
    const char *disk_ip = argv[2];
    int disk_port = atoi(argv[3]);
    
    // Validate filesystem server port range
    if (listen_port <= 0 || listen_port > 65535) {
        fprintf(stderr, "Error: Listen port must be between 1 and 65535\n");
        fprintf(stderr, "Usage: %s <listen_port> <disk_ip> <disk_port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Additional validation: ports below 1024 require root privileges
    if (listen_port < 1024) {
        fprintf(stderr, "Error: Ports below 1024 require root privileges\n");
        fprintf(stderr, "Please use a port between 1024 and 65535\n");
        return EXIT_FAILURE;
    }
    
    // Validate disk port range
    if (disk_port <= 0 || disk_port > 65535) {
        fprintf(stderr, "Error: Disk port must be between 1 and 65535\n");
        fprintf(stderr, "Usage: %s <listen_port> <disk_ip> <disk_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    connect_to_disk(disk_ip, disk_port);
    init_fs_meta();

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listenfd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(listen_port);

    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listenfd);
        return EXIT_FAILURE;
    }

    if (listen(listenfd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(listenfd);
        return EXIT_FAILURE;
    }

    printf("[FS] Server listening on port %d\n", listen_port);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int clientfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }

        Session *s = malloc(sizeof(Session));
        if (!s) {
            perror("malloc");
            close(clientfd);
            continue;
        }
        s->sock = clientfd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, s) != 0) {
            perror("pthread_create");
            close(clientfd);
            free(s);
            continue;
        }
        pthread_detach(tid);
    }

    close(listenfd);
    close(disk_sock);
    free(FAT);
    return EXIT_SUCCESS;
}

// Add a trailing blank line at EOF for -Wnewline-eof
