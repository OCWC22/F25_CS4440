# Changelog: 06_2025-08-26 - Week 1 Guides Formatting Cleanup (Task 006)

**Task:** [006] Week 1 Guides Formatting Cleanup  
**Status:** Done

### Files Updated:
- **UPDATED:** `week_1/nginx_essential_c_01_intro_types_control_flow.md` – normalized code and output fences in sections 4.1–4.3; removed stray markers; preserved CEO explanations.
- **UPDATED:** `week_1/nginx_essential_c_02_pointers_and_memory_pools.md` – closed code blocks; added fenced Build/Run (bash) and Output (text); removed stray `code/C/Code` artifacts.
- **UPDATED:** `week_1/nginx_essential_c_03_strings_and_buffers.md` – promoted H1; removed markdown wrapper and path comment; normalized code fences and outputs; removed unrelated trailing notes.
- **UPDATED:** `week_1/nginx_essential_c_04_arrays_lists_hashes.md` – normalized fences; added Build/Run and Output blocks; removed stray tokens; ensured conceptual code remains fenced as code.
- **UPDATED:** `week_1/nginx_essential_c_05_structs_enums_typedefs.md` – moved H1 to top; removed wrappers/path comment; fenced all code examples and reference snippets; added Build/Run and Output blocks.
- **UPDATED:** `week_1/nginx_essential_c_06_function_pointers_callbacks_modules.md` – removed "File 6" label and markdown wrapper; removed path comment; normalized all code fences; added Build/Run and Output sections; fenced NGINX reference snippet.
- **UPDATED:** `week_1/nginx_essential_c_07_io_sockets_events.md` – removed "File 7" label and markdown wrapper; closed/opened fences correctly; wrapped conceptual and reference snippets in `c` fences.
- **UPDATED:** `week_1/nginx_essential_c_08_build_debug_cross_platform.md` – moved H1 to top; removed wrapper/path comment; fenced bash command blocks; moved CEO explanations outside code fences.

### Description:
Standardized formatting across all Week 1 guides to improve readability and copy/paste reliability. Consolidated H1 placement, removed stray wrappers and artifacts, and normalized code/output fencing with proper language tags.

### Reasoning:
Consistent, clean formatting reduces cognitive load for technical leaders and prevents copy errors during micro-exercises. Clear separation of code, commands, and outputs improves learnability and cross-platform execution.

### Key Decisions & Trade‑off:
- Chose simple, explicit fenced blocks (`c`, `bash`, `text`) to avoid ambiguity; minimal Markdown features to maximize portability across editors.
- Preserved all technical content and CEO-focused prose; no content changes beyond formatting corrections.
- Kept conceptual code as fenced examples (not runnable) where intended; added Build/Run only for runnable labs.

### Notable References:
- NGINX source (for context): `src/core/ngx_palloc.c`, `src/http/ngx_http_request.c`, `src/event/ngx_event.h`, `src/http/ngx_http_config.h`.

### Follow-ups:
- Consider adding a small “Conventions” preface in Week 1 index summarizing fence usage and copy/paste tips.
- If desired, add platform-specific notes under Build/Run for Windows/MSYS where applicable.
