---
name: ra-startup-boot-chain-audit
description: Analyze Renesas RA/FSP startup and boot-chain failures in C/C++ firmware. Use when requests mention Reset_Handler, startup.c/system.c, R_BSP_WarmStart, vector table, tx_kernel_enter, early hardfault, boot hang before main or thread entry, clock/pin init timing, or linker/vector placement issues.
---

# RA Startup Boot Chain Audit

Build an evidence-based startup timeline and isolate boot-order failures.

## Workflow

1. Build startup timeline: `Reset_Handler -> SystemInit -> R_BSP_WarmStart -> main/tx_kernel_enter -> thread entry`.
2. Read startup-critical files and map first use of clocks, pins, IRQ, and drivers.
3. Check init-order hazards: peripheral used before prerequisite init, callback/IRQ enabled before context ready.
4. Check vector/linker integrity: entry symbol, vector section placement, memory region mapping.
5. Output fixed sections:
- `Boot Timeline`
- `Failure Candidates`
- `Minimal Change Plan`
- `Hardware Validation Steps`

## References

Read `references/boot-checklist.md` when symptoms are startup hang, early hardfault, or pre-thread boot failure.
