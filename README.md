# SampleTower (DPF)

An 11-slot, RAM-loaded, MIDI-triggered sample player — a DPF (DISTRHO Plugin
Framework) port of the JUCE [SampleTower](https://github.com/LeoFabre/SampleTower)
designed to run headless on Bela / PocketBeagle2 inside Sushi, without the JUCE
runtime.

It is an **instrument**: MIDI in → stereo out, no audio input.

## Features

- 11 sample slots, fully loaded into RAM, each triggered by a MIDI note.
- Global **Speed** control (0.1–2.0×) — vinyl-style variable-rate playback via
  Catmull-Rom cubic interpolation.
- Per-slot **loop / one-shot** mode.
- **Kill-all** (stops every playing voice).
- Master **Gain** (−60…0 dB).
- **LowCut** high-pass filter: 1–4 cascaded RBJ biquad stages (12/24/36/48 dB/oct).
- Audio formats: **WAV, AIFF/AIFF-C, FLAC, MP3** (dr_libs + a small AIFF reader).
- Optional NanoVG debug UI (desktop only; off for Bela).

## MIDI map

| Note | Action            |
|------|-------------------|
| 59   | Kill all voices   |
| 60   | Trigger slot 1    |
| 61   | Trigger slot 2    |
| …    | …                 |
| 70   | Trigger slot 11   |

A note-off on a slot's note disarms its loop (the voice finishes the current
pass, then stops); it does not cut the sound immediately.

## Sample configuration

Samples are loaded at startup from `~/SampleTower/samples_config.json`. There is
no UI file picker (headless tower workflow). Schema:

```json
{
  "samples": [
    { "path": "/home/root/samples/kick.wav", "loop": true  },
    { "path": "/home/root/samples/snare.aiff", "loop": false }
  ]
}
```

The Nth entry maps to slot N (note 60 + N − 1). Entries without a `path` are
skipped; `loop` defaults to `false`. A missing or malformed config file leaves
all slots empty (no crash). The referenced sample files must exist on the device.

## Parameters

`Speed`, `Gain`, `LowCut Enabled`, `LowCut Freq` (log), `LowCut Slope` (0–3 =
12/24/36/48 dB/oct), and `Loop 1`…`Loop 11`. The loop parameters are initialised
from the JSON config and remain host-automatable.

## Build (desktop)

Headless (no UI), as used for Bela:

```bash
git submodule update --init --recursive      # pulls dpf + pugl
cmake -B build -DSAMPLETOWER_BUILD_UI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

With the desktop debug UI:

```bash
cmake -B build-ui -DSAMPLETOWER_BUILD_UI=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-ui -j
```

Outputs land in `build/bin/SampleTower.vst3` and `SampleTower.lv2`.

## Tests

Pure-C++ unit tests (no DPF/JUCE dependency) cover DSP math, the RBJ high-pass
chain, the Catmull-Rom resampler, the WAV/AIFF decoder, the JSON config loader,
the 11-voice sampler, and an offline analytic null-test (at speed = 1.0 the
output reproduces the source WAV to better than −120 dB). Run with `ctest`.

## Cross-build for Bela (aarch64)

From the parent `nexus-preamp` repo:

```bash
./cross-build-dpf-plugin.sh plugins/sampletower-dpf            # headless
```

Output: `build-arm64/sampletower-dpf/SampleTower.vst3`. The script auto-detects
the `SAMPLETOWER_BUILD_UI` CMake option.

## License

GPL-3.0-or-later.
