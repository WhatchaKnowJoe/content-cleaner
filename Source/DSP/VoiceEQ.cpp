#include "VoiceEQ.h"

void VoiceEQ::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    mud.prepare (spec); presence.prepare (spec); air.prepare (spec);
    dirty = true;
    update();
}

void VoiceEQ::reset() { mud.reset(); presence.reset(); air.reset(); }

void VoiceEQ::update()
{
    if (! dirty) return;
    dirty = false;
    using C = juce::dsp::IIR::Coefficients<float>;
    *mud.state      = *C::makePeakFilter (sampleRate, 250.f,  1.0f, DSPUtil::dbToGain (mudDb));
    *presence.state = *C::makePeakFilter (sampleRate, 3500.f, 0.9f, DSPUtil::dbToGain (presenceDb));
    *air.state      = *C::makeHighShelf  (sampleRate, 10000.f, 0.7f, DSPUtil::dbToGain (airDb));
}

void VoiceEQ::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed) return;
    update();
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    mud.process (ctx); presence.process (ctx); air.process (ctx);
}
