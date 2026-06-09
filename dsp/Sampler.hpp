#pragma once
#include "SampleVoice.hpp"
#include <array>
#include <vector>
#include <cstdint>

namespace st {

class Sampler {
public:
    static constexpr int kNumSlots = 11;
    static constexpr int kBaseNote = 60;
    static constexpr int kKillNote = 59;

    void setHostSampleRate(double sr) noexcept { for (auto& v : voices_) v.setHostSampleRate(sr); }
    void setSpeedAll(float s)         noexcept { for (auto& v : voices_) v.setSpeed(s); }
    void setLoopingForSlot(int i, bool l) noexcept {
        if (i >= 0 && i < kNumSlots) voices_[(size_t) i].setLooping(l);
    }
    void loadSlot(int i, std::vector<std::vector<float>> buf, double fileSR) {
        if (i >= 0 && i < kNumSlots) voices_[(size_t) i].load(std::move(buf), fileSR);
    }

    void noteOn(int note) noexcept {
        if (note == kKillNote) { killAll(); return; }
        const int i = note - kBaseNote;
        if (i >= 0 && i < kNumSlots) voices_[(size_t) i].start();
    }
    void noteOff(int note) noexcept {
        const int i = note - kBaseNote;
        if (i >= 0 && i < kNumSlots) voices_[(size_t) i].disarmLoop();
    }
    void killAll() noexcept { for (auto& v : voices_) v.stopImmediately(); }

    inline void render(float* __restrict L, float* __restrict R, uint32_t n) noexcept {
        for (auto& v : voices_) v.render(L, R, n);
    }

    SampleVoice& voice(int i) noexcept { return voices_[(size_t) i]; }

private:
    std::array<SampleVoice, kNumSlots> voices_;
};

} // namespace st
