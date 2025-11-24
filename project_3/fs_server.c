#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include "fs_defs.h"

/*
 * CS4440 Project 3: File System Server
 * Implements Parts 4 (Files) and 5 (Directories).
 */

#define MAX_CLIENTS 20
#define BUFFER_SIZE 4096

// --- GLOBAL STATE ---

int disk_sock = -1;
pthread_mutex_t disk_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t fs_lock = PTHREAD_MUTEX_INITIALIZER;
int cylinders, sectors_per_cyl;
int total_blocks;
int fat_blocks_count;
int root_dir_block;
uint16_t *FAT = NULL;

// --- DISK INTERFACE ---

void connect_to_disk(const char *ip, int port) {
    struct sockaddr_in serv_addr;
    if ((disk_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Disk socket creation error");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid disk address");
        exit(EXIT_FAILURE);
    }
    if (connect(disk_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection to Disk Server failed");
        exit(EXIT_FAILURE);
    }
    
    // Get Geometry
    send(disk_sock, "I", 1, 0);
    char buf[64] = {0};
    recv(disk_sock, buf, 64, 0);
    
    if (sscanf(buf, "%d %d", &cylinders, &sectors_per_cyl) != 2) {
        fprintf(stderr, "Failed to parse geometry: %s\n", buf);
        exit(EXIT_FAILURE);
    }
    total_blocks = cylinders * sectors_per_cyl;
    printf("[FS] Connected to Disk: %d Cyls, %d Sec/Cyl, Total Blocks: %d\n",
           cylinders, sectors_per_cyl, total_blocks);
}

void disk_read(int block_idx, char *buffer) {
    pthread_mutex_lock(&disk_lock);
    int c = block_idx / sectors_per_cyl;
    int s = block_idx % sectors_per_cyl;
    char cmd[64];
    sprintf(cmd, "R %d %d\n", c, s);
    send(disk_sock, cmd, strlen(cmd), 0);
    
    char status;
    recv(disk_sock, &status, 1, 0);
    if (status == '1') {
        int total = 0;
        while(total < BLOCK_SIZE) {
             int n = recv(disk_sock, buffer + total, BLOCK_SIZE - total, 0);
             if (n <= 0) break;
             total += n;
        }
    } else {
        memset(buffer, 0, BLOCK_SIZE);
    }
    pthread_mutex_unlock(&disk_lock);
}

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

int find_free_block(void) {
    for(int i = root_dir_block + 1; i < total_blocks; i++) {
        if(FAT[i] == FAT_FREE) return i;
    }
    return -1;
}

// --- COMMANDS ---

void cmd_format(int client_sock) {
    pthread_mutex_lock(&fs_lock);
    memset(FAT, 0, total_blocks * sizeof(uint16_t));
    for (int i = 0; i <= root_dir_block; i++) FAT[i] = FAT_EOF;
    save_fat();
    char zeros[BLOCK_SIZE];
    memset(zeros, 0, BLOCK_SIZE);
    disk_write(root_dir_block, zeros);
    pthread_mutex_unlock(&fs_lock);
    send(client_sock, "0 Format Complete\n", 18, 0);
}

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

typedef struct {
    int sock;
    int current_dir;
    char pwd[256];
} Session;

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