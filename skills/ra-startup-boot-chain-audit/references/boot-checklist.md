# Boot Checklist

## Symptom to Candidate Root Cause

1. Hang before thread entry:
- Init sequence mismatch
- Early IRQ firing before callback/context setup
- Peripheral access before clock or pin init

2. Early hardfault:
- Vector table or linker placement mismatch
- Invalid callback pointer/context
- Section mapping overlap or invalid memory region assumptions

3. Boot-only peripheral failure:
- WarmStart phase mismatch (`POST_CLOCK` vs `POST_C`)
- Driver open order not matching hardware dependency

## Evidence Template

- Boot stage:
- First failing function:
- Last known-good call:
- Related IRQ/vector:
- Related linker section:
- Candidate root cause:
- Minimal fix:
- On-board validation:
