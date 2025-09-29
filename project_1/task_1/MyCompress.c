/**
 * CS4440 Project 1 - Task 1: Basic Sequential Compression
 *
 * PURPOSE:
 * This program implements run-length encoding (RLE) compression for binary data.
 * It reads from a source file containing binary data (0s and 1s) mixed with text,
 * compresses sequences of 16 or more consecutive identical bits using the format
 * +count+ for '1's and -count- for '0's, and writes the compressed output to
 * a destination file.
 *
 * ALGORITHM:
 * - Reads input file character by character
 * - Maintains a running count of consecutive identical bits (0s or 1s)
 * - When a different bit is encountered or a non-binary character is found:
 *   - If the count >= 16, outputs compressed format (+count+ or -count-)
 *   - Otherwise, outputs the bits individually
 * - Non-binary characters (spaces, newlines, etc.) are passed through unchanged
 * - Handles large files efficiently using buffered I/O
 *
 * USAGE:
 * ./MyCompress <source_file> <dest_file>
 *
 * EXAMPLE:
 * Input: "hello world\n1111111111111111\n"
 * Output: "hello world\n+16+\n"
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 MyCompress.c -o MyCompress
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SZ 65536

/**
 * Prints error message to stderr and exits with status 1
 * @param msg Error message to display
 */
static void die(const char *msg) {
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
    _exit(1);
}

/**
 * Writes exactly n bytes from buf to file descriptor fd
 * Handles partial writes and EINTR interruptions
 * @param fd File descriptor to write to
 * @param buf Buffer containing data to write
 * @param n Number of bytes to write
 */
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

/**
 * Writes an unsigned long long integer as decimal digits to file descriptor
 * @param fd File descriptor to write to
 * @param x Number to write
 */
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

/**
 * Writes n repetitions of character c to file descriptor fd efficiently
 * Uses block writes for better performance on large counts
 * @param fd File descriptor to write to
 * @param c Character to repeat
 * @param n Number of times to repeat the character
 */
static void write_repeat(int fd, char c, unsigned long long n) {
    char block[1024];
    memset(block, c, sizeof(block));
    while (n) {
        size_t chunk = n > sizeof(block) ? sizeof(block) : (size_t)n;
        write_all(fd, block, chunk);
        n -= chunk;
    }
}

/**
 * Outputs a compressed or uncompressed run of bits to the output file
 * If run length >= 16, uses compressed format (+count+ or -count-)
 * Otherwise, writes the bits individually
 * @param outfd Output file descriptor
 * @param bit The bit character ('0' or '1')
 * @param len Number of consecutive bits
 */
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

/**
 * Main compression function that processes the input stream
 * Reads data in chunks, processes each character, and maintains compression state
 * @param infd Input file descriptor
 * @param outfd Output file descriptor
 */
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
                // Binary digit - continue or start new run
                if (run == 0) { cur = ch; run = 1; }
                else if (ch == cur) { ++run; }
                else { flush_run(outfd, cur, run); cur = ch; run = 1; }
            } else if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
                // Whitespace - flush current run and output character
                flush_run(outfd, cur, run);
                cur = 0; run = 0;
                write_all(outfd, &ch, 1);    
            } else {
                // Other character - flush current run and output character
                flush_run(outfd, cur, run);
                cur = 0; run = 0;
                write_all(outfd, &ch, 1);
            }
        }
    }

    flush_run(outfd, cur, run);
}

/**
 * Main function - entry point for the compression program
 * Validates arguments, opens files, performs compression, and cleans up
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 on success, exits with error code on failure
 */
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
