#pragma once
#include "ModuleBase.h"

/*  6. Mouth clicks + plosives.

    Plosive tamer (implemented): splits at ~150 Hz and runs a fast, hard
    compressor on the low band only, so "P" and "B" pops are flattened without
    thinning the voice the way a static high-pass would.

    De-click (basic): derivative-based detector; a sample whose slope exceeds an
    adaptive threshold gets replaced by a short linear interpolation. Good for
    isolated ticks; proper mouth-noise removal is an ML/roadmap item.           */
class DeClicker : public ModuleBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;

    void setPlosiveEnabled (bool b) noexcept   { plosiveOn = b; }
    void setDeClickEnabled (bool b) noexcept   { declickOn = b; }
    void setSensitivity (float a01) noexcept   { sensitivity = a01; }

private:
    void processPlosives (juce::AudioBuffer<float>& buffer);
    void processClicks (juce::AudioBuffer<float>& buffer);

    bool plosiveOn = true, declickOn = false;
    float sensitivity = 0.5f;

    juce::dsp::LinkwitzRileyFilter<float> lowSplit, highSplit;
    juce::dsp::Compressor<float> lowComp;
    juce::AudioBuffer<float> lowBand;

    // de-click state per channel
    std::vector<float> prevSample, slopeEnv;
    std::vector<int>   repairCountdown;
    float slopeEnvCoeff = 0.f;
};
