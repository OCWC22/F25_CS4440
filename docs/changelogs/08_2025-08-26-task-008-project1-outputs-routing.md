# Changelog: 08_2025-08-26 - Route Project 1 outputs to outputs/ directory (Task 008)

**Task:** [008] Route outputs of Project 1 programs into project_1/outputs/
**Status:** Done

### Files Updated:
- **UPDATED:** `project_1/MyCompress.c` – ensure `outputs/` exists and write to `outputs/<basename>`; improved status message
- **UPDATED:** `project_1/MyDecompress.c` – ensure `outputs/` exists and write to `outputs/<basename>`; improved status message
- **UPDATED:** `project_1/Prj1README` – document outputs routing and correct example commands to reference `outputs/`

### Description:
Updated the compression and decompression programs to consistently emit files under `project_1/outputs/` using the basename of the destination argument. Added README notes and examples to reflect the new behavior.

### Reasoning:
Centralizing outputs simplifies cleanup and validation, prevents stray artifacts in the repo, and aligns with repeatable testing. The logic is contained within each program to avoid Makefile coupling.

### Key Decisions & Trade-off:
- Implemented directory creation inside binaries (simple and robust) vs. adding Makefile rules (would not cover direct invocation).
- Kept destination basename semantics for backward familiarity while enforcing a stable outputs location.

### Notes:
- Fork wrapper `ForkCompress` works unchanged since it calls `MyCompress`, which now handles routing.
- Examples in README show reading compressed inputs from `outputs/` when running `MyDecompress`.

### How to Verify:
```bash
# from project_1/
make
printf '1111111111111111 0000000000000000 1010101010101010\n' > sample.txt
./MyCompress sample.txt compressed.txt
./MyDecompress outputs/compressed.txt decompressed.txt

diff -u sample.txt outputs/decompressed.txt
```
