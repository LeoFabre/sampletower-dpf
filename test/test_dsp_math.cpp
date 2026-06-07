#include "DspMath.hpp"
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

int main() {
    EXPECT_NEAR(st::dbToGain(0.0f), 1.0f, 1e-6);
    EXPECT_NEAR(st::dbToGain(-6.0206f), 0.5f, 1e-3);
    EXPECT_NEAR(st::dbToGain(-60.0f), 0.001f, 1e-5);
    EXPECT_NEAR(st::gainToDb(st::dbToGain(-12.3f)), -12.3f, 1e-3);
    EXPECT_NEAR(st::clamp(5.0f, 0.0f, 1.0f), 1.0f, 1e-9);
    EXPECT_NEAR(st::clamp(-5.0f, 0.0f, 1.0f), 0.0f, 1e-9);
    std::printf("test_dsp_math OK\n");
    return 0;
}
