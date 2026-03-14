---
name: mcu-isr-realtime-debug
description: Diagnose MCU interrupt latency and real-time regressions for RTOS or bare-metal firmware. Use when requests mention ISR jitter, missed IRQ, interrupt storm, callbackSet context, vector mapping, DMAC/DTC ISR interactions, ISR-to-thread handoff, priority inversion, or deadline violations.
---

# MCU ISR Realtime Debug

Diagnose interrupt latency regressions and missed-deadline behavior in IRQ paths.

## Workflow

1. Map IRQ chain end-to-end: vector -> HAL ISR -> user callback -> deferred thread/work item.
2. Build ISR budget table: source period, worst-case ISR time, nesting risk, deadline.
3. Audit ISR/thread boundary: ISR-safe API usage, blocking risk, ownership of shared buffers/flags.
4. Audit transfer-complete IRQ path for DMA/DTC: ack/clear behavior, callback context, channel mapping.
5. Output fixed sections:
- `IRQ Path Map`
- `Budget and Overrun Risks`
- `Root Cause Candidates`
- `Minimal Remediation Plan`
- `Timing Validation Plan`

## References

Read `references/isr-playbook.md` for anti-patterns and remediation templates.
