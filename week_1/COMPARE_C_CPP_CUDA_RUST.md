# Compare: C vs C++ vs CUDA vs Rust (Directory list + File copy)

This document compares four implementations of two tiny tasks:
- List the current directory
- Copy a file (source → destination)

Files in `week_1/`:
- C++: `list_dir.cpp`, `copy_file.cpp`
- CUDA: `list_dir.cu`, `copy_file.cu`
- Rust: `list_dir.rs`, `copy_file.rs`

## Build & run quickstart (macOS)

- C++
  - Build: `clang++ -std=c++17 -Wall -Wextra -O2 -o list_dir_cpp list_dir.cpp`
  - Run: `./list_dir_cpp`
  - Build: `clang++ -std=c++17 -Wall -Wextra -O2 -o copy_file_cpp copy_file.cpp`
  - Run: `./copy_file_cpp source.txt destination_cpp.txt`

- CUDA (requires NVIDIA GPU + CUDA toolkit; not available on Apple Silicon)
  - Build: `nvcc -std=c++17 -O2 -o list_dir_cuda list_dir.cu`
  - Run: `./list_dir_cuda`
  - Build: `nvcc -std=c++17 -O2 -o copy_file_cuda copy_file.cu`
  - Run: `./copy_file_cuda source.txt destination_cuda.txt`

- Rust (requires `rustc`/`cargo`)
  - Build: `rustc -C opt-level=2 list_dir.rs -o list_dir_rs`
  - Run: `./list_dir_rs`
  - Build: `rustc -C opt-level=2 copy_file.rs -o copy_file_rs`
  - Run: `./copy_file_rs source.txt destination_rust.txt`

## Matrix: similarities and differences

| Aspect | C (originals) | C++ | CUDA | Rust |
|---|---|---|---|---|
| Directory listing approach | Exec external `ls -l` via `execvp` (process replacement) | Native `std::filesystem::directory_iterator` (no shell) | Native host-side `std::filesystem` (with a trivial CUDA kernel to show GPU availability) | Native `std::fs::read_dir` |
| Copy approach | Byte loop (`fgetc`/`fputc`) | `std::filesystem::copy_file` (fast, binary safe) | Same as C++ host-side | `std::fs::copy` |
| Error handling | Minimal in sample | Exceptions caught, clear messages | Same pattern; also reports CUDA availability | Results/Errors propagated, exits with codes |
| Portability | POSIX (execvp), fine on Linux/mac | Excellent with C++17 | Needs NVIDIA CUDA (limited platforms) | Excellent, cross-platform |
| Performance | Good; manual I/O | Uses system fast paths; usually optimal | Same as C++ for file ops (host); CUDA for compute-heavy tasks | Good; uses OS fast paths |
| Safety | Low-level, manual | Higher-level stdlib | Same as C++ + GPU when applicable | Strong safety guarantees by default |
| Typical use | OS education, syscalls | General apps, OpenCV, systems | GPU-accelerated compute kernels | Reliable tooling, CLI tools, services |

## When to use what

- **C++**: Default choice for apps with files, images, OpenCV. Modern stdlib (`<filesystem>`) simplifies safe, fast implementations.
- **CUDA**: Use only when you need GPU acceleration for heavy number crunching. File I/O remains on the CPU.
- **Rust**: Strong safety and great tooling; good for CLIs and services. Easy cross-platform builds.

## Notes for OpenCV workflows

- Prefer staying inside one process; avoid shelling out. In C++ use `std::filesystem` and OpenCV (`cv::imread`, `cv::imwrite`, `cv::glob`).
- For a pure copy, `std::filesystem::copy_file` is simpler and faster than reading/writing bytes manually.

## Exit codes

- Success: `0`  
- Bad usage: `2`  
- Operational error (I/O, etc.): `1`
