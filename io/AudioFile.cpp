#include "AudioFile.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace st {

// ---- helpers ------------------------------------------------------------

static std::string lowerExt(const std::string& path) {
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string e = path.substr(dot + 1);
    for (char& c : e) c = (char) std::tolower((unsigned char) c);
    return e;
}

// Deinterleave `frames*channels` floats into exactly-2-channel output.
static void toStereo(DecodedAudio& out, const float* inter, uint64_t frames, uint32_t ch) {
    out.channels.assign(2, std::vector<float>((size_t) frames, 0.0f));
    if (ch == 1) {
        for (uint64_t i = 0; i < frames; ++i) {
            const float v = inter[i];
            out.channels[0][i] = v;
            out.channels[1][i] = v;
        }
    } else {
        for (uint64_t i = 0; i < frames; ++i) {
            out.channels[0][i] = inter[i * ch + 0];
            out.channels[1][i] = inter[i * ch + 1];
        }
    }
}

// ---- AIFF / AIFF-C reader (maison) -------------------------------------

static uint32_t rdBE32(const uint8_t* p) { return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]; }
static uint16_t rdBE16(const uint8_t* p) { return (uint16_t)((p[0]<<8)|p[1]); }

static double readExtended80(const uint8_t* p) {
    uint16_t expon = rdBE16(p);
    uint64_t mant  = ((uint64_t) rdBE32(p + 2) << 32) | rdBE32(p + 6);
    const int sign = (expon & 0x8000) ? -1 : 1;
    expon &= 0x7FFF;
    if (expon == 0 && mant == 0) return 0.0;
    const double f = std::ldexp((double) mant, expon - 16383 - 63);
    return sign * f;
}

static bool decodeAiff(const std::string& path, DecodedAudio& out) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { out.error = "cannot open"; return false; }
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz < 12) { std::fclose(f); out.error = "too small"; return false; }
    std::vector<uint8_t> d((size_t) sz);
    if (std::fread(d.data(), 1, (size_t) sz, f) != (size_t) sz) { std::fclose(f); out.error="read"; return false; }
    std::fclose(f);

    if (std::memcmp(d.data(), "FORM", 4) != 0) { out.error = "not FORM"; return false; }
    const bool isAifc = std::memcmp(d.data() + 8, "AIFC", 4) == 0;
    if (!isAifc && std::memcmp(d.data() + 8, "AIFF", 4) != 0) { out.error = "not AIFF"; return false; }

    uint32_t numCh = 0, numFrames = 0, bits = 0;
    double sr = 0.0;
    bool littleEndian = false, isFloat = false;
    const uint8_t* ssnd = nullptr;
    uint32_t ssndBytes = 0, ssndOffset = 0;

    size_t pos = 12;
    while (pos + 8 <= (size_t) sz) {
        const uint8_t* c = d.data() + pos;
        uint32_t clen = rdBE32(c + 4);
        const uint8_t* body = c + 8;
        if (std::memcmp(c, "COMM", 4) == 0) {
            numCh     = rdBE16(body);
            numFrames = rdBE32(body + 2);
            bits      = rdBE16(body + 6);
            sr        = readExtended80(body + 8);
            if (isAifc && clen >= 22) {
                const uint8_t* comp = body + 18; // 4-char compression id
                if (std::memcmp(comp, "sowt", 4) == 0) littleEndian = true;
                else if (std::memcmp(comp, "fl32", 4) == 0 || std::memcmp(comp, "FL32", 4) == 0) isFloat = true;
                else if (std::memcmp(comp, "NONE", 4) != 0) { out.error = "unsupported AIFC compression"; return false; }
            }
        } else if (std::memcmp(c, "SSND", 4) == 0) {
            ssndOffset = rdBE32(body);
            ssnd       = body + 8 + ssndOffset;
            ssndBytes  = (clen >= 8u + ssndOffset) ? clen - 8u - ssndOffset : 0;
        }
        pos += 8 + clen + (clen & 1); // chunks are word-aligned
    }
    if (numCh == 0 || sr <= 0.0 || !ssnd) { out.error = "missing COMM/SSND"; return false; }

    const uint32_t bytesPer = bits / 8;
    if (bytesPer == 0) { out.error = "unsupported bit depth"; return false; }
    const uint64_t maxFrames = bytesPer && numCh ? ssndBytes / (bytesPer * numCh) : 0;
    if (numFrames == 0 || numFrames > maxFrames) numFrames = (uint32_t) maxFrames;

    // Verify the SSND data region lies fully within the file buffer (corrupt files must fail cleanly).
    const uint64_t needed = (uint64_t) numFrames * numCh * bytesPer;
    if (ssnd < d.data() || ssnd + needed > d.data() + (size_t) sz) {
        out.error = "SSND out of bounds";
        return false;
    }

    std::vector<float> inter((size_t) numFrames * numCh, 0.0f);
    for (uint64_t i = 0; i < (uint64_t) numFrames * numCh; ++i) {
        const uint8_t* s = ssnd + i * bytesPer;
        float v = 0.0f;
        if (isFloat && bits == 32) {
            uint32_t u = littleEndian ? (s[0]|(s[1]<<8)|(s[2]<<16)|(s[3]<<24)) : rdBE32(s);
            std::memcpy(&v, &u, 4);
        } else if (bits == 16) {
            int16_t x = littleEndian ? (int16_t)(s[0]|(s[1]<<8)) : (int16_t) rdBE16(s);
            v = x / 32768.0f;
        } else if (bits == 24) {
            int32_t x = (s[0]<<16)|(s[1]<<8)|s[2];
            if (x & 0x800000) x |= ~0xFFFFFF;
            v = x / 8388608.0f;
        } else if (bits == 32) {
            int32_t x = littleEndian ? (int32_t)(s[0]|(s[1]<<8)|(s[2]<<16)|(s[3]<<24)) : (int32_t) rdBE32(s);
            v = x / 2147483648.0f;
        } else if (bits == 8) {
            v = (int8_t) s[0] / 128.0f;
        } else { out.error = "unsupported bit depth"; return false; }
        inter[(size_t) i] = v;
    }
    out.sampleRate = sr;
    toStereo(out, inter.data(), numFrames, numCh);
    out.ok = true;
    return true;
}

// ---- dispatch -----------------------------------------------------------

DecodedAudio decodeAudioFile(const std::string& path) {
    DecodedAudio out;
    const std::string ext = lowerExt(path);

    if (ext == "wav" || ext == "w64") {
        unsigned int ch = 0; unsigned int sr = 0; drwav_uint64 frames = 0;
        float* data = drwav_open_file_and_read_pcm_frames_f32(path.c_str(), &ch, &sr, &frames, nullptr);
        if (!data) { out.error = "dr_wav failed"; return out; }
        out.sampleRate = sr;
        toStereo(out, data, frames, ch);
        drwav_free(data, nullptr);
        out.ok = true;
        return out;
    }
    if (ext == "flac") {
        unsigned int ch = 0; unsigned int sr = 0; drflac_uint64 frames = 0;
        float* data = drflac_open_file_and_read_pcm_frames_f32(path.c_str(), &ch, &sr, &frames, nullptr);
        if (!data) { out.error = "dr_flac failed"; return out; }
        out.sampleRate = sr;
        toStereo(out, data, frames, ch);
        drflac_free(data, nullptr);
        out.ok = true;
        return out;
    }
    if (ext == "mp3") {
        drmp3_config cfg{};
        drmp3_uint64 frames = 0;
        float* data = drmp3_open_file_and_read_pcm_frames_f32(path.c_str(), &cfg, &frames, nullptr);
        if (!data) { out.error = "dr_mp3 failed"; return out; }
        out.sampleRate = cfg.sampleRate;
        toStereo(out, data, frames, cfg.channels);
        drmp3_free(data, nullptr);
        out.ok = true;
        return out;
    }
    if (ext == "aif" || ext == "aiff" || ext == "aifc") {
        decodeAiff(path, out);
        return out;
    }
    out.error = "unsupported extension: " + ext;
    return out;
}

} // namespace st
