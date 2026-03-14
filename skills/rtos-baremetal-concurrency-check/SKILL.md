---
name: rtos-baremetal-concurrency-check
description: Audit concurrency behavior in ThreadX RTOS or bare-metal firmware. Use when requests mention race conditions, mutex/semaphore misuse, ISR-safe API boundaries, priority inversion, deadlock, shared buffer ownership, producer-consumer handoff, or blocking/sleep strategy.
---

# RTOS Bare-Metal Concurrency Check

Audit thread/ISR interaction and shared-state safety under real-time constraints.

## Workflow

1. Map shared resources and owners (thread, ISR, DMA callback).
2. Map synchronization primitives and wait/timeout paths.
3. Check ISR-safe API boundaries and critical section scope.
4. Identify deadlock, starvation, and priority inversion risks.
5. Output fixed sections:
- `Concurrency Topology`
- `Hazards`
- `Minimal Fix Options`
- `Realtime Validation Plan`
