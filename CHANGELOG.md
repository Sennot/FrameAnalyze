# FWBot v0.2.3

- Fixed Record staying Idle by adopting the actual last placed Practice checkpoint instead of calling `createCheckpoint()` out of GD's placement flow.
- Record now restores the checkpoint first and only starts input capture after `loadFromCheckpoint` completes.
- Added explicit UI/status errors instead of silent Record failures.
- Supplemental Practice state is captured after checkpoint load, keeping anchor state aligned with frame 0.

# Changelog

## v0.2.2 — runtime stability rebuild
- Move Frame Stepper and speedhack timing control to `CCScheduler::update`; remove the old manual `GJBaseGameLayer::update` loop that could freeze Record/Stepper state.
- Count and inject replay inputs from the real physics-loop `processCommands` path, including accelerated multi-tick renders.
- Starting Record now clears stale stepper state and leaves scheduler gameplay live; user can enable stepper afterwards for frame-by-frame recording.
- Prevent a newly prepared analyzer branch from running on the previous branch world before the Practice anchor reset completes.
- Fix trajectory node leaks: old `CCDrawNode`/fake players are removed from parent on reset/toggle/exit instead of only dropping pointers.
- Make trajectory runtime-only and OFF on launch; remove the persisted `show-trajectory` setting that could make lines appear unexpectedly after upgrading.
- Add collision-bounded fake-player trajectory with world/effect save/restore; analyzer verdicts remain independent real gameplay branches.
- Re-apply FMOD master pitch continuously while manual speedhack audio-follow is active, so speed changes take effect immediately and survive attempt resets.
- Run automatic analysis as 1–16 exact `1/240` scheduler ticks per visual frame instead of feeding one oversized variable delta.
- Fix an analyzer/playback horizon off-by-one: the final input frame is now guaranteed to execute before PASS can be declared.
- Restore the original Practice anchor after the final analyzer branch instead of leaving the player inside the simulation.
- Rework ImGui into a compact black tabbed UI with right-side safe default placement, screen clamping and persistent draggable position.
- Keep F3–F8 FWBot hotkeys passing through while ImGui has keyboard focus, so F8 can always close the menu.
- Prevent Frame Stepper from being armed in menus or while Geometry Dash PauseLayer is active, avoiding a stale freeze on level entry/resume.
- Keep the 4-frame-gap regression: recorded second valid frame still yields `Early=1`, `Late=2`, `N_i=4`.

## v0.2.1 — Geode 5.8.2 binding fixes
- Fix PracticeAnchor restore on GD 2.2081: `GJEffectManager::loadFromState` takes a non-const `EffectManagerState&`, so FWBot now restores from a working copy while preserving the captured anchor for repeated playback/analyze branches.
- Include the required generated `<Geode/modify/CCEGLView.hpp>` header before applying the ImGui `CCEGLView::swapBuffers()` hook.
- Add `tests/source_contract_tests.py` and run it in GitHub Actions to catch missing class-specific Geode modify headers and the EffectManagerState constness contract before the Win64 build.

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
