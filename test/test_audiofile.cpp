#include "AudioFile.hpp"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
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

static void w16(FILE* f, uint16_t v){ fputc(v & 0xFF, f); fputc((v>>8)&0xFF, f); }
static void w32(FILE* f, uint32_t v){ for(int i=0;i<4;++i) fputc((v>>(8*i))&0xFF, f); }
static void b16(FILE* f, int16_t v){ fputc((v>>8)&0xFF, f); fputc(v&0xFF, f); } // big-endian

// Minimal 16-bit PCM mono WAV writer.
static void writeWav(const std::string& p, const std::vector<int16_t>& s, uint32_t sr){
    FILE* f = fopen(p.c_str(), "wb");
    const uint32_t dataBytes = (uint32_t)s.size()*2;
    fwrite("RIFF",1,4,f); w32(f, 36+dataBytes); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); w32(f,16); w16(f,1); w16(f,1); w32(f,sr);
    w32(f, sr*2); w16(f,2); w16(f,16);
    fwrite("data",1,4,f); w32(f,dataBytes);
    for(int16_t v : s) w16(f,(uint16_t)v);
    fclose(f);
}

// Minimal 16-bit PCM mono AIFF writer (big-endian, NONE/uncompressed).
static void writeAiff(const std::string& p, const std::vector<int16_t>& s, uint32_t sr){
    FILE* f = fopen(p.c_str(), "wb");
    const uint32_t n = (uint32_t)s.size();
    const uint32_t ssndBytes = 8 + n*2;       // offset+blocksize + samples
    const uint32_t commBytes = 18;
    const uint32_t formBytes = 4 + (8+commBytes) + (8+ssndBytes);
    auto W32BE=[&](uint32_t v){ for(int i=3;i>=0;--i) fputc((v>>(8*i))&0xFF,f); };
    fwrite("FORM",1,4,f); W32BE(formBytes); fwrite("AIFF",1,4,f);
    fwrite("COMM",1,4,f); W32BE(commBytes);
    b16(f,1);                                  // numChannels
    W32BE(n);                                  // numSampleFrames
    b16(f,16);                                 // bits
    // 80-bit IEEE extended sample rate (sr). exponent + 64-bit mantissa.
    { double v = (double)sr; uint16_t e=16398; uint64_t m=(uint64_t)1<<63;
      // normalize: find exponent so mantissa MSB set
      int exp=0; double mant=v; while(mant>=2.0){mant/=2.0;++exp;} while(mant<1.0&&mant>0){mant*=2.0;--exp;}
      e = (uint16_t)(16383+exp); m=(uint64_t)(mant*9223372036854775808.0); // 2^63
      fputc((e>>8)&0xFF,f); fputc(e&0xFF,f);
      for(int i=7;i>=0;--i) fputc((m>>(8*i))&0xFF,f); }
    fwrite("SSND",1,4,f); W32BE(ssndBytes); W32BE(0); W32BE(0);
    for(int16_t v : s) b16(f,v);
    fclose(f);
}

// Deliberately corrupt AIFF: valid headers but an absurd SSND offset that, if
// trusted, would drive an out-of-bounds read in the sample-conversion loop.
static void writeCorruptAiff(const std::string& p, const std::vector<int16_t>& s, uint32_t sr){
    FILE* f = fopen(p.c_str(), "wb");
    const uint32_t n = (uint32_t)s.size();
    const uint32_t ssndBytes = 8 + n*2;
    const uint32_t commBytes = 18;
    const uint32_t formBytes = 4 + (8+commBytes) + (8+ssndBytes);
    auto W32BE=[&](uint32_t v){ for(int i=3;i>=0;--i) fputc((v>>(8*i))&0xFF,f); };
    fwrite("FORM",1,4,f); W32BE(formBytes); fwrite("AIFF",1,4,f);
    fwrite("COMM",1,4,f); W32BE(commBytes);
    b16(f,1);                                  // numChannels
    W32BE(n);                                  // numSampleFrames
    b16(f,16);                                 // bits
    { double v = (double)sr;
      int exp=0; double mant=v; while(mant>=2.0){mant/=2.0;++exp;} while(mant<1.0&&mant>0){mant*=2.0;--exp;}
      uint16_t e = (uint16_t)(16383+exp); uint64_t m=(uint64_t)(mant*9223372036854775808.0);
      fputc((e>>8)&0xFF,f); fputc(e&0xFF,f);
      for(int i=7;i>=0;--i) fputc((m>>(8*i))&0xFF,f); }
    // SSND with an absurd offset far beyond the chunk/file size.
    fwrite("SSND",1,4,f); W32BE(ssndBytes); W32BE(0xFFFFFF00u); W32BE(0);
    for(int16_t v : s) b16(f,v);
    fclose(f);
}

int main(){
    std::vector<int16_t> s = {0, 16384, -16384, 32767, -32768, 8192, -8192, 0};

    writeWav("/tmp/st_test.wav", s, 44100);
    st::DecodedAudio w = st::decodeAudioFile("/tmp/st_test.wav");
    EXPECT_TRUE(w.ok);
    EXPECT_NEAR(w.sampleRate, 44100.0, 1.0);
    EXPECT_TRUE(w.channels.size() == 2);                    // mono duplicated
    EXPECT_TRUE(w.channels[0].size() == s.size());
    EXPECT_NEAR(w.channels[0][1], 16384.0f/32768.0f, 1e-3); // ~0.5
    EXPECT_NEAR(w.channels[1][1], 16384.0f/32768.0f, 1e-3); // duplicated to R
    EXPECT_NEAR(w.channels[0][3], 32767.0f/32768.0f, 1e-3);

    writeAiff("/tmp/st_test.aiff", s, 48000);
    st::DecodedAudio a = st::decodeAudioFile("/tmp/st_test.aiff");
    EXPECT_TRUE(a.ok);
    EXPECT_NEAR(a.sampleRate, 48000.0, 1.0);
    EXPECT_TRUE(a.channels.size() == 2);
    EXPECT_NEAR(a.channels[0][1], 16384.0f/32768.0f, 1e-3);
    EXPECT_NEAR(a.channels[0][4], -1.0f, 1e-3);

    st::DecodedAudio bad = st::decodeAudioFile("/tmp/does_not_exist.wav");
    EXPECT_TRUE(!bad.ok);                                   // clean failure, no crash

    writeCorruptAiff("/tmp/st_corrupt.aiff", s, 48000);
    st::DecodedAudio c = st::decodeAudioFile("/tmp/st_corrupt.aiff");
    EXPECT_TRUE(!c.ok);                                     // corrupt SSND fails cleanly, no OOB/crash

    std::printf("test_audiofile OK\n");
    return 0;
}
