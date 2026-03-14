---
name: firmware-memory-stability-analysis
description: Analyze memory map and long-run stability risks in resource-constrained MCU firmware. Use when requests mention linker scripts, section placement, stack usage, .bss/.data growth, noinit regions, memory corruption hardfaults, heap-avoidance strategy, or realtime stability under load.
---

# Firmware Memory Stability Analysis

Analyze memory layout, runtime pressure, and corruption risk with embedded constraints.

## Workflow

1. Build memory map from linker and region files.
2. Identify high-risk sections: stack, .bss, DMA buffers, noinit, external memory mappings.
3. Check allocation strategy and lifetime boundaries.
4. Correlate fault signatures with likely overwrite or starvation paths.
5. Output fixed sections:
- `Memory Layout Summary`
- `Stability Risk Points`
- `Minimal Mitigations`
- `Verification Steps`
