// project_1/MyDecompress.c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

void decompress(FILE *source, FILE *dest) {
    char ch;
    while ((ch = fgetc(source)) != EOF) {
        if (ch == '+' || ch == '-') {
            int count = 0;
            char bit_to_write = (ch == '+') ? '1' : '0';
            fscanf(source, "%d", &count);
            fgetc(source); // Consume the closing delimiter

            for (int i = 0; i < count; i++) {
                fputc(bit_to_write, dest);
            }
        } else {
            fputc(ch, dest);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <compressed_file> <destination_file>\n", argv[0]);
        return 1;
    }

    FILE *source = fopen(argv[1], "r");
    if (source == NULL) {
        perror("Error opening compressed file");
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

    decompress(source, dest);
    printf("File decompressed successfully -> %s\n", out_path);
    fclose(source);
    fclose(dest);
    return 0;
}
