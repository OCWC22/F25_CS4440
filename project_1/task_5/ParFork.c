#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void comp_buf(char *s, long len, FILE *w) {
    char prev = 0;
    long count = 0;
    for (long i = 0; i < len; i++) {
        char curr = s[i];
        if (curr == '0' || curr == '1') {
            if (count == 0) {
                prev = curr;
                count = 1;
            } else if (curr == prev) {
                count++;
            } else {
                if (count >= 16) {
                    fprintf(w, "%c%ld%c", prev == '1' ? '+' : '-', count, prev == '1' ? '+' : '-');
                } else {
                    for (long j = 0; j < count; j++) {
                        fputc(prev, w);
                    }
                }
                prev = curr;
                count = 1;
            }
        } else {
            if (count >= 16) {
                fprintf(w, "%c%ld%c", prev == '1' ? '+' : '-', count, prev == '1' ? '+' : '-');
            } else {
                for (long j = 0; j < count; j++) {
                    fputc(prev, w);
                }
            }
            fputc(curr, w);
            prev = 0;
            count = 0;
        }
    }
    if (count >= 16) {
        fprintf(w, "%c%ld%c", prev == '1' ? '+' : '-', count, prev == '1' ? '+' : '-');
    } else {
        for (long j = 0; j < count; j++) {
            fputc(prev, w);
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s src dst nproc\n", argv[0]);
        return 1;
    }

    FILE* in = fopen(argv[1], "r");
    if (!in) {
        perror(argv[1]);
        return 1;
    }
    fseek(in, 0, SEEK_END);
    long N = ftell(in);
    fseek(in, 0, SEEK_SET);
    char *buf = malloc(N);
    if (!buf) {
        perror("malloc");
        return 1;
    }
    fread(buf, 1, N, in);
    fclose(in);

    int n = atoi(argv[3]);
    if (n < 1) n = 1;
    long chunk = (N + n - 1) / n;
    int (*pp)[2] = malloc(sizeof(int[2]) * n);
    pid_t *pids = malloc(sizeof(pid_t) * n);

    for (int i = 0; i < n; i++) {
        pipe(pp[i]);
        if ((pids[i] = fork()) == 0) {
            close(pp[i][0]);
            FILE* w = fdopen(pp[i][1], "w");
            long s = (long)i * chunk, e = s + chunk;
            if (e > N) e = N;
            if (s < N) comp_buf(buf + s, e - s, w);
            fclose(w);
            _exit(0);
        }
        close(pp[i][1]);
    }

    FILE* out = fopen(argv[2], "w");
    for (int i = 0; i < n; i++) {
        FILE* r = fdopen(pp[i][0], "r");
        int c;
        while ((c = fgetc(r)) != EOF) fputc(c, out);
        fclose(r);
        waitpid(pids[i], NULL, 0);
    }
    fclose(out);
    free(buf);
    free(pp);
    free(pids);
    return 0;
}
