#include "DeClicker.h"

void DeClicker::prepare (const juce::dsp::ProcessSpec& spec)
{
    lowSplit.prepare (spec);  lowSplit.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);  lowSplit.setCutoffFrequency (150.f);
    highSplit.prepare (spec); highSplit.setType (juce::dsp::LinkwitzRileyFilterType::highpass); highSplit.setCutoffFrequency (150.f);

    lowComp.prepare (spec);
    lowComp.setThreshold (-30.f);
    lowComp.setRatio (8.f);
    lowComp.setAttack (1.f);
    lowComp.setRelease (80.f);

    lowBand.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);

    prevSample.assign (spec.numChannels, 0.f);
    slopeEnv.assign (spec.numChannels, 0.f);
    repairCountdown.assign (spec.numChannels, 0);
    slopeEnvCoeff = DSPUtil::tauToCoeff (20.f, spec.sampleRate);
    reset();
}

void DeClicker::reset()
{
    lowSplit.reset(); highSplit.reset(); lowComp.reset();
    std::fill (prevSample.begin(), prevSample.end(), 0.f);
    std::fill (slopeEnv.begin(), slopeEnv.end(), 0.f);
    std::fill (repairCountdown.begin(), repairCountdown.end(), 0);
}

void DeClicker::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed) return;
    if (declickOn) processClicks (buffer);
    if (plosiveOn) processPlosives (buffer);
}

void DeClicker::processPlosives (juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();
    lowBand.setSize (numCh, n, false, false, true);

    for (int c = 0; c < numCh; ++c)
    {
        auto* in  = buffer.getWritePointer (c);
        auto* low = lowBand.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            const float x = in[i];
            low[i] = lowComp.processSample (c, lowSplit.processSample (c, x));
            in[i]  = highSplit.processSample (c, x) + low[i];
        }
    }
}

void DeClicker::processClicks (juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();
    // Higher sensitivity -> lower multiplier over the running slope envelope.
    const float threshMul = DSPUtil::mapAmount (sensitivity, 14.f, 5.f);

    for (int c = 0; c < numCh; ++c)
    {
        auto* d = buffer.getWritePointer (c);
        float& prev = prevSample[(size_t) c];
        float& env  = slopeEnv[(size_t) c];
        int& cd     = repairCountdown[(size_t) c];

        for (int i = 0; i < n; ++i)
        {
            const float slope = std::abs (d[i] - prev);
            if (cd > 0)
            {
                d[i] = prev;            // hold through the click (short, so inaudible)
                --cd;
            }
            else if (slope > threshMul * (env + 1.0e-4f) && env > 1.0e-4f)
            {
                d[i] = prev;
                cd = 3;                 // repair the next few samples too
            }
            env  = slopeEnvCoeff * env + (1.f - slopeEnvCoeff) * slope;
            prev = d[i];
        }
    }
}
