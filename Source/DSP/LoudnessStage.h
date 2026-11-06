#pragma once
#include "ModuleBase.h"

/*  12. Loudness targeting + true-peak-style limiter.

    Measures K-weighted (ITU-R BS.1770 pre-filter) short-term loudness, then
    very slowly steers a gain so the programme sits at the target LUFS, and
    finally limits at the ceiling. The slow steer (≈ 1 dB/s) means it behaves
    like a mastering trim, not a compressor. Measurement is short-term (3 s)
    rather than fully gated integrated loudness — accurate enough for
    conversational speech, and it runs in real time.                            */
class LoudnessStage : public ModuleBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;

    void setTargetLUFS (float lufs) noexcept { targetLUFS = lufs; }   // 0 = off
    void setCeilingDb (float db) noexcept;
    void setLimiterEnabled (bool b) noexcept { limiterOn = b; }

    float getShortTermLUFS() const noexcept { return shortTermLUFS; }
    float getTrimDb() const noexcept        { return trimDb; }

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    Filter kShelf, kHighpass;                 // K-weighting
    juce::AudioBuffer<float> weighted;
    juce::dsp::Limiter<float> limiter;

    float targetLUFS = -14.f, ceilingDb = -1.f;
    bool limiterOn = true;
    float msCoeff = 0.f, meanSq = 0.f, shortTermLUFS = -70.f, trimDb = 0.f;
    float trimStepPerSample = 0.f;
};
