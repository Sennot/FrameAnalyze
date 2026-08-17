# Changelog

## v0.1.1

- Fixed Geode CMake configure failure caused by mixing keyword and plain `target_link_libraries` signatures on `FrameWindowAnalyzer`.
- Raised the project language level to C++23, as required by Geode v5.
- Added a regression test for a four-frame timing when the recorded click is on the second valid frame.

## v0.1.0

- Initial analyzer prototype.
