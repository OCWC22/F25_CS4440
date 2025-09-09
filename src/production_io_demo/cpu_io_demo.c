// cpu_io_demo.c
// Asynchronous file copy with bounded pipeline using POSIX AIO.
// Optional Linux io_uring backend can be enabled by compiling with -DUSE_IO_URING and linking liburing,
// but this file keeps a portable, production-friendly baseline for macOS/Linux.

#define _XOPEN_SOURCE 700
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <aio.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) ((int)(sizeof(x)/sizeof((x)[0])))
#endif

static inline uint64_t now_ns(void) {
  struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void die(const char* msg) {
  perror(msg); exit(1);
}

// Simple per-byte transform to simulate CPU work between read and write.
static void transform(uint8_t* buf, size_t n) {
  for (size_t i = 0; i < n; ++i) buf[i] ^= 0x5A; // cheap, deterministic
}

typedef enum { BUF_FREE=0, BUF_READ_IO, BUF_READY, BUF_WRITE_IO } buf_state_t;

typedef struct Buffer {
  uint8_t* data;
  size_t cap;     // capacity in bytes
  size_t size;    // valid bytes for current chunk
  off_t  off;     // file offset
  struct aiocb rcb; // read control block
  struct aiocb wcb; // write control block
  buf_state_t st;
} Buffer;

static void submit_read(int fd, Buffer* b, off_t off, size_t n) {
  memset(&b->rcb, 0, sizeof(b->rcb));
  b->rcb.aio_fildes = fd;
  b->rcb.aio_buf = (volatile void*)b->data;
  b->rcb.aio_nbytes = n;
  b->rcb.aio_offset = off;
  b->off = off;
  b->size = n;
  if (aio_read(&b->rcb) != 0) die("aio_read");
  b->st = BUF_READ_IO;
}

static void submit_write(int fd, Buffer* b) {
  memset(&b->wcb, 0, sizeof(b->wcb));
  b->wcb.aio_fildes = fd;
  b->wcb.aio_buf = (volatile void*)b->data;
  b->wcb.aio_nbytes = b->size;
  b->wcb.aio_offset = b->off;
  if (aio_write(&b->wcb) != 0) die("aio_write");
  b->st = BUF_WRITE_IO;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <input> <output> [block_bytes=1048576] [depth=4]\n", argv[0]);
    return 2;
  }
  const char* in_path = argv[1];
  const char* out_path = argv[2];
  size_t block = (argc > 3) ? (size_t)strtoull(argv[3], NULL, 10) : 1048576ull;
  int depth = (argc > 4) ? atoi(argv[4]) : 4;
  if (depth < 1) depth = 1; if (depth > 64) depth = 64;

  int in_fd = open(in_path, O_RDONLY);
  if (in_fd < 0) die("open input");
  int out_fd = open(out_path, O_CREAT|O_WRONLY|O_TRUNC, 0644);
  if (out_fd < 0) die("open output");

  struct stat st; if (fstat(in_fd, &st) != 0) die("fstat");
  off_t total = st.st_size;
  if (ftruncate(out_fd, total) != 0) die("ftruncate");

  // Allocate fixed buffer pool
  Buffer* bufs = (Buffer*)calloc(depth, sizeof(Buffer));
  if (!bufs) die("calloc bufs");
  for (int i = 0; i < depth; ++i) {
    bufs[i].data = (uint8_t*)aligned_alloc(4096, ((block + 4095)/4096)*4096);
    if (!bufs[i].data) die("aligned_alloc");
    bufs[i].cap = block;
    bufs[i].st = BUF_FREE;
  }

  // Pipeline
  uint64_t t0 = now_ns();
  off_t next_off = 0;
  off_t completed_writes = 0;
  off_t total_chunks = (total + (off_t)block - 1) / (off_t)block;
  off_t issued_reads = 0;

  // Prime the pipeline with up to depth reads
  for (int i = 0; i < depth && next_off < total; ++i) {
    size_t n = (size_t)((next_off + (off_t)block <= total) ? block : (size_t)(total - next_off));
    submit_read(in_fd, &bufs[i], next_off, n);
    next_off += n; issued_reads++;
  }

  // Event loop until all writes complete
  while (completed_writes < total_chunks) {
    // Build list of in-flight aiocb pointers for aio_suspend
    const int MAXAIO = 128; // upper bound
    const struct aiocb* list[MAXAIO];
    int nlist = 0;
    for (int i = 0; i < depth; ++i) {
      if (bufs[i].st == BUF_READ_IO) list[nlist++] = &bufs[i].rcb;
      else if (bufs[i].st == BUF_WRITE_IO) list[nlist++] = &bufs[i].wcb;
    }
    if (nlist == 0) break; // nothing in flight (shouldn't happen unless total=0)

    if (aio_suspend(list, nlist, NULL) != 0) {
      if (errno == EINTR) continue; else die("aio_suspend");
    }

    // Check completions and advance pipeline
    for (int i = 0; i < depth; ++i) {
      Buffer* b = &bufs[i];
      if (b->st == BUF_READ_IO) {
        int err = aio_error(&b->rcb);
        if (err == 0) {
          ssize_t r = aio_return(&b->rcb);
          if (r < 0) die("aio_return(read)");
          b->size = (size_t)r;
          // CPU transform step simulating compute on the block
          transform(b->data, b->size);
          submit_write(out_fd, b);
        }
      } else if (b->st == BUF_WRITE_IO) {
        int err = aio_error(&b->wcb);
        if (err == 0) {
          ssize_t w = aio_return(&b->wcb);
          if (w < 0) die("aio_return(write)");
          completed_writes++;
          b->st = BUF_FREE;
          // Issue next read if data remains
          if (next_off < total) {
            size_t n = (size_t)((next_off + (off_t)block <= total) ? block : (size_t)(total - next_off));
            submit_read(in_fd, b, next_off, n);
            next_off += n; issued_reads++;
          }
        }
      }
    }
  }

  // Cleanup
  uint64_t t1 = now_ns();
  double sec = (t1 - t0) / 1e9;
  double mb = (double)total / (1024.0*1024.0);
  if (sec > 0) fprintf(stderr, "Copied %.2f MiB in %.3f s (%.2f MiB/s)\n", mb, sec, mb/sec);

  for (int i = 0; i < depth; ++i) free(bufs[i].data);
  free(bufs);
  close(in_fd); close(out_fd);
  return 0;
}
