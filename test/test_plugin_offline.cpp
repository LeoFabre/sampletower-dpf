#include "Sampler.hpp"
#include "AudioFile.hpp"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <cmath>
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

static void w16(FILE* f, uint16_t v){ fputc(v&0xFF,f); fputc((v>>8)&0xFF,f); }
static void w32(FILE* f, uint32_t v){ for(int i=0;i<4;++i) fputc((v>>(8*i))&0xFF,f); }
static void writeWav(const std::string& p, const std::vector<int16_t>& s, uint32_t sr){
    FILE* f=fopen(p.c_str(),"wb"); const uint32_t db=(uint32_t)s.size()*2;
    fwrite("RIFF",1,4,f); w32(f,36+db); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); w32(f,16); w16(f,1); w16(f,1); w32(f,sr); w32(f,sr*2); w16(f,2); w16(f,16);
    fwrite("data",1,4,f); w32(f,db); for(int16_t v:s) w16(f,(uint16_t)v); fclose(f);
}

int main(){
    const uint32_t sr = 48000, N = 512;
    std::vector<int16_t> s(N);
    for (uint32_t i = 0; i < N; ++i) s[i] = (int16_t) std::lround(20000.0 * std::sin(0.05 * i));
    writeWav("/tmp/st_null.wav", s, sr);

    st::DecodedAudio dec = st::decodeAudioFile("/tmp/st_null.wav");
    EXPECT_TRUE(dec.ok);

    st::Sampler sampler;
    sampler.setHostSampleRate((double) sr);
    sampler.loadSlot(0, dec.channels, dec.sampleRate); // copy; keep dec for reference
    sampler.setSpeedAll(1.0f);
    sampler.setLoopingForSlot(0, false);
    sampler.noteOn(60);

    std::vector<float> L(N, 0.f), R(N, 0.f);
    sampler.render(L.data(), R.data(), N);

    double peak = 0.0;
    for (uint32_t i = 0; i < N; ++i)
        peak = std::max(peak, (double) std::abs(L[i] - dec.channels[0][i]));
    const double peakDb = 20.0 * std::log10(peak + 1e-30);
    std::printf("null-test peak residual: %.2f dB\n", peakDb);
    EXPECT_TRUE(peakDb <= -120.0);

    std::printf("test_plugin_offline OK\n");
    return 0;
}
