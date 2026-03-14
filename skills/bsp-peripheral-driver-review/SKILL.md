---
name: bsp-peripheral-driver-review
description: Review and design MCU BSP/peripheral drivers for C/C++ firmware. Use when requests involve UART/I2C/SPI/ADC/DAC/RTC/GPIO/ELC/DMAC drivers, API contract review, init/deinit order, register access safety, callback context, timeout/error handling, or bring-up checklists.
---

# BSP Peripheral Driver Review

Review or design driver logic with hardware-accurate and low-risk changes.

## Workflow

1. Build driver contract table: init, start/stop, read/write, callback, error path.
2. Check init/deinit and dependency order against real peripheral requirements.
3. Check callback and ISR interaction, buffer ownership, and timeout behavior.
4. Check register-level assumptions and edge-case handling.
5. Output fixed sections:
- `Driver Contract Findings`
- `Risk and Regression Points`
- `Minimal Patch Plan`
- `Validation on Hardware`
