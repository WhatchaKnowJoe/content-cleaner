#include "Dereverb.h"
#include "ModuleBase.h"

void DereverbEstimator::prepare (int numBins, int numChannels, double sampleRate, int hopSize)
{
    bins = numBins;
    const double frameRate = sampleRate / hopSize;          // frames per second
    // Reverb tail estimate decays with ~120 ms time constant; gain smooths over ~30 ms.
    decayCoeff  = (float) std::exp (-1.0 / (0.120 * frameRate));
    smoothCoeff = (float) std::exp (-1.0 / (0.030 * frameRate));

    reverbEst.assign ((size_t) numChannels, std::vector<float> ((size_t) bins, 0.f));
    prevMag  .assign ((size_t) numChannels, std::vector<float> ((size_t) bins, 0.f));
    gainState.assign ((size_t) numChannels, std::vector<float> ((size_t) bins, 1.f));
}

void DereverbEstimator::reset()
{
    for (auto& v : reverbEst) std::fill (v.begin(), v.end(), 0.f);
    for (auto& v : prevMag)   std::fill (v.begin(), v.end(), 0.f);
    for (auto& v : gainState) std::fill (v.begin(), v.end(), 1.f);
}

void DereverbEstimator::applyGains (int ch, const float* mags, float* gains, float amount01, float character01)
{
    if (amount01 <= 0.001f) return;

    auto& R  = reverbEst[(size_t) ch];
    auto& P  = prevMag  [(size_t) ch];
    auto& G  = gainState[(size_t) ch];

    // Stronger amount -> more aggressive subtraction and lower floor.
    const float strength = DSPUtil::mapAmount (amount01, 0.3f, 1.1f + 0.8f * character01);
    const float floorG   = DSPUtil::mapAmount (amount01, 0.9f, 0.25f - 0.2f * character01);

    for (int k = 0; k < bins; ++k)
    {
        // Reverb estimate: decaying memory of previous frame's magnitude.
        R[(size_t) k] = juce::jmax (R[(size_t) k] * decayCoeff, P[(size_t) k] * decayCoeff);
        P[(size_t) k] = mags[k];

        const float m = mags[k] + 1.0e-9f;
        float g = 1.f - strength * (R[(size_t) k] / m);
        g = juce::jlimit (floorG, 1.f, g);

        // Fast attack on gain increase (speech onset), smoothed release.
        G[(size_t) k] = g > G[(size_t) k] ? g : smoothCoeff * G[(size_t) k] + (1.f - smoothCoeff) * g;
        gains[k] *= G[(size_t) k];
    }
}
