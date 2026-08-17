# Debug log guide

`logs/debug/latest.log` is replaced every time the mod loads.

Useful records:

- `[Record] frame=...` — exact captured input.
- `[Playback] ...` — replay lifecycle.
- `[Analyzer] next input#... offset=...` — branch that is about to run.
- `[Branch] ... result=PASS/FAIL reason=...` — result used by the Frame Window summarizer.
- `death at frame X causePtr=...` — candidate death frame and raw cause object pointer.
- `[Stepper] ...` — pause/frame-step state.

A clean analysis report is never mixed with these messages.

## Common diagnosis

### Baseline FAIL
The original recorded timing itself could not survive the replay validation horizon. Do not trust that input's window (`N_i=0`). Check for replay desync, unrecorded state, checkpoints, random triggers, or insufficient state capture.

### Window touches scan boundary
Increase **Frame Scan Radius**. The result is clipped and may be larger than reported.

### Fast scan misses an unusual PASS island
Enable **Exhaustive Scan**. Fast scan stops each direction after the first FAIL because normal frame windows are expected to be contiguous around the successful recorded timing.

### Input has delayed consequences
Increase **Post-macro Validation Frames** or record the macro farther beyond the timing.
