# Frame Window Analyzer — Geode mod prototype v0.1.1

Frame Window Analyzer records a deterministic Geometry Dash input macro and automatically measures the passing frame window around every recorded **HOLD** and **RELEASE**.

## What v0.1 does

1. Start recording. The mod resets the level so frame 0 is deterministic.
2. Inputs are recorded as `frame / P1-P2 / button / HOLD-RELEASE`.
3. Playback replays those inputs at a fixed analysis TPS.
4. Analyzer changes exactly one input at a time by `-1,+1,-2,+2...` frames.
5. The modified branch is replayed with the real `PlayLayer` physics.
6. A branch passes if it survives the full macro plus `Post-macro Validation Frames`, or reaches `levelComplete()`.
7. A branch fails when `destroyPlayer()` fires.
8. The contiguous PASS island containing offset 0 becomes NaNDL `N_i`.
9. By default, stopping a non-empty recording immediately starts analysis; a clean `.log`, `_nandl.json`, and `_nandl.csv` are then written to the mod save directory under `logs/`. This can be disabled with **Analyze after recording**.
10. `logs/debug/latest.log` contains detailed branch/debug information.

This deliberately favors correctness over a fake geometry-only approximation. A future snapshot backend can accelerate the same analyzer without changing the output format.

## Frame-window meaning

For each input the report stores:

- `Early`: consecutive passing frames before the recorded input,
- `Late`: consecutive passing frames after it,
- `FRAME WINDOW (N_i)`: the size of the contiguous passing segment containing the recorded frame,
- all tested offsets,
- all passing segments when **Exhaustive Scan** is enabled.

Example: `-2 PASS, -1 PASS, 0 PASS, +1 PASS, +2 FAIL` gives `Early=2`, `Late=1`, `N_i=4`.

If a passing segment reaches the configured scan boundary, the report prints a warning so you know to increase **Frame Scan Radius**.

### Four-frame window example

If a wave timing has four valid ticks and the recorded click lands on the second valid tick, the analyzer still reports the full window. For example, `-1 PASS, 0 PASS, +1 PASS, +2 PASS` with failures outside that range gives `Early=1`, `Late=2`, `N_i=4`. The recorded frame is only the anchor used to search both directions; it is not treated as the whole window.

## NaNDL export

The calculator-facing export contains:

- Game FPS
- Window FPS
- respawn time
- Use Frame Numbers
- rows: Input number / Time / Frame window

`*_nandl.csv` uses the same three visible row columns and is the safest fallback for copying values into the calculator.

`*_nandl.json` uses readable keys (`gameFPS`, `windowFPS`, `respawnTime`, `useFrameNumbers`, `inputs`). The NaNDL page publicly documents what its JSON export contains, but not the exact serialized key schema in its rendered documentation. If its importer rejects this file, export one example JSON from NaNDL and compare/send it; the exporter is isolated in `src/core/NaNDLExport.cpp` and is trivial to match exactly.

## Menu and keybinds

Default keybinds:

| Action | Default |
|---|---|
| Open bot menu | `F8` |
| Start/stop recording | `F6` |
| Playback last macro | `F7` |
| Pause fixed-step simulation | `F4` |
| Step exactly one frame | `F5` |
| Show/hide trajectory | `Ctrl+F5` |
| Analyze last macro | `Ctrl+F7` |

All bindings are native Geode v5 **keybind settings**, so the player can change them in Geode's mod settings.

The menu is a small native Cocos/Geode overlay rather than ImGui. That avoids adding a large UI dependency only for six buttons. It exposes Record, Playback, Analyze, Pause, Frame Step, and Trajectory.

## Important v0.1 limitations

- The analyzer currently replays from level start for each candidate. It is exact with respect to the gameplay branch it runs, but not yet the high-speed Silicate-style snapshot/backstep backend.
- Recording intentionally starts from a level reset. Practice checkpoints / arbitrary mid-level start states are not serialized yet.
- The trajectory toggle draws the **actual simulated path**. Predictive fake-player trajectory is planned for the snapshot backend.
- Fast mode assumes the useful window around the recorded timing is contiguous and stops scanning a side after its first failure. Enable **Exhaustive Scan** to test every offset and detect disjoint PASS islands.
- A branch surviving until the configured post-macro horizon is counted as PASS. For macros that stop long before the consequence of an input, increase `Post-macro Validation Frames` or record farther.

## Build

### Local portable core tests (no Geode SDK)

```sh
cmake -S . -B build-core -DFWA_CORE_ONLY=ON -DFWA_BUILD_TESTS=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

### Geode build

Install the Geode CLI/SDK, then:

```sh
geode build
```

The project targets:

- Geode `5.8.2`
- Geometry Dash Windows `2.2081`

### GitHub Actions

`.github/workflows/build.yml` runs portable core tests first, then builds Win64 using the official `geode-sdk/build-geode-mod` action and uploads the `.geode`/debug output as an artifact.

## Debugging

When reporting a bad window, include:

1. `logs/debug/latest.log`
2. the clean `frame-window_*.log`
3. the corresponding `_nandl.json`
4. what input/frame you believe is wrong
5. whether Exhaustive Scan was enabled

The debug log records input number, offset, PASS/FAIL, failure frame, and branch reason. This is specifically intended to make orb/dual/slope/game-mode desyncs diagnosable.

## License / credits

GPL-3.0. See `LICENSE` and `CREDITS.md`.
