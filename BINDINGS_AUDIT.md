# Bindings audit — Geode 5.8.2 / GD 2.2081

The portable sandbox cannot download or launch the Windows Geode SDK/GD runtime, so the full DLL cannot be linked locally here. The GitHub Actions workflow performs the real Win64 Geode build.

Before packaging v0.1.1, runtime hook signatures were cross-checked against the current Silicate source that targets the same Geode/GD versions:

| Hook/API used here | Cross-check |
|---|---|
| `GJBaseGameLayer::update(float)` | current Silicate hooks `void update(float dt) override` |
| `GJBaseGameLayer::getModifiedDelta(float)` | current Silicate hooks it and returns `double` |
| `GJBaseGameLayer::handleButton(bool,int,bool)` | current Silicate uses the exact signature |
| `PlayLayer::resetLevel()` | current Silicate hooks exact signature |
| `PlayLayer::destroyPlayer(PlayerObject*,GameObject*)` | current Silicate hooks exact signature |
| `PlayLayer::levelComplete()` | current Silicate hooks exact signature |
| `PlayLayer::get()` | current Silicate uses it throughout gameplay hooks |
| Geode v5 keybind settings | official Geode settings docs use `type: keybind` and `listenForKeybindSettingPresses` |
| CI builder | official `geode-sdk/build-geode-mod@main` action |

Reference sources:

- https://git.silicate.dev/silicate/silicate/src/branch/main/src/hooks/GJBaseGameLayer.cpp
- https://git.silicate.dev/silicate/silicate/src/branch/main/src/hooks/PlayLayer.cpp
- https://docs.geode-sdk.org/mods/settings/
- https://github.com/geode-sdk/build-geode-mod

## v0.1.1 CI configure fix

The first real Win64 GitHub Actions run reached Geode 5.8.2 configuration and exposed a CMake-only issue before C++ compilation: the mod target used `target_link_libraries(FrameWindowAnalyzer PRIVATE fwa_core)`, while `setup_geode_mod()` links the same target with CMake's plain signature. CMake forbids mixing those signatures on one target. v0.1.1 changes the mod-target call to the plain form `target_link_libraries(FrameWindowAnalyzer fwa_core)`.

Geode v5 also requires C++23, so v0.1.1 sets both `CMAKE_CXX_STANDARD` and the portable core compile feature to C++23.

## What is still only CI/runtime-verifiable

UI/Cocos convenience calls (`ButtonSprite`, `CCScale9Sprite`, draw-node rendering), generated binding/link addresses, and actual Windows ABI linkage require the Geode SDK/toolchain. That is why the workflow has a dedicated Win64 build after portable tests.

If GitHub Actions reports a compile error, send the build log. The runtime code is deliberately split into small files so binding/API fixes do not touch the tested analyzer core.
