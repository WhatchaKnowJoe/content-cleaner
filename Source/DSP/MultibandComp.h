#pragma once
#include "ModuleBase.h"

/*  10. Three-band compressor (low < 250 Hz < mid < 3 kHz < high). One Amount
    control sets thresholds/ratios per band with speech-friendly offsets:
    the low band is tamed hardest (proximity boom), the mid gently, the high
    band lightly so de-essed sibilance isn't squashed twice. Auto make-up.      */
class MultibandComp : public ModuleBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;

    void setAmount (float a01) noexcept;

private:
    juce::dsp::LinkwitzRileyFilter<float> lowLP, lowHP, midLP, midHP;
    juce::dsp::Compressor<float> lowComp, midComp, highComp;
    float makeupGain = 1.f;
};
