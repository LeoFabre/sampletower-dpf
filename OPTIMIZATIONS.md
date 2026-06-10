# Optimization notes

This document describes the CPU optimization work done on the SampleTower DSP for
the production setup on Bela (PocketBeagle2, Cortex-A53, aarch64). Builds use
`-O3 -march=armv8-a -mtune=cortex-a53`. All optimizations were validated with an
offline A/B regression gate (the same audio rendered through two git refs, float
dumps compared in dBFS), unit tests, and real-device benchmarks on the Cortex-A53.

**Bottom line: 277 → 175 ns/sample on the Cortex-A53, a 1.58× speedup**, from
Tiers 1–3 alone. SIMD vectorization was evaluated and rejected (see below).

All of the work described below is merged on `main`.

## What was optimized

### Tier 1 — flush-to-zero denormals (`688ab8a`)

`dsp/DenormalGuard.hpp` arms FPCR.FZ (flush-to-zero) once on the audio thread at
the top of `Plugin::run()`. aarch64-only, no-op on host builds. Protects the
LowCut biquad cascade tails from the Cortex-A53 denormal penalty.

### Tier 2 — numerically-equivalent per-sample wins (`688ab8a`)

Safe rewrites (same math, cheaper form): `pow(x, 2)` → `x*x`,
reciprocal-multiply instead of per-sample divides, hoisted loop-invariant
coefficients, `__restrict` on hot buffers. Validated bit-equivalent (or below
−80 dB residual) by the offline gate; all DSP tests stay green.

### Tier 3 — fast-math on the DSP objects only (`15fb4ea`)

`-funsafe-math-optimizations` applied to the SampleTower DSP compilation units
only (not the whole plugin). Gated by the same A/B comparison.

## What was tried and rejected

### NEON SIMD vectorization — rejected

This is a deliberately documented negative result.

SampleTower's hot loop is 11 independent voices, each reading from a **different**
sample buffer at a **fractional, per-voice playback position** (Catmull-Rom
interpolation at variable rate). Vectorizing across voices would require gather
loads — reading 4 unrelated memory locations into one vector register — and the
Cortex-A53's NEON has no gather instruction. Emulating gathers with scalar loads
plus lane inserts costs more than it saves, and vectorizing *within* a voice does
not help either (the interpolation taps are sequential and cheap; the work is
memory-bound, not ALU-bound).

So the plugin stays scalar, and at 175 ns/sample it is by far the cheapest plugin
in the production chain anyway.

## Validation

- **Offline A/B regression gate**: identical input rendered through two git refs,
  float output dumps compared in dBFS. Thresholds are calibrated: bit-identical
  reads as ≈ −999 dB, sub-audible drift ≈ −85 dB, a real regression shows up
  around −14 dB.
- **Unit tests** (CTest) at every step.
- **Real-device benchmarks**: ns/sample measured on the Cortex-A53 itself.
