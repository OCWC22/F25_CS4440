/**
 * CS4440 Project 1 - Task 2: Basic Decompression
 *
 * PURPOSE:
 * This program decompresses files created by the MyCompress program.
 * It reads a compressed file containing run-length encoded binary data
 * in the format +count+ for '1's and -count- for '0's, expands these
 * compressed sequences back to their original form, and writes the
 * decompressed output to a destination file.
 *
 * ALGORITHM:
 * - Reads input file character by character
 * - When encountering '+' or '-' characters:
 *   - Parses the following number to get the count
 *   - Expects another '+' or '-' to close the compression marker
 *   - Expands the compressed sequence by writing count instances of '1' or '0'
 * - Regular characters are passed through unchanged
 * - Handles malformed input gracefully by treating it as literal text
 *
 * USAGE:
 * ./MyDecompress <compressed_file> <output_file>
 *
 * EXAMPLE:
 * Input: "hello world\n+16+\n"
 * Output: "hello world\n1111111111111111\n"
 *
 * Author: CS4440 Student
 * Build: gcc -O2 -Wall -Wextra -std=c11 MyDecompress.c -o MyDecompress
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Writes n repetitions of character ch to file descriptor out
 * @param out Output file descriptor
 * @param ch Character to repeat
 * @param n Number of times to repeat the character
 */
static void write_repeated(int out, char ch, long n){
    while(n-->0) write(out,&ch,1);
}

/**
 * Main decompression function that processes the input stream
 * Reads compressed data and expands run-length encoded sequences
 * @param in Input file descriptor
 * @param out Output file descriptor
 */
static void decompress_fd(int in, int out){
    unsigned char c;
    while(read(in,&c,1)==1){
        if(c=='+' || c=='-'){
            // Found compression marker - parse and expand
            char sign = c; long num = 0; unsigned char d;
            while(read(in,&d,1)==1 && d>='0' && d<='9') num = num*10 + (d-'0');
            if(d==sign && num>0){
                // Valid compression marker - expand the sequence
                write_repeated(out, sign=='+'?'1':'0', num);
            }
            else{
                // Malformed compression marker - treat as literal text
                write(out,&sign,1);
                if(num){ char buf[32]; int m=snprintf(buf,sizeof(buf), "%ld", num); write(out,buf,m); }
                if(d) write(out,&d,1);
            }
        }else{
            // Regular character - pass through unchanged
            write(out,&c,1);
        }
    }
}

/**
 * Main function - entry point for the decompression program
 * Validates arguments, opens files, performs decompression, and cleans up
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 on success, 1 on error
 */
int main(int argc, char *argv[]){
    if(argc!=3){ dprintf(2,"usage: %s compressed.txt out.txt\n",argv[0]); return 1; }
    int fdin = open(argv[1], O_RDONLY);
    if(fdin<0){ perror("open in"); return 1; }
    int fdout = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0644);
    if(fdout<0){ perror("open out"); close(fdin); return 1; }
    decompress_fd(fdin, fdout);
    close(fdin); close(fdout);
    return 0;
}
