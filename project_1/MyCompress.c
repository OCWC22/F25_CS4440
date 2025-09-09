// project_1/MyCompress.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

void compress(FILE *source, FILE *dest) {
    int count = 0;
    char current_char = EOF;
    char prev_char = EOF;

    while ((current_char = fgetc(source)) != EOF) {
        if (current_char == ' ' || current_char == '\n') {
            if (count > 0) {
                if (count >= 16) {
                    fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
                } else {
                    for (int i = 0; i < count; i++) fputc(prev_char, dest);
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
            if (count >= 16) {
                fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
            } else {
                for (int i = 0; i < count; i++) fputc(prev_char, dest);
            }
            prev_char = current_char;
            count = 1;
        }
    }

    if (count > 0) {
        if (count >= 16) {
            fprintf(dest, "%c%d%c", (prev_char == '1' ? '+' : '-'), count, (prev_char == '1' ? '+' : '-'));
        } else {
            for (int i = 0; i < count; i++) fputc(prev_char, dest);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <destination_file>\n", argv[0]);
        return 1;
    }

    FILE *source = fopen(argv[1], "r");
    if (source == NULL) {
        perror("Error opening source file");
        return 1;
    }

    // Ensure outputs/ directory exists
    if (mkdir("outputs", 0755) < 0 && errno != EEXIST) {
        perror("mkdir outputs");
        fclose(source);
        return 1;
    }

    // Build outputs/<basename(destination)>
    const char *dest_arg = argv[2];
    const char *base = strrchr(dest_arg, '/');
    base = (base != NULL) ? base + 1 : dest_arg;
    char out_path[PATH_MAX];
    if (snprintf(out_path, sizeof(out_path), "outputs/%s", base) >= (int)sizeof(out_path)) {
        fprintf(stderr, "Destination path too long.\n");
        fclose(source);
        return 1;
    }

    FILE *dest = fopen(out_path, "w");
    if (dest == NULL) {
        perror("Error opening destination file");
        fclose(source);
        return 1;
    }

    compress(source, dest);
    printf("File compressed successfully -> %s\n", out_path);
    fclose(source);
    fclose(dest);
    return 0;
}
