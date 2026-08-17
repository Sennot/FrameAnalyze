# Geode bindings audit

Target: Geode SDK v5.8.2, Geometry Dash 2.2081, Win64.

## KeybindSettingPressedEventV3 — corrected in v0.1.4

Geode 5.8.2 declares `listenForKeybindSettingPresses` with a callback constraint equivalent to:

```cpp
std::is_invocable_v<Callback, Keybind const&, bool, bool, double>
```

and `KeybindSettingPressedEventV3` uses the listener signature:

```cpp
bool(Keybind const&, bool down, bool repeat, double timestamp)
```

The v0.1.3 callback returned `bool`, but was declared `mutable`. Geode's EventV3 machinery stores/captures the listener and invokes it through a const callable context. A mutable lambda has a non-const `operator()`, so `std::invoke` rejected it.

v0.1.4 removes that ambiguity entirely. It uses an explicit listener type:

```cpp
struct KeybindPressListener {
    KeyAction action;
    bool operator()(Keybind const&, bool down, bool repeat, double) const;
};
```

`src/main.cpp` also contains a `static_assert(std::is_invocable_r_v<...>)` for the exact const invocation contract.

## GJBaseGameLayer update hook — corrected in v0.1.3

The fixed-step hook modifies `GJBaseGameLayer::update(float)`. `PlayLayer::get()` returns `PlayLayer*`, while `this` inside the modify class is `FWAGameLayerHook*`. v0.1.2 compared those distinct pointer types directly. v0.1.3 converts both to their common `GJBaseGameLayer*` base before comparing.

## Confirmed by the latest GitHub Actions compile log

The latest real Win64 run reached Geode codegen/bindings and compiled these project translation units successfully:

- `src/runtime/BotController.cpp`
- `src/runtime/TrajectoryOverlay.cpp`
- `src/runtime/FileLogger.cpp`
- `src/ui/BotMenu.cpp`
- `src/hooks/GameHooks.cpp`

The run stopped only because `src/main.cpp` failed on the mutable keybind callback. Therefore the hook signatures below have already been accepted by Clang + generated Geode 5.8.2 bindings in CI:

- `GJBaseGameLayer::handleButton(bool, int, bool)`
- `GJBaseGameLayer::update(float)`
- `GJBaseGameLayer::getModifiedDelta(float)`
- `PlayLayer::resetLevel()`
- `PlayLayer::destroyPlayer(PlayerObject*, GameObject*)`
- `PlayLayer::levelComplete()`

The next GitHub Actions run is the authoritative full Win64 link/package test.
