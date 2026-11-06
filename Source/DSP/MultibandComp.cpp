#include "MultibandComp.h"

void MultibandComp::prepare (const juce::dsp::ProcessSpec& spec)
{
    using T = juce::dsp::LinkwitzRileyFilterType;
    for (auto* f : { &lowLP, &lowHP, &midLP, &midHP }) f->prepare (spec);
    lowLP.setType (T::lowpass);  lowLP.setCutoffFrequency (250.f);
    lowHP.setType (T::highpass); lowHP.setCutoffFrequency (250.f);
    midLP.setType (T::lowpass);  midLP.setCutoffFrequency (3000.f);
    midHP.setType (T::highpass); midHP.setCutoffFrequency (3000.f);

    for (auto* c : { &lowComp, &midComp, &highComp }) c->prepare (spec);
    lowComp.setAttack (10.f);  lowComp.setRelease (120.f);
    midComp.setAttack (8.f);   midComp.setRelease (100.f);
    highComp.setAttack (3.f);  highComp.setRelease (60.f);
    setAmount (0.4f);
    reset();
}

void MultibandComp::reset()
{
    for (auto* f : { &lowLP, &lowHP, &midLP, &midHP }) f->reset();
    for (auto* c : { &lowComp, &midComp, &highComp }) c->reset();
}

void MultibandComp::setAmount (float a) noexcept
{
    lowComp.setThreshold  (DSPUtil::mapAmount (a, -12.f, -36.f));
    lowComp.setRatio      (DSPUtil::mapAmount (a, 1.5f, 6.f));
    midComp.setThreshold  (DSPUtil::mapAmount (a, -14.f, -32.f));
    midComp.setRatio      (DSPUtil::mapAmount (a, 1.3f, 4.f));
    highComp.setThreshold (DSPUtil::mapAmount (a, -16.f, -30.f));
    highComp.setRatio     (DSPUtil::mapAmount (a, 1.2f, 3.f));
    makeupGain = DSPUtil::dbToGain (DSPUtil::mapAmount (a, 0.f, 6.f));
}

void MultibandComp::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed) return;
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();
    for (int c = 0; c < numCh; ++c)
    {
        float* d = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i)
        {
            const float x    = d[i];
            const float lo   = lowLP.processSample (c, x);
            const float rest = lowHP.processSample (c, x);
            const float mid  = midLP.processSample (c, rest);
            const float hi   = midHP.processSample (c, rest);
            d[i] = makeupGain * (lowComp.processSample (c, lo)
                               + midComp.processSample (c, mid)
                               + highComp.processSample (c, hi));
        }
    }
}
