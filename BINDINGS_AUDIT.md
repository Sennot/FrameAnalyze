# Geode bindings audit

Target: Geode SDK v5.8.2, Geometry Dash 2.2081, Win64.

## Confirmed by GitHub Actions compile logs

### KeybindSettingPressedEventV3
`listenForKeybindSettingPresses` invokes listeners with:

```cpp
(Keybind const&, bool down, bool repeat, double timestamp) -> bool
```

The v0.1.2 listener accepted the four parameters but returned `void`, which failed template substitution in `Event.hpp` / `SettingV3.hpp`. v0.1.3 explicitly returns `bool` and returns `false` after processing the press so the event is not consumed.

### GJBaseGameLayer update hook
The fixed-step hook modifies `GJBaseGameLayer::update(float)`. `PlayLayer::get()` returns `PlayLayer*`, while `this` inside the modify class is `FWAGameLayerHook*`. v0.1.2 compared those distinct pointer types directly. v0.1.3 converts both to their common `GJBaseGameLayer*` base before comparing.

### Previously confirmed by compilation
The Win64 build reached and compiled Geode generated bindings and began compiling all FWA translation units. Signatures for the following hooks were accepted before the build stopped on the two errors above:

- `GJBaseGameLayer::handleButton(bool, int, bool)`
- `GJBaseGameLayer::update(float)`
- `GJBaseGameLayer::getModifiedDelta(float)`
- `PlayLayer::resetLevel()`
- `PlayLayer::destroyPlayer(PlayerObject*, GameObject*)`
- `PlayLayer::levelComplete()`

The next GitHub Actions run is still the authoritative full Win64 binding/link test.
