#pragma once
#include "ModuleBase.h"

/*  9. Split-band de-esser. Linkwitz-Riley split at the sibilance frequency;
    the high band runs through a fast compressor whose threshold/ratio are
    derived from Amount, then the bands are summed (LR is phase-coherent).      */
class DeEsser : public ModuleBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;

    void setFrequency (float hz) noexcept;
    void setAmount (float a01) noexcept;

private:
    juce::dsp::LinkwitzRileyFilter<float> low, high;
    juce::dsp::Compressor<float> comp;
    float freq = 6500.f, amount = 0.4f;
};
