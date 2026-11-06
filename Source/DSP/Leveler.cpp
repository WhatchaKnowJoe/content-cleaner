#include "Leveler.h"

void Leveler::prepare (const juce::dsp::ProcessSpec& spec)
{
    rmsCoeff      = DSPUtil::tauToCoeff (400.f,  spec.sampleRate);
    gainUpCoeff   = DSPUtil::tauToCoeff (1500.f, spec.sampleRate);   // boost slowly
    gainDownCoeff = DSPUtil::tauToCoeff (300.f,  spec.sampleRate);   // duck faster
    reset();
}

void Leveler::reset() { rmsSq = 0.f; currentGainDb = 0.f; }

void Leveler::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed || amount <= 0.001f) return;
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();
    const float invCh = 1.f / (float) numCh;

    for (int i = 0; i < n; ++i)
    {
        float sq = 0.f;
        for (int c = 0; c < numCh; ++c) { const float x = buffer.getSample (c, i); sq += x * x; }
        rmsSq = rmsCoeff * rmsSq + (1.f - rmsCoeff) * sq * invCh;

        const float levelDb = 10.f * std::log10 (rmsSq + 1.0e-12f);
        float desiredDb = currentGainDb;
        if (levelDb > speechFloorDb)
            desiredDb = juce::jlimit (-maxGainDb, maxGainDb, (targetDb - levelDb) * amount);

        const float coeff = desiredDb > currentGainDb ? gainUpCoeff : gainDownCoeff;
        currentGainDb = coeff * currentGainDb + (1.f - coeff) * desiredDb;

        const float g = DSPUtil::dbToGain (currentGainDb);
        for (int c = 0; c < numCh; ++c) buffer.getWritePointer (c)[i] *= g;
    }
}
