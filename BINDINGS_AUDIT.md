# Geode bindings audit

Target: Geode SDK v5.8.2, Geometry Dash 2.2081, Win64.

## Previously confirmed by real GitHub Actions
The v0.1.4 codebase reached Geode codegen/generated bindings and compiled the runtime after fixing the keybind listener contract. The following v0.1 hook signatures were accepted:

- `GJBaseGameLayer::handleButton(bool, int, bool)`
- `GJBaseGameLayer::update(float)`
- `GJBaseGameLayer::getModifiedDelta(float)`
- `PlayLayer::resetLevel()`
- `PlayLayer::destroyPlayer(PlayerObject*, GameObject*)`
- `PlayLayer::levelComplete()`
- `PlayLayer::onExit()`

The v0.2 GameLayer/PlayLayer hooks intentionally keep those already-proven signatures.

## Keybind listener contract
`listenForKeybindSettingPresses` requires a const-invocable bool callback compatible with:

```cpp
(Keybind const&, bool down, bool repeat, double timestamp) -> bool
```

`src/main.cpp` keeps the v0.1.4 `KeybindPressListener::operator()(...) const` implementation and a `static_assert` for this contract.

## New v0.2 Geode-facing surfaces
The architecture rebuild additionally uses:

- `PlayLayer::createCheckpoint()` / `loadFromCheckpoint(CheckpointObject*)`
- `GJGameState` and `EffectManagerState`
- `GJEffectManager::saveToState` / `loadFromState`
- `PlayerObject::copyAttributes`, `pushButton`, `releaseButton`, `update`, `updateRotation`
- `FMODAudioEngine::m_system` + FMOD master `ChannelGroup::setPitch`
- `CCEGLView::swapBuffers()` for the ImGui render hook

These are audited against the same Geode/Silicate API surface, but the included GitHub Actions Win64 job remains the authoritative compiler/linker check for v0.2 because the sandbox cannot run the Windows GD/Geode toolchain.

## UI/link audit
Dear ImGui uses its standard Win32 and OpenGL3 backends. CMake explicitly links `opengl32`, `user32`, `gdi32`, and `imm32` on the Win64 mod target.

## v0.2.1 findings from real Win64 build
The user's v0.2.0 GitHub Actions reached step 41/47. The build proved that Dear ImGui itself linked (`fwbot_imgui.lib`) and that `main.cpp`, `BotController.cpp`, `FileLogger.cpp`, `Speedhack.cpp`, and `TrajectoryOverlay.cpp` compiled. Two remaining compile contracts were exposed:

1. `GJEffectManager::loadFromState(EffectManagerState&)` is non-const in the generated GD 2.2081 binding. `PracticeAnchor::applySupplemental` now copies `m_effectState` into a working state before calling the binding, preserving deterministic repeated restores.
2. `Modify<..., cocos2d::CCEGLView>` requires the generated class-specific modifier header. `ImGuiOverlay.cpp` now includes `<Geode/modify/CCEGLView.hpp>`.

`tests/source_contract_tests.py` enforces both contracts in the portable CI job.
