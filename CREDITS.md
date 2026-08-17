# Credits

## Silicate

FWBot's bot architecture intentionally uses selected ideas and implementation patterns from Silicate rather than embedding the entire mod: deterministic fixed-tick update/frame stepping, practice/checkpoint restoration, replay-oriented HOLD/RELEASE inputs, audio speedhack behavior, and predictive fake-player trajectory structure.

Silicate source: https://git.silicate.dev/silicate/silicate
Silicate license: GPL-3.0

FWBot itself is distributed under GPL-3.0.

## Dear ImGui

FWBot v0.2 uses Dear ImGui for its Windows overlay UI and the standard Win32 + OpenGL3 backends. Dear ImGui is fetched at build time and is licensed under the MIT License.

Dear ImGui: https://github.com/ocornut/imgui
