#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>

void compress_chunk(const char* in_file, const char* out_file, long start, long size) {
    FILE *source = fopen(in_file, "r");
    if (!source) {
        perror("Failed to open input file");
        exit(1);
    }
    
    fseek(source, start, SEEK_SET);

    FILE *dest = fopen(out_file, "w");
    if (!dest) {
        perror("Failed to open output file");
        fclose(source);
        exit(1);
    }

    char prev_char = EOF;
    int count = 0;
    long bytes_read = 0;

    while (bytes_read < size) {
        char current_char = fgetc(source);
        if (current_char == EOF) break;
        bytes_read++;
        
        if (current_char == ' ' || current_char == '\n') {
            if (count > 0) {
                if (count >= 16) {
                    fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                } else {
                    for (int j = 0; j < count; j++) {
                        fputc(prev_char, dest);
                    }
                }
            }
            fputc(current_char, dest);
            count = 0;
            prev_char = EOF;
            continue;
        }
        
        if (prev_char == EOF) {
            prev_char = current_char;
            count = 1;
        } else if (current_char == prev_char) {
            count++;
        } else {
            if (count > 0) {
                if (count >= 16) {
                    fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                } else {
                    for (int j = 0; j < count; j++) {
                        fputc(prev_char, dest);
                    }
                }
            }
            prev_char = current_char;
            count = 1;
        }
    }
    
    if (count > 0) {
        if (count >= 16) {
            fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
        } else {
            for (int j = 0; j < count; j++) {
                fputc(prev_char, dest);
            }
        }
    }

    fclose(source);
    fclose(dest);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num_processes> <source_file> <dest_file>\n", argv[0]);
        return 1;
    }

    int n_processes = atoi(argv[1]);
    if (n_processes <= 0) {
        fprintf(stderr, "Number of processes must be positive\n");
        return 1;
    }

    const char* source_file = argv[2];
    const char* dest_file = argv[3];

    struct stat st;
    if (stat(source_file, &st) != 0) {
        perror("Failed to get file size");
        return 1;
    }
    
    long file_size = st.st_size;
    long chunk_size = file_size / n_processes;

    pid_t pids[n_processes];

    for (int i = 0; i < n_processes; i++) {
        long start = i * chunk_size;
        long size = (i == n_processes - 1) ? (file_size - start) : chunk_size;
        
        char temp_out_file[256];
        snprintf(temp_out_file, sizeof(temp_out_file), "temp_out_%d.txt", i);

        pids[i] = fork();
        
        if (pids[i] == 0) { // Child process
            compress_chunk(source_file, temp_out_file, start, size);
            exit(0);
        } else if (pids[i] < 0) {
            perror("fork failed");
            exit(1);
        }
    }

    // Parent process waits for all children and assembles the final file
    for (int i = 0; i < n_processes; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Child process %d failed\n", i);
        }
    }

    FILE *final_dest = fopen(dest_file, "w");
    if (!final_dest) {
        perror("Failed to open final destination file");
        return 1;
    }
    
    for (int i = 0; i < n_processes; i++) {
        char temp_out_file[256];
        snprintf(temp_out_file, sizeof(temp_out_file), "temp_out_%d.txt", i);
        
        FILE *temp_source = fopen(temp_out_file, "r");
        if (!temp_source) {
            perror("Failed to open temporary file");
            continue;
        }
        
        char ch;
        while ((ch = fgetc(temp_source)) != EOF) {
            fputc(ch, final_dest);
        }
        
        fclose(temp_source);
        remove(temp_out_file); // Clean up temporary file
    }
    
    fclose(final_dest);

    printf("Parallel compression complete with %d processes.\n", n_processes);
    return 0;
}