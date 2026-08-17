# Changelog

## v0.1.3
- Fix Geode 5.8.2 keybind listener callback return type (`bool`).
- Fix `PlayLayer*` vs `GJBaseGameLayer` hook pointer comparison in fixed-step update.
- Keep all v0.1.2 CI/CMake fixes.

## v0.1.2
- Replaced `geode-sdk/build-geode-mod@main` with a self-contained Win64 workflow.
- Geode CLI v3.8.0 is downloaded through authenticated `gh release` requests using `GITHUB_TOKEN`.
- Geode SDK is installed and pinned to v5.8.2 through the Geode CLI, then Win64 binaries are installed explicitly.
- CMake/Ninja configuration and `.geode`/PDB collection are now explicit workflow steps.
- This avoids the specific GitHub `codeload` 429 failure that prevented the previous workflow from starting.

## v0.1.1
- Fixed mixed keyword/plain `target_link_libraries` usage with `setup_geode_mod()`.
- Updated the project to C++23 for Geode v5.
- Added regression coverage for a 4-frame window where the recorded click is the second passing frame.
