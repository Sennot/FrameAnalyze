# Geode bindings audit — FWBot v0.2.2

Target: Geode SDK **5.8.2**, Geometry Dash **2.2081**, Win64.

## Contracts already proven by earlier real Actions builds

Earlier FWBot builds reached generated Geode bindings and compiled the established hooks after fixing callback/API issues:

- `GJBaseGameLayer::handleButton(bool, int, bool)`
- `PlayLayer::resetLevel()`
- `PlayLayer::destroyPlayer(PlayerObject*, GameObject*)`
- `PlayLayer::levelComplete()`
- `PlayLayer::onExit()`
- `GJEffectManager::loadFromState(EffectManagerState&)` non-const reference contract
- `CCEGLView` modifier requires `<Geode/modify/CCEGLView.hpp>`
- Geode keybind EventV3 callback is const-invocable and returns `bool`

## v0.2.2 timing surfaces

The old manual gameplay-update loop was removed. New timing code uses:

- `<Geode/modify/CCScheduler.hpp>` + `CCScheduler::update(float)` for freeze/step/speed control;
- `GJBaseGameLayer::processCommands(float, bool, bool)` as the per-physics replay/analyzer frame hook;
- `PlayLayer::m_extraDelta = 0` when entering/leaving step mode and before a one-step scheduler update.

`tests/source_contract_tests.py` rejects a source tree that reintroduces manual `GJBaseGameLayer::update(bot.fixedDt())` driving or omits the scheduler modifier header.

## Practice / analyzer surfaces

- `PlayLayer::createCheckpoint()` / `loadFromCheckpoint(CheckpointObject*)`
- `GJGameState`
- `EffectManagerState`
- `GJEffectManager::saveToState` / `loadFromState`
- real `PlayLayer::destroyPlayer`/`levelComplete` outcomes for analyzer verdicts

The analyzer runs exact `1/240` scheduler ticks and blocks further `processCommands` work after a branch is resolved until the Practice anchor reset is performed. This avoids contaminating the next candidate with the old branch world at accelerated analysis speed.

## Trajectory surfaces

- `PlayerObject::copyAttributes`
- `pushButton` / `releaseButton`
- `update` / `updateRotation`
- `GJBaseGameLayer::checkCollisions`
- fake-player destruction guarded from the real attempt
- explicit `removeFromParentAndCleanup(true)` cleanup for every trajectory/fake-player node

Trajectory remains a visual system only and cannot decide `N_i`.

## ImGui / Win32

- `<Geode/modify/CCEGLView.hpp>` + `CCEGLView::swapBuffers()`
- Dear ImGui Win32/OpenGL3 backends
- `opengl32`, `user32`, `gdi32`, `imm32` explicitly linked
- persistent `fwbot_imgui.ini` stored under `Mod::get()->getSaveDir()`

## CI authority

Portable tests can validate Frame Window logic and source contracts in this sandbox. The included Windows GitHub Actions job is still the authoritative full Geode compile/link test because the local environment does not contain Geometry Dash / Win64 Geode runtime libraries.
