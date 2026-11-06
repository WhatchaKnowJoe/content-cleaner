#include "ClipRepair.h"

void ClipRepair::prepare (const juce::dsp::ProcessSpec& spec)
{
    ringLen = maxRun + 4;
    channels.assign (spec.numChannels, {});
    for (auto& ch : channels) ch.ring.assign ((size_t) ringLen, 0.f);
    reset();
}

void ClipRepair::reset()
{
    for (auto& ch : channels)
    {
        std::fill (ch.ring.begin(), ch.ring.end(), 0.f);
        ch.write = 0; ch.runStart = -1; ch.runLen = 0;
    }
}

// idx: 0 = slot just written (newest), 1 = one before, ...
float& ClipRepair::at (Channel& ch, int back)
{
    int i = ch.write - 1 - back;
    while (i < 0) i += ringLen;
    return ch.ring[(size_t) (i % ringLen)];
}

void ClipRepair::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh = juce::jmin (buffer.getNumChannels(), (int) channels.size());
    const int n = buffer.getNumSamples();

    for (int c = 0; c < numCh; ++c)
    {
        auto& ch = channels[(size_t) c];
        float* d = buffer.getWritePointer (c);

        for (int i = 0; i < n; ++i)
        {
            const float x = d[i];
            d[i] = ch.ring[(size_t) ch.write];            // delayed output
            ch.ring[(size_t) ch.write] = x;
            ch.write = (ch.write + 1) % ringLen;

            if (bypassed) continue;

            const bool clipped = std::abs (x) >= threshold;
            if (clipped) { ++ch.runLen; }
            else
            {
                if (ch.runLen > 0 && ch.runLen <= maxRun) repairRun (ch);
                ch.runLen = 0;
            }
        }
    }
}

void ClipRepair::repairRun (Channel& ch)
{
    // Newest sample (back = 0) is the first clean sample after the run.
    const int len = ch.runLen;
    const float b   = at (ch, 0);              // after run
    const float a   = at (ch, len + 1);        // before run
    const float a2  = at (ch, len + 2);        // two before run
    const float sign = at (ch, 1) >= 0.f ? 1.f : -1.f;

    // Tangents in curve-parameter units (t in 0..1 spans len+1 samples).
    const float ta = (a - a2) * (float) (len + 1);
    const float tb = -ta;                       // assume a roughly symmetric peak

    for (int k = 1; k <= len; ++k)
    {
        const float t  = (float) k / (float) (len + 1);
        const float t2 = t * t, t3 = t2 * t;
        const float h00 =  2.f * t3 - 3.f * t2 + 1.f;
        const float h10 =        t3 - 2.f * t2 + t;
        const float h01 = -2.f * t3 + 3.f * t2;
        const float h11 =        t3 -       t2;
        float y = h00 * a + h10 * ta + h01 * b + h11 * tb;

        // Never reconstruct below the clipped level, never go absurd.
        y = sign * juce::jlimit (threshold, 4.f * threshold, std::abs (y));
        at (ch, len + 1 - k) = y;
    }
}
