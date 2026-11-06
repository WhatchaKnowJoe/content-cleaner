#include "DeEsser.h"

void DeEsser::prepare (const juce::dsp::ProcessSpec& spec)
{
    low.prepare (spec);  low.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    high.prepare (spec); high.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
    comp.prepare (spec);
    comp.setAttack (0.5f);
    comp.setRelease (40.f);
    setFrequency (freq);
    setAmount (amount);
    reset();
}

void DeEsser::reset() { low.reset(); high.reset(); comp.reset(); }

void DeEsser::setFrequency (float hz) noexcept
{
    freq = hz;
    low.setCutoffFrequency (hz);
    high.setCutoffFrequency (hz);
}

void DeEsser::setAmount (float a01) noexcept
{
    amount = a01;
    comp.setThreshold (DSPUtil::mapAmount (a01, -18.f, -42.f));
    comp.setRatio     (DSPUtil::mapAmount (a01, 1.5f, 8.f));
}

void DeEsser::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed) return;
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();
    for (int c = 0; c < numCh; ++c)
    {
        float* d = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            const float x  = d[i];
            const float lo = low.processSample (c, x);
            const float hi = comp.processSample (c, high.processSample (c, x));
            d[i] = lo + hi;
        }
    }
}
