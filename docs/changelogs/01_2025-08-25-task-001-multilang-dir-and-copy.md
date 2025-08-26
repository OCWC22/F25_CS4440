# Changelog: 01_2025-08-25 - Multilanguage directory list and file copy examples (Task 001)

**Task:** [001] Add C++, CUDA, and Rust versions of directory listing and file copy, plus comparison document  
**Status:** Done

### Files Updated:
- **CREATED:** `week_1/list_dir.cpp` – C++ directory listing using `std::filesystem`.
- **CREATED:** `week_1/copy_file.cpp` – C++ file copy using `std::filesystem::copy_file`.
- **CREATED:** `week_1/list_dir.cu` – CUDA host-side directory listing with GPU noop kernel.
- **CREATED:** `week_1/copy_file.cu` – CUDA host-side file copy with GPU noop kernel.
- **CREATED:** `week_1/list_dir.rs` – Rust directory listing using `std::fs::read_dir`.
- **CREATED:** `week_1/copy_file.rs` – Rust file copy using `std::fs::copy`.
- **CREATED:** `week_1/COMPARE_C_CPP_CUDA_RUST.md` – Comparison/matrix and usage notes.

### Description:
Added minimal, production-leaning examples for directory listing and file copying in C++, CUDA, and Rust, along with a comparison markdown and quickstart build commands. This enables side-by-side learning and quick validation.

### Reasoning:
Provide clear, safe, and modern equivalents of the original C programs, mapping to common real-world choices (C++/OpenCV, CUDA for GPU, Rust for safety).

### Key Decisions & Trade-off:
- Prefer native APIs (`std::filesystem`, `std::fs`) over shelling out for portability and safety.
- Include CUDA noop kernel to illustrate environment constraints; file ops remain on host.
- Keep examples minimal to reduce cognitive load and ease compilation.

### Risks & Mitigations:
- CUDA may not be available on the local machine (macOS/Apple Silicon). Files compile only where CUDA is installed; noted clearly in docs.
- Rust toolchain may not be installed; commands provided.

### Next Steps:
- Optionally add C++ OpenCV demo using `cv::imread/cv::imwrite` and `cv::glob`.
- Add unit tests or simple scripts to validate behavior across languages.
