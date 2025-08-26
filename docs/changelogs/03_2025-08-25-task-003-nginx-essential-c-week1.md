# Changelog: 03_2025-08-25 - Week 1 NGINX-Essential C Guides (Task 003)

**Task:** [003] Create Week 1 guides teaching Essential C via NGINX source
**Status:** Done

### Files Updated:
- **CREATED:** `week_1/nginx_essential_c_01_intro_types_control_flow.md` – Intro, types, return codes, control flow with NGINX pointers
- **CREATED:** `week_1/nginx_essential_c_02_pointers_and_memory_pools.md` – Pointers and NGINX pool allocator (`ngx_palloc`)
- **CREATED:** `week_1/nginx_essential_c_03_strings_and_buffers.md` – `ngx_str_t`, `ngx_buf_t`, `ngx_chain_t`
- **CREATED:** `week_1/nginx_essential_c_04_arrays_lists_hashes.md` – `ngx_array_t`, `ngx_list_t`, hash APIs
- **CREATED:** `week_1/nginx_essential_c_05_structs_enums_typedefs.md` – Reading big structs (`ngx_http_request_t`), enums, typedefs
- **CREATED:** `week_1/nginx_essential_c_06_function_pointers_callbacks_modules.md` – Function pointers and module wiring
- **CREATED:** `week_1/nginx_essential_c_07_io_sockets_events.md` – Files/sockets, non-blocking I/O, event loop
- **CREATED:** `week_1/nginx_essential_c_08_build_debug_cross_platform.md` – Build & debug on macOS/Linux/Windows (WSL2 recommended)

### Description:
Eight concise, production-grounded guides that teach Stanford Essential C topics by reading and tracing real NGINX code. Tailored for a fast-moving technical leader to gain practical comprehension in Week 1.

### Reasoning:
Using widely deployed, high-performance NGINX code bases the C fundamentals in realistic patterns: pools, length-prefixed strings, non-blocking I/O, function-pointer phase engines, and portable typedefs. This accelerates comprehension versus toy examples.

### Key Decisions & Trade-off:
- Focused on HTTP core and event loop for universally relevant concepts.
- Windows path uses WSL2 to keep parity with Linux; native Windows builds differ and are less representative for server internals.
- Kept each guide short and actionable with concrete source file anchors and micro-exercises to avoid overwhelm while preserving depth.

### Implementation Notes:
- macOS configure uses Homebrew for OpenSSL/PCRE2 paths and enables `--with-debug`.
- Linux uses standard dev packages and GDB; macOS uses LLDB; both demonstrate breakpoints on `ngx_http_process_request`.
- All guides link to specific source directories: `src/core/`, `src/http/`, `src/event/`.

### Testing & Validation:
- Documentation-only change. Commands and paths align with common NGINX build workflows on macOS and Ubuntu/Debian; Windows guidance via WSL2.

### Next Steps:
- Optional: add a toy HTTP module exercise and step-by-step walkthrough (Week 2).
- Optional: diagrams for `ngx_buf_t` chains and HTTP phase engine.

### References:
- NGINX Source: `src/core/*`, `src/http/*`, `src/event/*`
- Pool allocator: `src/core/ngx_palloc.c`
- HTTP request lifecycle: `src/http/ngx_http_request.c`
- Event loop: `src/event/ngx_event.c`, platform modules in `src/event/modules/`
