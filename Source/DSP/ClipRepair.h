#pragma once
#include "ModuleBase.h"

/*  7. Clip restoration (basic). Detects runs of samples at/above the threshold
    and rebuilds them with a cubic Hermite curve fitted to the samples either
    side, then applies a little headroom so the reconstructed peak isn't
    re-clipped. Works on runs up to maxRun samples; longer runs are left alone.

    Introduces maxRun samples of latency (needs to see the end of a run).      */
class ClipRepair : public ModuleBase
{
public:
    static constexpr int maxRun = 64;

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;
    int  getLatencySamples() const override { return maxRun + 2; }

    void setThresholdDb (float db) noexcept { threshold = DSPUtil::dbToGain (db); }

private:
    struct Channel
    {
        std::vector<float> ring;    // delay line, length = latency
        int write = 0;
        int runStart = -1, runLen = 0;
        float lastGood = 0.f, lastGoodSlope = 0.f;
    };
    void repairRun (Channel& ch);
    float& at (Channel& ch, int idx);   // ring access relative to write head (0 = oldest)

    std::vector<Channel> channels;
    float threshold = DSPUtil::dbToGain (-0.3f);
    int ringLen = maxRun + 2;
};
