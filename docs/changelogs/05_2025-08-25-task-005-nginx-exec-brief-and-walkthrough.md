# Changelog: 05_2025-08-25 — Exec Brief + Walkthrough (Task 005)

**Task:** [005] Add non-technical CEO/executive guides that explain NGINX and Essential C in plain English with a simple code walk-through.

**Status:** Done

### Files Added
- **CREATED:** `week_1/nginx_for_execs_01_big_picture.md` — NGINX in 1 page for execs: why it matters, KPIs, risks, questions to ask.
- **CREATED:** `week_1/nginx_for_execs_02_request_journey.md` — A 10-step “request journey” from socket → response, mapped to NGINX/C.
- **CREATED:** `week_1/nginx_for_execs_03_c_fundamentals_with_analogies.md` — Pointers, structs, memory pools, strings/buffers explained with everyday analogies + tiny, harmless C snippets.

### Why
Leaders need to understand what “good” looks like without learning C syntax. These guides keep to outcomes (latency, throughput, safety) while bridging to how the code actually achieves them (memory pools, event loop, buffers, return codes).

### What Changed
Documentation only. No code execution impact.

### How to Use
Skim `nginx_for_execs_01_big_picture.md` first (5 min), then the “request journey” (8–10 min). Keep the checklists handy when reviewing plans/PRDs or joining engineering reviews.