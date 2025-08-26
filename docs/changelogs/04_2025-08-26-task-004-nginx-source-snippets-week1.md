# Changelog: 04_2025-08-26 - Embed NGINX Source Snippets in Week 1 Guides (Task 004)

**Task:** [TASK-004] Add authoritative NGINX C source snippets to Week 1 Essential C guides  
**Status:** Done

### Files Updated:
- **UPDATED:** `week_1/nginx_essential_c_01_intro_types_control_flow.md` – Added `NGX_OK`/`NGX_ERROR` family and core typedefs.
- **UPDATED:** `week_1/nginx_essential_c_02_pointers_and_memory_pools.md` – Added `ngx_pool_s` and key pool APIs.
- **UPDATED:** `week_1/nginx_essential_c_03_strings_and_buffers.md` – Added `ngx_str_t`, `ngx_buf_s`, `ngx_chain_s`.
- **UPDATED:** `week_1/nginx_essential_c_04_arrays_lists_hashes.md` – Added `ngx_array_t` with `ngx_array_init`, and `ngx_list_t`/`ngx_list_part_s`.
- **UPDATED:** `week_1/nginx_essential_c_05_structs_enums_typedefs.md` – Added `ngx_http_request_s` field excerpt and `ngx_http_state_e` enum.
- **UPDATED:** `week_1/nginx_essential_c_06_function_pointers_callbacks_modules.md` – Added `ngx_module_s` and `ngx_http_module_t` context.
- **UPDATED:** `week_1/nginx_essential_c_07_io_sockets_events.md` – Added event flags from `ngx_event.h`.

### Description:
Embedded concise, authoritative code snippets from official NGINX headers into Week 1 Markdown guides. Each snippet is paired with minimal context to illustrate Essential C concepts (return codes, pools, strings/buffers, containers, structs/enums, modules, event flags) using production code.

### Reasoning:
Using real NGINX source anchors learning in practical examples and supports the goal of teaching Essential C fundamentals to a non-technical audience by example, not contrived code. Snippets are short, focused, and traceable back to their source files.

### Key Decisions & Trade‑off:
- Kept snippets minimal to avoid overwhelming readers while remaining faithful to source.
- Prioritized common, representative definitions over exhaustive coverage.
- Deferred adding debug macro snippets to the Build & Debug guide to keep this change focused.

### Alternatives Considered:
- Linking only to source without embedding: reduced immediacy for learners.
- Longer excerpts: higher cognitive load for Week 1 audience.

### Implementation Notes:
- Snippets copied from: `src/core/ngx_core.h`, `src/core/ngx_config.h`, `src/core/ngx_palloc.h`, `src/core/ngx_string.h`, `src/core/ngx_buf.h`, `src/core/ngx_array.h`, `src/core/ngx_list.h`, `src/http/ngx_http_request.h`, `src/core/ngx_module.h`, `src/http/ngx_http_config.h`, `src/event/ngx_event.h`.
- Text adjusted minimally for formatting; file path comments included atop code blocks.

### Next Steps:
- Optionally add a brief snippet in `nginx_essential_c_08_build_debug_cross_platform.md` showing `ngx_log_debug*` macros and enabling `--with-debug` (already noted) for source-aligned debugging examples.
- Add micro-exercises referencing the new snippets (some already present).
