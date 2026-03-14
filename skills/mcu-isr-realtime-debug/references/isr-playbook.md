# ISR Playbook

## High-Risk Anti-Patterns

1. Blocking call in ISR
2. Large formatting/logging inside ISR
3. Shared buffer without ownership boundary
4. Callback context not validated
5. Heavy computation in ISR

## Preferred Patterns

1. ISR set-flag, thread does heavy work
2. ISR push fixed-size event to queue/ring buffer
3. DMA-complete ISR only ack + schedule deferred handler
4. Keep ISR bounded and branch-light

## Validation Checklist

- Measure IRQ period and jitter before/after change
- Count missed IRQ or overflow events
- Verify deferred handler latency is within budget
- Run stress test for lockup or corruption
