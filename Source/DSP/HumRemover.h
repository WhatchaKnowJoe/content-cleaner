#pragma once
#include "ModuleBase.h"

/*  2. Mains hum removal: narrow notches at the fundamental and its first
    harmonics (50/60 Hz, x2..x6). Depth sets attenuation.                        */
class HumRemover : public ModuleBase
{
public:
    static constexpr int numHarmonics = 6;

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;

    void setFundamental (float hz) noexcept;
    void setDepthDb (float db) noexcept;

private:
    void updateCoefficients();

    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    std::array<Filter, numHarmonics> notches;
    double sampleRate = 48000.0;
    float fundamental = 60.f, depthDb = 30.f;
    bool dirty = true;
};
