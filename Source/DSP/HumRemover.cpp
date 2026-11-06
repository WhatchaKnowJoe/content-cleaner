#include "HumRemover.h"

void HumRemover::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    for (auto& n : notches) n.prepare (spec);
    dirty = true;
    updateCoefficients();
}

void HumRemover::reset()                      { for (auto& n : notches) n.reset(); }
void HumRemover::setFundamental (float hz) noexcept { if (hz != fundamental) { fundamental = hz; dirty = true; } }
void HumRemover::setDepthDb (float db) noexcept     { if (db != depthDb)     { depthDb = db;     dirty = true; } }

void HumRemover::updateCoefficients()
{
    if (! dirty) return;
    dirty = false;
    const float gain = DSPUtil::dbToGain (-depthDb);
    for (int h = 0; h < numHarmonics; ++h)
    {
        const float f = fundamental * (float) (h + 1);
        // Q narrows for higher harmonics so we don't carve out the voice.
        const float q = 20.f + 10.f * (float) h;
        *notches[(size_t) h].state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, f, q, gain);
    }
}

void HumRemover::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed) return;
    updateCoefficients();
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    for (auto& n : notches) n.process (ctx);
}
