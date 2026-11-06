#pragma once
#include <JuceHeader.h>
#include <vector>

/*  Late-reverb suppression operating on STFT magnitudes.
    Runs inside SpectralDenoiser's frame loop (shares the FFT).

    Model: late reverberation in each bin is approximated as an exponentially
    decaying, time-smoothed version of recent magnitude. The ratio of that estimate
    to the current magnitude drives a per-bin attenuation.

    Baseline algorithm — slot an ML model here later (see README roadmap).      */
class DereverbEstimator
{
public:
    void prepare (int numBins, int numChannels, double sampleRate, int hopSize);
    void reset();

    /** Computes per-bin gains in [floor,1] for one channel's magnitude frame
        and multiplies them into `gains` (so denoise + dereverb stack).      */
    void applyGains (int channel, const float* mags, float* gains, float amount01, float character01);

private:
    int bins = 0;
    float decayCoeff = 0.f;      // per-frame decay of the reverb estimate
    float smoothCoeff = 0.f;     // per-frame smoothing of the gain
    std::vector<std::vector<float>> reverbEst, prevMag, gainState;
};
