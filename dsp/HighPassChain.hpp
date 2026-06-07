#pragma once
#include "DspMath.hpp"
#include <cmath>
#include <cstdint>

namespace st {

struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float s1 = 0, s2 = 0;
    bool bypassed = true;

    inline float process(float x) noexcept {
        if (bypassed) return x;
        const float y = b0 * x + s1;     // Transposed Direct Form II
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        return y;
    }
    void reset() noexcept { s1 = s2 = 0.0f; }

    // RBJ cookbook high-pass, Q = 1/sqrt(2), normalized by a0. Matches JUCE makeHighPass.
    void setHighPass(double sr, double fc) noexcept {
        const double w0    = 2.0 * kPi * fc / sr;
        const double cw    = std::cos(w0);
        const double sw    = std::sin(w0);
        const double Q     = 0.70710678118654752440;
        const double alpha = sw / (2.0 * Q);
        const double a0    = 1.0 + alpha;
        b0 = float(((1.0 + cw) * 0.5) / a0);
        b1 = float((-(1.0 + cw)) / a0);
        b2 = float(((1.0 + cw) * 0.5) / a0);
        a1 = float((-2.0 * cw) / a0);
        a2 = float((1.0 - alpha) / a0);
        bypassed = false;
    }
};

// Up to 4 cascaded high-pass biquads for a SINGLE channel.
class HighPassChain {
public:
    static constexpr int kMaxStages = 4;

    // slopeIdx 0..3 -> 1..4 active stages (12/24/36/48 dB/oct). Always recomputes coeffs.
    void setParams(double sampleRate, float freqHz, int slopeIdx) noexcept {
        const double fc = (double) clamp(freqHz, 20.0f, 1000.0f);
        const int active = clamp(slopeIdx, 0, 3) + 1;
        for (int i = 0; i < kMaxStages; ++i) {
            if (i < active) stages_[i].setHighPass(sampleRate, fc);
            else            stages_[i].bypassed = true;
        }
    }
    inline void process(float* data, uint32_t n) noexcept {
        for (uint32_t i = 0; i < n; ++i) {
            float x = data[i];
            for (int s = 0; s < kMaxStages; ++s) x = stages_[s].process(x);
            data[i] = x;
        }
    }
    void reset() noexcept { for (auto& s : stages_) s.reset(); }

private:
    Biquad stages_[kMaxStages];
};

} // namespace st
