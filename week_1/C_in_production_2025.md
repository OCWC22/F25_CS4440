# Where C is used in production in 2025 (with authoritative sources)

C remains central to systems, networking, security, databases, media, and AI runtimes because it’s small, predictable, portable, and maps closely to OS/hardware. Below is a concise, link-first summary.

## TL;DR
- C is the backbone for kernels/OS, high-throughput networking, cryptography, embedded DBs, codecs, and AI math backends. 
- App logic is often written in higher-level languages (C++, Rust, Python), but the hot path remains C.

## Operating systems and kernel-adjacent
- Linux kernel (C, GNU dialect gnu11)
  - Programming language note: https://www.kernel.org/doc/html/latest/process/programming-language.html
  - Releases: https://www.kernel.org/category/releases.html
- eBPF ecosystem (kernel + user space)
  - What is eBPF: https://ebpf.io/what-is-ebpf/
  - Applications landscape: https://ebpf.io/applications/
- systemd (system/service manager, C)
  - Site: https://systemd.io/

## Networking, proxies, web
- NGINX (web server/reverse proxy, C)
  - Docs: https://docs.nginx.com
  - Source: https://github.com/nginx/nginx
- HAProxy (TCP/HTTP load balancer, C)
  - Site: http://www.haproxy.org/
  - Source: https://github.com/haproxy/haproxy
- WireGuard (modern VPN; kernel/userspace implementations)
  - Repositories: https://www.wireguard.com/repositories/
  - Embedding: https://www.wireguard.com/embedding/
- libcurl (client‑side URL transfers, C API)
  - Site/API: https://curl.se/libcurl/

## Security and cryptography
- OpenSSL (TLS/SSL + crypto, C)
  - Docs: https://docs.openssl.org/3.1/man3/
  - Source: https://github.com/openssl/openssl/
- zlib (compression, C)
  - Site: https://zlib.net/

## Databases, storage, caching
- SQLite (embedded SQL database, C API)
  - C interface intro: https://www.sqlite.org/cintro.html
- PostgreSQL (server core is C; C extension interface)
  - C-language functions: https://www.postgresql.org/docs/current/xfunc-c.html
- Redis (in‑memory data store, ANSI C)
  - About: https://redis.io/about/

## Media and streaming
- FFmpeg (codecs, mux/demux, filters; C)
  - Site: https://ffmpeg.org/
  - Dev docs: https://ffmpeg.org/developer.html
- GStreamer (multimedia framework, C)
  - Site: https://gstreamer.freedesktop.org/
  - FAQ: https://gstreamer.freedesktop.org/documentation/frequently-asked-questions/general.html
- libjpeg‑turbo (JPEG codec, C)
  - Site: https://libjpeg-turbo.org/
- libpng (PNG library, C)
  - Site: https://www.libpng.org/pub/png/libpng.html

## AI/ML runtimes and math backends
- ONNX Runtime (C and C++ APIs for inference/training engine)
  - C/C++ API docs: https://onnxruntime.ai/docs/api/c/
- cuDNN (NVIDIA DNN primitives, C API)
  - Overview: https://developer.nvidia.com/cudnn
  - Docs: https://docs.nvidia.com/deeplearning/cudnn/latest/
- OpenBLAS (BLAS math library, C)
  - Site: https://www.openblas.net/
  - Source: https://github.com/OpenMathLib/OpenBLAS

## Developer tooling and runtimes
- Git (version control, largely C)
  - Source: https://github.com/git/git
- CPython (Python reference interpreter, C; C extension API)
  - Extending with C/C++: https://docs.python.org/3/extending/extending.html
- libuv (cross‑platform async I/O used by Node.js, C)
  - Site: https://libuv.org/
  - Source: https://github.com/libuv/libuv

---

## Practical guidance
- Typical “C role” in AI stacks: fast runtime/ops kernels (cuDNN, BLAS), model serving engines (ONNX Runtime C API), high‑throughput I/O (libcurl), and media preprocess (FFmpeg/libjpeg/libpng).
- For your repo, read this alongside `week_1/README.md` and `week_1/COMPARE_C_CPP_CUDA_RUST.md` to connect concepts to runnable examples.

## Suggested next steps in this repo
- Add small demos that call C libraries from C++ (e.g., use libpng or libcurl) to practice build/linking.
- If you want, I can scaffold a minimal CMake example showing how to link a C library and call it safely from C++.
