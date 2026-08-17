# FWBot v0.2.0

FWBot is a Windows Geode bot for Geometry Dash 2.2081 / Geode 5.8.2 focused on deterministic practice recording and automatic HOLD/RELEASE frame-window analysis.

## Core workflow

1. Enter Practice Mode and place a checkpoint just before the section you want to analyze.
2. Open FWBot with **F8**.
3. Press **F6** (or Record in the ImGui panel). Frame 0 becomes the current gameplay/practice state; FWBot does not restart the level.
4. Play the section. You can enable FWBot Frame Stepper with **F4**, then use **F5** for one physics tick at a time.
5. Press **F6** again to stop. With Auto Analyze enabled, FWBot restores the saved anchor and tests every HOLD/RELEASE at earlier/later frames.
6. Reports are written to the mod save directory under `logs/` as `.log`, `_nandl.csv`, `_nandl.json`. Detailed diagnostics are in `logs/debug/latest.log`.

## Binds

- F3 — FWBot speedhack toggle
- F4 — Frame Stepper toggle (physics freeze; **not** Geometry Dash PauseLayer)
- F5 — Next physics frame
- F6 — Start/stop recording from current practice state
- F7 — Playback last macro from its practice anchor
- F8 — FWBot ImGui menu
- Ctrl+F5 — trajectory
- Ctrl+F7 — analyze last macro

All binds are editable in Geode settings.

## Frame Window / NaNDL

For each input FWBot treats the recorded successful frame as offset 0, then tests frames to the left and right using real gameplay branches. Example: if a wave timing is passable on offsets `-1, 0, +1, +2`, the result is `Early=1`, `Late=2`, `N_i=4` even if the recorded click was the second valid frame.

`FRAME WINDOW (N_i)` is exported for NaNDL together with frame/time data. Exhaustive Scan can reveal disjoint passing segments; the NaNDL value remains the contiguous passing island containing the recorded timing.

## Practice anchor

The v0.2 runtime captures a retained `CheckpointObject` plus input-state metadata at Record start. Playback and analysis force-load that anchor rather than resetting to frame 0 of the level. This is the same high-level model used by Silicate's practice/checkpoint system, while FWBot keeps a smaller state surface.

## Frame Stepper

The stepper intercepts gameplay update directly. When enabled with no queued step, game physics receives no update; UI/rendering remain alive. `Next frame` consumes one request, clears extra delta, and advances exactly one FWBot fixed physics tick. Disabling the stepper clears queued/overflow state so the game does not catch up in a burst.

## Speedhack + audio

FWBot includes a manual speedhack. If `Audio follows speed` is enabled, the FMOD master channel pitch follows the speed multiplier. Automatic frame-window analysis remains fixed-tick and does not use the manual speedhack path.

## Trajectory

v0.2 replaces the old historical line with predictive fake-player HOLD/RELEASE branches based on the Silicate trajectory architecture (`copyAttributes`, fake players, push/release simulation). It is intentionally isolated from analyzer verdicts: exact Frame Window PASS/FAIL always comes from real PlayLayer branch simulation. Collision-complete Silicate-level predictive physics remains a separate backend target because Silicate also uses dedicated physics/collision helpers beyond the fake player itself.

## Build

Portable analyzer tests:

```bash
cmake -S . -B build-core -DFWBOT_CORE_ONLY=ON -DFWBOT_BUILD_TESTS=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

The included GitHub Actions workflow builds Win64 with Geode 5.8.2 and fetches pinned Dear ImGui v1.92.4 with retry logic.

## License / credits

FWBot is GPL-3.0. Selected bot architecture is derived from/inspired by Silicate and is credited in `CREDITS.md`. Dear ImGui is fetched as an external MIT-licensed dependency during the full Windows build.
