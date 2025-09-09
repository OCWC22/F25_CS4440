# Changelog: 07_2025-08-26 - Project 1 initial implementation (Parts 1–3)

**Task:** [007] Project 1 starter: MyCompress, MyDecompress, ForkCompress + Makefile/README  
**Status:** Done

### Files Updated:
- **CREATED:** `project_1/MyCompress.c` – syscall-based run-length compression (>=16) using tokens `-N-` for zeros, `+N+` for ones; preserves separators.
- **CREATED:** `project_1/MyDecompress.c` – expands `-N-`/`+N+` tokens back into bits; robust parsing with fallbacks.
- **CREATED:** `project_1/ForkCompress.c` – forks and execs `./MyCompress`, waits and reports exit status.
- **CREATED:** `project_1/Makefile` – builds three targets with strict warnings enabled.
- **CREATED:** `project_1/Prj1README` – build/run instructions and testing notes.

### Description:
Implemented the first three parts of Project 1 using clean, syscall-centric C programs, added a Makefile for reproducible builds, and included a concise README. Verified compilation via `make`.

### Reasoning:
- Start with minimal, correct, and robust building blocks that match the assignment spec.
- Use only Unix system calls for IO in compression/decompression to meet learning objectives.
- Provide strict compiler flags to catch errors early and keep code production-grade.

### Key Decisions & Trade‑off:
- Chosen threshold-based run-length encoding strictly on >=16-length runs to match spec; shorter runs copied verbatim to balance simplicity and speed.
- Token grammar `-N-`/`+N+` kept simple; decompressor is forgiving (falls back to literal on malformed tokens) to avoid brittle failures.

### How to Build & Test:
- Build: `cd project_1 && make`
- Run: `./MyCompress input_bits.txt compressed.txt && ./MyDecompress compressed.txt roundtrip.txt && diff -u input_bits.txt roundtrip.txt`

### Next Steps:
- Implement remaining parts (pipe-based compressor, parallel fork version, mini shell with args and pipes, pthreads version) and benchmarking for part 10.
