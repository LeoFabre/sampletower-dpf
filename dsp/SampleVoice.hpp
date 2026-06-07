#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include <utility>

namespace st {

class SampleVoice {
public:
    // buffer must have exactly 2 channels (caller duplicates mono).
    void load(std::vector<std::vector<float>> buffer, double fileSampleRate) {
        buffer_ = std::move(buffer);
        lengthFrames_ = buffer_.empty() ? 0.0 : (double) buffer_[0].size();
        fileSampleRate_ = fileSampleRate > 0 ? fileSampleRate : 48000.0;
        loaded_ = lengthFrames_ > 0.0 && buffer_.size() >= 2;
        playing_ = false;
        readPos_ = 0.0;
        recomputeIncrement();
    }

    bool isLoaded()  const noexcept { return loaded_; }
    bool isPlaying() const noexcept { return playing_; }

    void setHostSampleRate(double sr) noexcept { hostSampleRate_ = sr; recomputeIncrement(); }
    void setSpeed(float s)            noexcept { speed_ = s; recomputeIncrement(); }
    void setLooping(bool l)           noexcept { looping_ = l; }
    void disarmLoop()                 noexcept { loopArmed_ = false; }

    void start() noexcept {
        if (!loaded_) return;
        readPos_ = 0.0; playing_ = true; loopArmed_ = looping_;
    }
    void stopImmediately() noexcept { playing_ = false; readPos_ = 0.0; }

    inline void render(float* outL, float* outR, uint32_t frames) noexcept {
        if (!playing_ || !loaded_) return;
        for (uint32_t f = 0; f < frames; ++f) {
            outL[f] += interp(0, readPos_);
            outR[f] += interp(1, readPos_);
            readPos_ += increment_;
            if (readPos_ >= lengthFrames_) {
                if (loopArmed_) {
                    do { readPos_ -= lengthFrames_; } while (readPos_ >= lengthFrames_);
                } else {
                    playing_ = false;
                    return;
                }
            }
        }
    }

private:
    inline float interp(int ch, double pos) const noexcept {
        const long len = (long) lengthFrames_;
        const long i   = (long) std::floor(pos);
        const float t  = (float) (pos - (double) i);
        const std::vector<float>& b = buffer_[(size_t) ch];
        auto at = [&](long idx) -> float {
            if (idx < 0)    idx = 0;
            if (idx >= len) idx = len - 1;
            return b[(size_t) idx];
        };
        const float y0 = at(i - 1), y1 = at(i), y2 = at(i + 1), y3 = at(i + 2);
        const float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
        const float c1 =  y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c2 = -0.5f * y0 + 0.5f * y2;
        return ((a * t + c1) * t + c2) * t + y1;
    }
    void recomputeIncrement() noexcept {
        increment_ = (hostSampleRate_ > 0.0)
            ? (fileSampleRate_ / hostSampleRate_) * (double) speed_
            : (double) speed_;
    }

    std::vector<std::vector<float>> buffer_;
    double lengthFrames_ = 0.0, readPos_ = 0.0, increment_ = 1.0;
    double fileSampleRate_ = 48000.0, hostSampleRate_ = 48000.0;
    float  speed_ = 1.0f;
    bool   loaded_ = false, playing_ = false, looping_ = true, loopArmed_ = false;
};

} // namespace st
