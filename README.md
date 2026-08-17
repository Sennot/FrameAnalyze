# FWBot v0.2.3

FWBot is a Windows Geode bot for Geometry Dash 2.2081 / Geode 5.8.2 focused on Practice recording, deterministic playback and automatic HOLD/RELEASE Frame Window analysis for NaNDL.

## Recommended workflow

1. Enter Practice Mode and place a checkpoint immediately before the timing/section.
2. Open FWBot with **F8**.
3. Press **F6** / Record. The current Practice state becomes macro frame 0. Record no longer freezes or owns the gameplay update loop.
4. Play normally, or enable **F4 Frame Stepper** and use **F5** for one scheduler/physics step at a time.
5. Press **F6** again. If Auto Analyze is enabled, FWBot restores the same anchor and tests each recorded HOLD/RELEASE earlier and later.
6. Read `logs/*.log`, `*_nandl.csv`, `*_nandl.json`; detailed runtime diagnostics are in `logs/debug/latest.log`.

## Binds

- F3 — FWBot speedhack
- F4 — Frame Stepper toggle (no Geometry Dash PauseLayer)
- F5 — next physics step
- F6 — Record / Stop
- F7 — Playback
- F8 — FWBot ImGui menu
- Ctrl+F5 — predictive trajectory
- Ctrl+F7 — Analyze Last

All binds are editable in Geode settings.

## v0.2.3 stability architecture

### Scheduler-level Frame Stepper

FWBot no longer manually loops `GJBaseGameLayer::update`. Time control is hooked at `CCScheduler::update`:

- stepper OFF: scheduler receives normal `dt * speed`;
- stepper ON with no request: scheduler gameplay update is skipped while rendering/ImGui remains alive;
- F5: one request is consumed and scheduler receives one `1/240` step;
- enabling/disabling stepper clears queued steps and `m_extraDelta`, preventing the old catch-up/frozen-state behavior.

Recording can run in realtime or while the stepper is enabled. Starting Record clears a stale stepper state first, so pressing Record itself never intentionally freezes the player.

### Physics-frame replay clock

Macro injection and frame accounting live in `GJBaseGameLayer::processCommands`, inside the gameplay physics loop. This means playback and analysis continue to count actual physics steps even when one rendered frame contains several game updates at speed.

### Practice anchor

Record captures the current `CheckpointObject`, `GJGameState`, `EffectManagerState` and held-input metadata. Playback and analysis return to this same anchor instead of restarting the whole level.

### Speedhack + immediate audio

Manual speedhack operates at scheduler level. If **Audio follows speed** is enabled, FWBot applies the FMOD master-group pitch immediately and re-synchronizes it on visual frames and resets. Automatic analysis uses 1–16 exact `1/240` scheduler ticks per rendered frame and does not speed up audio.

### Trajectory cleanup and safety

Predictive trajectory is **OFF by default every launch** and can only be enabled explicitly from the menu / Ctrl+F5. Old persisted `show-trajectory` state was removed.

Every reset, death/branch restore, visibility toggle and PlayLayer exit destroys previous trajectory draw/fake-player nodes with `removeFromParentAndCleanup(true)`. This fixes the stale long-line accumulation from v0.2.1.

Prediction uses fake-player HOLD/RELEASE branches, collision checks and save/simulate/restore of gameplay/effect state. It remains visual only: Frame Window PASS/FAIL never depends on the trajectory renderer.

### Frame Window / NaNDL

For each input, recorded timing is offset 0 and FWBot tests earlier/later candidate frames with real gameplay branch playback. Example: pass offsets `-1, 0, +1, +2` produce:

- Early = 1
- Late = 2
- `FRAME WINDOW (N_i) = 4`

This remains true even if the recorded click was the second valid frame of the gap. A branch can only PASS after its final recorded input frame has actually been processed, plus the configured validation frames. Fast Scan stops after the first FAIL on each side; Exhaustive Scan can discover disjoint PASS islands. NaNDL `N_i` is the contiguous passing island containing the recorded timing.

## ImGui menu

The menu is a black tabbed Dear ImGui interface: **Macro / Tools / Analysis / Visuals / Debug**. It defaults to a right-side safe area below the top progress UI, is draggable, is clamped inside a safe screen region and persists its position in `fwbot_imgui.ini` under the mod save directory. FWBot does not add a separate Cocos button on top of gameplay.

## Build / tests

Portable core:

```bash
cmake -S . -B build-core -DFWBOT_CORE_ONLY=ON -DFWBOT_BUILD_TESTS=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
python3 tests/source_contract_tests.py
```

GitHub Actions performs the authoritative Geode 5.8.2 / GD 2.2081 Win64 compile/link and produces `.geode` + PDB artifacts.

## License / credits

FWBot is GPL-3.0. Silicate-derived/inspired architecture is documented in `CREDITS.md`. Dear ImGui is an external MIT-licensed build dependency.
