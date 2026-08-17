# Changelog

## v0.2.0 — FWBot architecture rebuild
- Rename the user-facing bot to **FWBot** while keeping the existing mod ID so the update replaces v0.1 instead of installing beside it.
- Replace the old CocosUI menu with a black Dear ImGui Win32/OpenGL3 overlay, grouped into Macro, Frame tools, Speedhack, Analysis, Visuals and Debug sections.
- Add a minimalist in-menu `FW` logo rendered directly with ImGui draw commands.
- Rebuild Frame Stepper as a physics-freeze/one-tick system that does not open Geometry Dash PauseLayer. Disabling it clears queued steps, fixed-tick accumulator and `m_extraDelta` to prevent catch-up bursts.
- Rework recording around a current Practice/gameplay anchor. Playback and analysis restore the same retained checkpoint instead of always restarting the level from the beginning.
- Add built-in speedhack with optional FMOD master-channel pitch following.
- Rebuild trajectory as predictive fake-player HOLD/RELEASE branches. Analyzer PASS/FAIL remains isolated from the visual trajectory and continues using real PlayLayer branches.
- Keep HOLD/RELEASE frame-window scanning on both sides of the recorded timing and NaNDL exports.
- Add candidate-order validation so shifted inputs cannot cross into nonsensical HOLD/RELEASE ordering and corrupt `N_i`.
- Add Dear ImGui v1.92.4 as a pinned build-time dependency and link the required Win32/OpenGL system libraries explicitly.

## v0.1.4
- Fix Geode 5.8.2 keybind listeners by removing `mutable` from the listener closure.
- Replace templated forwarding callback with a captureless-action function pointer (`void (*)()`) to guarantee const-invocability inside Geode EventV3.
- Confirm from the latest GitHub Actions log that `GameHooks.cpp` now compiles; the only failing translation unit was `src/main.cpp`.

## v0.1.3
- Fix Geode 5.8.2 keybind listener callback return type (`bool`).
- Fix `PlayLayer*` vs `GJBaseGameLayer` hook pointer comparison in fixed-step update.

## v0.1.2
- Replace `geode-sdk/build-geode-mod@main` with a self-contained Win64 workflow to avoid composite-action download failures.

## v0.1.1
- Fix mixed keyword/plain `target_link_libraries` usage with `setup_geode_mod()`.
- Update the project to C++23.
- Add regression coverage for a 4-frame window where the recorded click is the second passing frame.
