# Credits and provenance

## Silicate

Frame Window Analyzer was designed after studying Silicate's public Geometry Dash bot architecture, especially its ideas around:

- deterministic replay inputs (frame / player / button / press state),
- fixed-timestep stepping and frame stepping,
- trajectory visualization,
- replay-driven simulation and state restoration concepts.

Silicate source: https://git.silicate.dev/silicate/silicate

Silicate is distributed under GNU GPL v3. Frame Window Analyzer is therefore also distributed under GPL v3 so that future code adapted from Silicate can remain license-compatible.

**This v0.1 source package is an independently written implementation; it does not vendor or copy Silicate source files verbatim.** If later versions directly adapt Silicate functions, preserve their notices and document the exact files/functions here.

## NaNDL / NaN GD

The calculator export is designed around the public NaNDL Frame Window definition and formula:

- `N_i`: number of frames/ticks available to pass input `i`
- `w_i = N_i / f`

Website: https://nandl.pages.dev/

NaNDL is credited for the public calculation model; no NaNDL site source code is included in this repository.
