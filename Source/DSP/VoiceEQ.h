#pragma once
#include "ModuleBase.h"

/*  8. Three-band voice EQ with fixed, speech-tuned centres:
       Mud      — peak cut around 250 Hz  (boxiness / proximity build-up)
       Presence — peak around 3.5 kHz     (intelligibility)
       Air      — high shelf at 10 kHz                                            */
class VoiceEQ : public ModuleBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;

    void setMudDb (float db) noexcept      { if (db != mudDb)      { mudDb = db;      dirty = true; } }
    void setPresenceDb (float db) noexcept { if (db != presenceDb) { presenceDb = db; dirty = true; } }
    void setAirDb (float db) noexcept      { if (db != airDb)      { airDb = db;      dirty = true; } }

private:
    void update();
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    Filter mud, presence, air;
    double sampleRate = 48000.0;
    float mudDb = -2.f, presenceDb = 1.5f, airDb = 1.f;
    bool dirty = true;
};
