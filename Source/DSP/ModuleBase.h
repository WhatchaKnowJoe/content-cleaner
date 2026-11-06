#pragma once
#include <JuceHeader.h>

/*  Common interface for every stage in the chain.
    Modules are mono-or-stereo agnostic: they size their state from the ProcessSpec. */
class ModuleBase
{
public:
    virtual ~ModuleBase() = default;
    virtual void prepare (const juce::dsp::ProcessSpec& spec) = 0;
    virtual void reset() = 0;
    virtual void process (juce::AudioBuffer<float>& buffer) = 0;
    virtual int  getLatencySamples() const { return 0; }

    void setBypassed (bool b) noexcept { bypassed = b; }
    bool isBypassed() const noexcept   { return bypassed; }

protected:
    bool bypassed = false;
};

namespace DSPUtil
{
    inline float dbToGain (float db) noexcept { return juce::Decibels::decibelsToGain (db); }
    inline float gainToDb (float g)  noexcept { return juce::Decibels::gainToDecibels (g, -120.f); }

    // Map 0..100 "amount" to a range, optionally with a curve.
    inline float mapAmount (float amount01, float lo, float hi, float curve = 1.f) noexcept
    {
        const float a = std::pow (juce::jlimit (0.f, 1.f, amount01), curve);
        return lo + (hi - lo) * a;
    }

    // One-pole smoothing coefficient for a given time constant.
    inline float tauToCoeff (float ms, double sr) noexcept
    {
        return ms <= 0.f ? 0.f : std::exp (-1.f / (0.001f * ms * (float) sr));
    }
}
