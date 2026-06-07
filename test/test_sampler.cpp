#include "Sampler.hpp"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>
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

static void loadConst(st::Sampler& s, int slot, float val, int n, double fileSR=48000.0){
    std::vector<std::vector<float>> buf(2, std::vector<float>((size_t)n, val));
    s.loadSlot(slot, std::move(buf), fileSR);
}

int main(){
    st::Sampler s;
    s.setHostSampleRate(48000.0);
    loadConst(s, 0, 1.0f, 32);
    loadConst(s, 1, 0.25f, 32);
    s.setSpeedAll(1.0f);

    // Note 60 triggers slot 0 only.
    s.noteOn(60);
    EXPECT_TRUE(s.voice(0).isPlaying());
    EXPECT_TRUE(!s.voice(1).isPlaying());
    std::vector<float> L(16,0.f), R(16,0.f);
    s.render(L.data(), R.data(), 16);
    EXPECT_NEAR(L[5], 1.0f, 1e-6);
    EXPECT_NEAR(R[5], 1.0f, 1e-6);

    // Adding slot 1 sums additively.
    s.noteOn(61);
    std::vector<float> L2(16,0.f), R2(16,0.f);
    s.render(L2.data(), R2.data(), 16);
    EXPECT_NEAR(L2[3], 1.25f, 1e-6);

    // Note 59 kills everything.
    s.noteOn(59);
    EXPECT_TRUE(!s.voice(0).isPlaying());
    EXPECT_TRUE(!s.voice(1).isPlaying());

    // Out-of-range notes are ignored (no crash).
    s.noteOn(127);
    s.noteOn(0);

    std::printf("test_sampler OK\n");
    return 0;
}
