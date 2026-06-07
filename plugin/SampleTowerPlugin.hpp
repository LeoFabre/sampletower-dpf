#pragma once
#include "DistrhoPlugin.hpp"
#include "ParameterDefs.hpp"
#include "Sampler.hpp"
#include "HighPassChain.hpp"

START_NAMESPACE_DISTRHO

class SampleTowerPlugin : public Plugin
{
public:
    SampleTowerPlugin();

    const char* getLabel()       const override { return "SampleTower"; }
    const char* getDescription() const override { return "11-slot MIDI sample player"; }
    const char* getMaker()       const override { return "Nexus"; }
    const char* getHomePage()    const override { return "https://github.com/LeoFabre/sampletower-dpf"; }
    const char* getLicense()     const override { return "GPL-3.0-or-later"; }
    uint32_t    getVersion()     const override { return d_version(0, 1, 0); }
    int64_t     getUniqueId()    const override { return d_cconst('S','t','w','r'); }

    void initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

    void activate() override;
    void run(const float** inputs, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override;

private:
    st::Sampler sampler_;
    st::HighPassChain hpL_, hpR_;
    float paramValues_[st::kParamCount] {};
    double sampleRate_ = 0.0;
    bool   lowCutEnabled_ = true;

    void loadSamplesFromConfig();
    void pushParamsToDsp();
    void handleMidi(const MidiEvent& ev);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleTowerPlugin)
};

END_NAMESPACE_DISTRHO
