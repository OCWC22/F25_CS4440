# Production I/O + GPU Overlap Demo (C, C++)

This demo shows practical multiprogramming I/O on CPU and overlapped GPU execution.

Contents
- cpu_io_demo.c — Asynchronous file copy with bounded pipeline and transform.
  - Default: POSIX AIO (macOS/Linux)
  - Optional: Linux io_uring backend (-DUSE_IO_URING and liburing)
- gpu_pipeline_cuda.cu — CUDA streams: H2D → kernel → D2H with triple buffering.
- gpu_pipeline_fallback.cpp — CPU-only pipeline that mimics the CUDA stages for portability.

Requirements
- macOS or Linux.
- Build tools: clang/gcc, make.
- Optional (Linux): liburing dev package to enable io_uring.
- Optional (GPU): NVIDIA CUDA toolkit if you want to build the CUDA target.

Build
- Default (portable targets only):
  make -C src/production_io_demo

- Linux with io_uring backend:
  make -C src/production_io_demo IO_URING=1

- CUDA target (if nvcc available):
  make -C src/production_io_demo gpu

Run
- CPU async I/O demo:
  ./src/production_io_demo/bin/cpu_io_demo input.bin output.bin 1048576 4
  Args: <in> <out> [block_bytes=1048576] [depth=4]

- GPU pipeline (CUDA):
  ./src/production_io_demo/bin/gpu_pipeline_cuda 100000000 1048576
  Args: <num_elements> <chunk_elements>

- GPU pipeline (fallback, CPU):
  ./src/production_io_demo/bin/gpu_pipeline_fallback 100000000 1048576

Notes (production)
- Use bounded queues and fixed buffer pools to avoid unbounded pinned memory.
- Keep buffers NUMA-local to NIC/NVMe on Linux; prefer direct I/O when needed.
- For CUDA, use pinned host memory and multiple streams, plus events for dependencies.
- Add monitoring for queue depths, latency histograms, and error budgets.
