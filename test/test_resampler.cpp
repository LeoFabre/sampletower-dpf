#include "SampleVoice.hpp"
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

static st::SampleVoice makeVoice(const std::vector<float>& mono, double fileSR) {
    std::vector<std::vector<float>> ch(2, mono); // duplicate to L/R
    st::SampleVoice v;
    v.load(std::move(ch), fileSR);
    v.setHostSampleRate(fileSR);
    return v;
}

int main() {
    // Identity at speed=1, fileSR==hostSR: output == input sample-for-sample.
    {
        std::vector<float> src(64);
        for (int i = 0; i < 64; ++i) src[i] = std::sin(0.13f * i) * 0.7f;
        st::SampleVoice v = makeVoice(src, 48000.0);
        v.setSpeed(1.0f);
        v.start();
        std::vector<float> L(64, 0.0f), R(64, 0.0f);
        v.render(L.data(), R.data(), 64);
        for (int i = 0; i < 64; ++i) EXPECT_NEAR(L[i], src[i], 1e-6);
        for (int i = 0; i < 64; ++i) EXPECT_NEAR(R[i], src[i], 1e-6);
    }
    // DC preserved at non-integer ratio.
    {
        std::vector<float> src(128, 0.5f);
        st::SampleVoice v = makeVoice(src, 48000.0);
        v.setSpeed(0.7f);
        v.start();
        std::vector<float> L(40, 0.0f), R(40, 0.0f);
        v.render(L.data(), R.data(), 40);
        for (int i = 1; i < 39; ++i) EXPECT_NEAR(L[i], 0.5f, 1e-4);
    }
    // One-shot stops at end; loop wraps and keeps playing.
    {
        std::vector<float> src(16, 1.0f);
        st::SampleVoice v = makeVoice(src, 48000.0);
        v.setLooping(false); v.setSpeed(1.0f); v.start();
        std::vector<float> L(32, 0.0f), R(32, 0.0f);
        v.render(L.data(), R.data(), 32);
        EXPECT_TRUE(!v.isPlaying());           // stopped after 16 samples
        EXPECT_NEAR(L[20], 0.0f, 1e-9);        // silence past the end

        st::SampleVoice w = makeVoice(src, 48000.0);
        w.setLooping(true); w.setSpeed(1.0f); w.start();
        std::vector<float> L2(40, 0.0f), R2(40, 0.0f);
        w.render(L2.data(), R2.data(), 40);
        EXPECT_TRUE(w.isPlaying());            // still looping
        EXPECT_NEAR(L2[20], 1.0f, 1e-6);       // wrapped content present
    }
    // disarmLoop: a looping voice finishes its current pass then stops.
    {
        std::vector<float> src(16, 1.0f);
        st::SampleVoice v = makeVoice(src, 48000.0);
        v.setLooping(true); v.setSpeed(1.0f); v.start();
        v.disarmLoop();
        std::vector<float> L(32, 0.0f), R(32, 0.0f);
        v.render(L.data(), R.data(), 32);
        EXPECT_TRUE(!v.isPlaying());
    }
    std::printf("test_resampler OK\n");
    return 0;
}
