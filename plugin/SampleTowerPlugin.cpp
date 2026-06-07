#include "SampleTowerPlugin.hpp"
#include "DspMath.hpp"
#include "AudioFile.hpp"
#include "SampleConfig.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

START_NAMESPACE_DISTRHO

SampleTowerPlugin::SampleTowerPlugin()
    : Plugin(st::kParamCount, 0, 0)
{
    Parameter tmp;
    for (uint32_t i = 0; i < st::kParamCount; ++i) {
        initParameter(i, tmp);
        paramValues_[i] = tmp.ranges.def;
    }
    loadSamplesFromConfig();
}

void SampleTowerPlugin::loadSamplesFromConfig()
{
    const auto cfg = st::loadSampleConfig(st::defaultConfigPath());
    for (size_t i = 0; i < cfg.size() && i < (size_t) st::kNumSlots; ++i) {
        st::DecodedAudio dec = st::decodeAudioFile(cfg[i].path);
        if (!dec.ok) {
#ifndef NDEBUG
            std::fprintf(stderr, "[SampleTower] slot %zu load failed: %s (%s)\n",
                         i, cfg[i].path.c_str(), dec.error.c_str());
#endif
            continue;
        }
        sampler_.loadSlot((int) i, std::move(dec.channels), dec.sampleRate);
        sampler_.setLoopingForSlot((int) i, cfg[i].loop);
        paramValues_[st::kParamLoop1 + i] = cfg[i].loop ? 1.0f : 0.0f;
    }
}

void SampleTowerPlugin::initParameter(uint32_t index, Parameter& parameter)
{
    const st::ParamRange r = st::rangeFor(index);
    parameter.ranges.min = r.min;
    parameter.ranges.max = r.max;
    parameter.ranges.def = r.def;
    parameter.hints = kParameterIsAutomatable;
    if (r.isBool) parameter.hints |= kParameterIsBoolean;
    if (r.isLog)  parameter.hints |= kParameterIsLogarithmic;
    if (r.isInt)  parameter.hints |= kParameterIsInteger;

    char nm[32], sym[16];
    switch (index) {
        case st::kParamSpeed:         parameter.name="Speed";         parameter.symbol="speed";    parameter.unit="x";  return;
        case st::kParamGain:          parameter.name="Gain";          parameter.symbol="gain";     parameter.unit="dB"; return;
        case st::kParamLowCutEnabled: parameter.name="LowCut Enabled";parameter.symbol="lcEnabled";parameter.unit="";   return;
        case st::kParamLowCutFreq:    parameter.name="LowCut Freq";   parameter.symbol="lcFreq";   parameter.unit="Hz"; return;
        case st::kParamLowCutSlope:   parameter.name="LowCut Slope";  parameter.symbol="lcSlope";  parameter.unit="";   return;
        default: {
            const uint32_t slot = index - st::kParamLoop1 + 1;
            std::snprintf(nm,  sizeof nm,  "Loop %u", slot);
            std::snprintf(sym, sizeof sym, "loop%u", slot);
            parameter.name = nm; parameter.symbol = sym; parameter.unit = "";
            return;
        }
    }
}

float SampleTowerPlugin::getParameterValue(uint32_t index) const
{
    if (index >= st::kParamCount) return 0.0f;
    return paramValues_[index];
}

void SampleTowerPlugin::setParameterValue(uint32_t index, float value)
{
    if (index >= st::kParamCount) return;
    paramValues_[index] = value;
    pushParamsToDsp();
}

void SampleTowerPlugin::activate()
{
    sampleRate_ = getSampleRate();
    sampler_.setHostSampleRate(sampleRate_);
    hpL_.reset();
    hpR_.reset();
    pushParamsToDsp();
}

void SampleTowerPlugin::pushParamsToDsp()
{
    sampler_.setSpeedAll(paramValues_[st::kParamSpeed]);
    for (int i = 0; i < st::kNumSlots; ++i)
        sampler_.setLoopingForSlot(i, paramValues_[st::kParamLoop1 + i] > 0.5f);

    lowCutEnabled_ = paramValues_[st::kParamLowCutEnabled] > 0.5f;
    if (sampleRate_ > 0.0) {
        const float freq = paramValues_[st::kParamLowCutFreq];
        const int slope = (int) (paramValues_[st::kParamLowCutSlope] + 0.5f);
        hpL_.setParams(sampleRate_, freq, slope);
        hpR_.setParams(sampleRate_, freq, slope);
    }
}

void SampleTowerPlugin::handleMidi(const MidiEvent& ev)
{
    if (ev.size < 2) return;
    const uint8_t status = ev.data[0] & 0xF0;
    const uint8_t note   = ev.data[1];
    const uint8_t vel    = ev.size > 2 ? ev.data[2] : 0;
    const bool noteOn  = (status == 0x90 && vel > 0);
    const bool noteOff = (status == 0x80) || (status == 0x90 && vel == 0);
    if (noteOn)       sampler_.noteOn(note);
    else if (noteOff) sampler_.noteOff(note);
}

void SampleTowerPlugin::run(const float**, float** outputs, uint32_t frames,
                            const MidiEvent* midiEvents, uint32_t midiEventCount)
{
    float* outL = outputs[0];
    float* outR = outputs[1];
    std::memset(outL, 0, frames * sizeof(float));
    std::memset(outR, 0, frames * sizeof(float));

    // Sample-accurate MIDI dispatch: render sub-blocks between events.
    uint32_t cursor = 0;
    for (uint32_t e = 0; e < midiEventCount; ++e) {
        const uint32_t evFrame = std::min(midiEvents[e].frame, frames);
        if (evFrame > cursor) {
            sampler_.render(outL + cursor, outR + cursor, evFrame - cursor);
            cursor = evFrame;
        }
        handleMidi(midiEvents[e]);
    }
    if (cursor < frames)
        sampler_.render(outL + cursor, outR + cursor, frames - cursor);

    if (lowCutEnabled_) {
        hpL_.process(outL, frames);
        hpR_.process(outR, frames);
    }

    const float g = st::dbToGain(paramValues_[st::kParamGain]);
    for (uint32_t i = 0; i < frames; ++i) { outL[i] *= g; outR[i] *= g; }
}

Plugin* createPlugin() { return new SampleTowerPlugin(); }

END_NAMESPACE_DISTRHO
