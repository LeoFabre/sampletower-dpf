#pragma once
#include <string>
#include <vector>

namespace st {

struct DecodedAudio {
    std::vector<std::vector<float>> channels; // exactly 2 when ok
    double sampleRate = 0.0;
    bool   ok = false;
    std::string error;
};

// Decode a WAV/AIFF/FLAC/MP3 file into 2-channel float. Never throws.
DecodedAudio decodeAudioFile(const std::string& path);

} // namespace st
