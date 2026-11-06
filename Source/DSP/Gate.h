#pragma once
#include "ModuleBase.h"

/*  5. Downward expander ("soft gate"). Linked across channels so stereo images
    don't wobble. Range limits how far it closes, so it never sounds like a
    hard cut on room tone.                                                        */
class Gate : public ModuleBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;

    void setThresholdDb (float db) noexcept { thresholdDb = db; }
    void setRangeDb (float db) noexcept     { rangeDb = db; }
    void setReleaseMs (float ms) noexcept;
    void setAttackMs (float ms) noexcept;
    void setHoldMs (float ms) noexcept;

    float getCurrentGainReductionDb() const noexcept { return currentGrDb; }

private:
    double sampleRate = 48000.0;
    float thresholdDb = -50.f, rangeDb = 12.f;
    float attackCoeff = 0.f, releaseCoeff = 0.f, detCoeff = 0.f;
    int holdSamples = 0, holdCounter = 0;
    float envelope = 0.f, gain = 1.f, currentGrDb = 0.f;
};
