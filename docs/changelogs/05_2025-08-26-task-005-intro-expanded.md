# Changelog: 05_2025-08-26 - Expanded Intro, Types, Control Flow Guide (Task 005)

**Task:** [TASK-005] Expand Week 1 Intro/Types/Control Flow guide with runnable labs and clear mapping to NGINX source  
**Status:** Done

### Files Updated:
- **UPDATED:** `week_1/nginx_essential_c_01_intro_types_control_flow.md` – Rewrote/expanded with sections on mental model, runnable C labs (headers vs sources, signedness pitfalls, status-driven control flow), concrete reading path, checklist, and authoritative NGINX snippets.

### Description:
Expanded the Intro guide to a self-contained, practical walkthrough that ties Essential C patterns directly to NGINX. Adds micro-labs and a 20–30 minute source reading path.

### Reasoning:
Executives/tech leads need a concise, source-anchored path to understand how NGINX’s status-driven, non-blocking model works. Runnable labs reinforce the patterns quickly.

### Key Decisions & Trade‑off:
- Focused on a few repeating patterns (status codes, early returns, typedefs) to avoid overload.
- Kept snippets short and cited file paths for authenticity.

### Next Steps:
- Optional: Add small references linking this guide to later week labs (pools, buffers, events) for continuity.
