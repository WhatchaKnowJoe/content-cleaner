#include "Gate.h"

void Gate::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    detCoeff = DSPUtil::tauToCoeff (5.f, sampleRate);     // RMS-ish detector
    setAttackMs (3.f);
    setReleaseMs (150.f);
    setHoldMs (60.f);
    reset();
}

void Gate::reset()                       { envelope = 0.f; gain = 1.f; holdCounter = 0; currentGrDb = 0.f; }
void Gate::setReleaseMs (float ms) noexcept { releaseCoeff = DSPUtil::tauToCoeff (ms, sampleRate); }
void Gate::setAttackMs (float ms) noexcept  { attackCoeff  = DSPUtil::tauToCoeff (ms, sampleRate); }
void Gate::setHoldMs (float ms) noexcept    { holdSamples  = (int) (0.001f * ms * (float) sampleRate); }

void Gate::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed) { currentGrDb = 0.f; return; }

    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    const float thresholdLin = DSPUtil::dbToGain (thresholdDb);
    const float floorGain    = DSPUtil::dbToGain (-rangeDb);
    const float kneeLin      = DSPUtil::dbToGain (thresholdDb + 6.f);   // 6 dB soft knee above threshold

    for (int i = 0; i < n; ++i)
    {
        // Linked detector: max across channels
        float x = 0.f;
        for (int c = 0; c < numCh; ++c) x = juce::jmax (x, std::abs (buffer.getSample (c, i)));
        envelope = detCoeff * envelope + (1.f - detCoeff) * x;

        // Target gain: 1 above knee, floorGain below threshold, linear-in-dB between.
        float target;
        if (envelope >= kneeLin)       target = 1.f;
        else if (envelope <= thresholdLin) target = floorGain;
        else
        {
            const float t = (DSPUtil::gainToDb (envelope) - thresholdDb) / 6.f;
            target = DSPUtil::dbToGain (-rangeDb * (1.f - t));
        }

        if (target >= gain) { gain = attackCoeff * gain + (1.f - attackCoeff) * target; holdCounter = holdSamples; }
        else if (holdCounter > 0) --holdCounter;
        else gain = releaseCoeff * gain + (1.f - releaseCoeff) * target;

        for (int c = 0; c < numCh; ++c)
            buffer.getWritePointer (c)[i] *= gain;
    }
    currentGrDb = DSPUtil::gainToDb (gain);
}
