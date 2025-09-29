// MyCompress.c  -> build:  gcc -O2 -Wall -Wextra -std=c11 Problem1.c -o Problem1
// usage: ./MyCompress <source_file> <dest_file>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SZ 65536

static void die(const char *msg) {
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
    _exit(1);
}
    
static void write_all(int fd, const void *buf, size_t n) {
    const char *p = (const char*)buf;
    while (n) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            die("write() failed");
        }
        p += (size_t)w;
        n -= (size_t)w;
    }
}

static void write_uint_dec(int fd, unsigned long long x) {
    char tmp[32];
    int i = 0;
    if (x == 0) tmp[i++] = '0';
    else {
        while (x) {
            tmp[i++] = (char)('0' + (x % 10));
            x /= 10ULL;
        }
    }
    // reverse to write in correct order
    for (int j = i - 1; j >= 0; --j) write_all(fd, &tmp[j], 1);
}

static void write_repeat(int fd, char c, unsigned long long n) {
    char block[1024];
    memset(block, c, sizeof(block));
    while (n) {
        size_t chunk = n > sizeof(block) ? sizeof(block) : (size_t)n;
        write_all(fd, block, chunk);
        n -= chunk;
    }
}

static void flush_run(int outfd, char bit, unsigned long long len) {
    if (len == 0) return;
    if (len >= 16ULL) {
        char sign = (bit == '1') ? '+' : '-';
        write_all(outfd, &sign, 1);
        write_uint_dec(outfd, len);
        write_all(outfd, &sign, 1);
    } else {
        write_repeat(outfd, bit, len);
    }
}

static void compress_stream(int infd, int outfd) {
    char buf[BUF_SZ];
    char cur = 0;                       // current run bit ('0' or '1'), 0 = none
    unsigned long long run = 0;         // current run length

    for (;;) {
        ssize_t r = read(infd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR) continue;
            die("read() failed");
        }
        if (r == 0) break;

        for (ssize_t i = 0; i < r; ++i) {
            char ch = buf[i];

            if (ch == '0' || ch == '1') {
                if (run == 0) { cur = ch; run = 1; }
                else if (ch == cur) { ++run; }
                else { flush_run(outfd, cur, run); cur = ch; run = 1; }
            } else if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
                flush_run(outfd, cur, run);
                cur = 0; run = 0;
                write_all(outfd, &ch, 1);    
            } else {
                flush_run(outfd, cur, run);
                cur = 0; run = 0;
                write_all(outfd, &ch, 1);
            }
        }
    }

    flush_run(outfd, cur, run);
}

int main(int argc, char **argv) {
    if (argc != 3) die("usage: MyCompress <source_file> <dest_file>");

    int infd = open(argv[1], O_RDONLY);
    if (infd < 0) die("open(source) failed");

    int outfd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (outfd < 0) die("open(dest) failed");

    compress_stream(infd, outfd);

    if (close(infd) < 0) die("close(source) failed");
    if (close(outfd) < 0) die("close(dest) failed");
    return 0;
}
