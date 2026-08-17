# FWBot v0.2.2 debug guide

`logs/debug/latest.log` is replaced when FWBot initializes.

Useful records:

- `[Record] started ... scheduler remains live` — Record should not freeze gameplay.
- `[Record] frame=...` — captured HOLD/RELEASE frame.
- `[Stepper] enabled=...` — stepper lifecycle.
- `[Stepper] queued one scheduler/physics tick` — F5 request.
- `[Speedhack] speed=...` / `audioFollow=...` — manual speed state.
- `[Practice] FWBot anchor loaded` — restore completed.
- `[Analyzer] next input#... offset=...` — next candidate branch.
- `[Branch] ... PASS/FAIL ...` — verdict used in Frame Window calculation.

## If gameplay freezes

1. Press F4 once to disable Frame Stepper.
2. Open FWBot > Debug and press **Disable stepper** / **Cancel playback / analysis**.
3. Send `logs/debug/latest.log` if gameplay still does not resume.

In v0.2.2 Record itself disables stale stepper state before capturing the Practice anchor.

## If trajectory lines remain

Trajectory starts OFF every launch. Ctrl+F5 toggles it. v0.2.2 removes old draw/fake nodes from the PlayLayer on reset/toggle/exit instead of only forgetting their pointers. If any line survives after disabling trajectory, send a screenshot plus `latest.log` and note whether it happened after death, checkpoint reset, Playback or Analyze.

## If speedhack audio is wrong

Toggle F3 or change the speed slider while staying in the same attempt. Pitch should update immediately; it is re-applied every visual frame and after reset. Automatic Analyze intentionally returns audio to normal pitch while branches are processed.

## Analyzer diagnosis

### Baseline FAIL
The original recorded timing did not survive replay from the saved Practice anchor. Treat `N_i=0` as a desync signal rather than a valid difficulty result. Send the debug log.

### Window touches scan radius
Increase **Frame Scan Radius**. A boundary-touching result may be clipped.

### Unusual PASS island
Enable **Exhaustive Scan**. Normal Fast Scan stops each side at the first FAIL.

### Delayed death
Increase **Post-macro Validation Frames** or record farther past the timing.
