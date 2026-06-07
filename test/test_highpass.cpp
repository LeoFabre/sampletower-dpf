#include "HighPassChain.hpp"
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#define EXPECT_NEAR(a, b, eps) do {                                          \
    const double _a = double(a), _b = double(b), _e = double(eps);           \
    if (std::abs(_a - _b) > _e) {                                            \
        std::fprintf(stderr, "FAIL %s:%d: %.9f != %.9f (eps=%.6g)\n",        \
            __FILE__, __LINE__, _a, _b, _e);                                 \
        std::exit(1);                                                        \
    }                                                                        \
} while (0)
#define EXPECT_TRUE(c) do {                                                  \
    if (!(c)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
        std::exit(1); }                                                      \
} while (0)

// Drive a steady sine through the chain, return settled peak amplitude.
static float settledPeak(st::HighPassChain& hp, double sr, double freq) {
    const int warm = 8192, meas = 4096;
    float peak = 0.0f;
    double ph = 0.0, inc = 2.0 * st::kPi * freq / sr;
    std::vector<float> buf(warm + meas);
    for (int i = 0; i < warm + meas; ++i) { buf[i] = std::sin(ph); ph += inc; }
    hp.process(buf.data(), (uint32_t)buf.size());
    for (int i = warm; i < warm + meas; ++i) peak = std::max(peak, std::abs(buf[i]));
    return peak;
}

int main() {
    const double sr = 48000.0;

    // 1 stage @ 100 Hz: passband (2 kHz) ~ unity, deep sub-cutoff (20 Hz) attenuated.
    { st::HighPassChain hp; hp.setParams(sr, 100.0f, 0);
      EXPECT_NEAR(settledPeak(hp, sr, 2000.0), 1.0f, 0.05f);
      st::HighPassChain hp2; hp2.setParams(sr, 100.0f, 0);
      EXPECT_TRUE(settledPeak(hp2, sr, 20.0) < 0.2f); }

    // Steeper slope attenuates more at a fixed sub-cutoff frequency.
    { st::HighPassChain a; a.setParams(sr, 200.0f, 0); // 12 dB/oct
      st::HighPassChain d; d.setParams(sr, 200.0f, 3); // 48 dB/oct
      EXPECT_TRUE(settledPeak(d, sr, 50.0) < settledPeak(a, sr, 50.0)); }

    // setParams re-applied with a different freq must recompute (gotcha #5):
    // after raising cutoff far above the test tone, attenuation increases.
    { st::HighPassChain hp; hp.setParams(sr, 100.0f, 0);
      const float lowCut = settledPeak(hp, sr, 300.0);
      st::HighPassChain hp2; hp2.setParams(sr, 800.0f, 0);
      const float highCut = settledPeak(hp2, sr, 300.0);
      EXPECT_TRUE(highCut < lowCut); }

    std::printf("test_highpass OK\n");
    return 0;
}
