#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256

void compress_and_write(int read_fd, FILE *dest) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    char prev_char = EOF;
    int count = 0;

    while ((bytes_read = read(read_fd, buffer, BUFFER_SIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            char current_char = buffer[i];
            
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
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <destination_file>\n", argv[0]);
        return 1;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) { // Child process: The Writer/Compressor
        close(pipe_fd[1]); // Close the write-end of the pipe

        FILE *dest = fopen(argv[2], "w");
        if (!dest) {
            perror("Child failed to open destination file");
            exit(1);
        }
        
        compress_and_write(pipe_fd[0], dest);
        
        fclose(dest);
        close(pipe_fd[0]);
        exit(0);
    } else { // Parent process: The Reader
        close(pipe_fd[0]); // Close the read-end of the pipe

        FILE *source = fopen(argv[1], "r");
        if (!source) {
            perror("Parent failed to open source file");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source)) > 0) {
            write(pipe_fd[1], buffer, bytes_read);
        }

        fclose(source);
        close(pipe_fd[1]); // Signal EOF to the child
        
        wait(NULL); // Wait for child to finish
        printf("Pipe compression complete.\n");
    }

    return 0;
} 
