#pragma once
#include "ModuleBase.h"
#include "Dereverb.h"
#include <atomic>
#include <vector>

/*  STFT engine hosting the two spectral stages:
        3. Denoise  — spectral subtraction / Wiener-style gain against a noise profile
        4. De-reverb — late-reverb suppression (DereverbEstimator)

    Noise profile:
      * "Learn Room": call startLearning(seconds); the module averages magnitude
        for that long, then freezes the profile. isLearning() goes false when done.
      * Otherwise an adaptive minimum-statistics tracker runs continuously, so the
        plugin works out of the box even if the user never presses Learn.

    Latency = fftSize samples (reported to the host via getLatencySamples()).

    Roadmap: replace computeDenoiseGains() with an ONNX / RTNeural speech-isolation
    model (DeepFilterNet-style). The STFT scaffolding stays the same.             */
class SpectralDenoiser : public ModuleBase
{
public:
    static constexpr int fftOrder = 10;              // 1024 @ 48k ≈ 21 ms frames
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int hopSize  = fftSize / 4;     // 75 % overlap
    static constexpr int numBins  = fftSize / 2 + 1;

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;
    int  getLatencySamples() const override { return fftSize; }

    // Parameters (set from the processor each block; 0..1)
    void setDenoiseAmount (float a)  noexcept { denoiseAmount = a; }
    void setDereverbAmount (float a) noexcept { dereverbAmount = a; }
    void setDenoiseEnabled (bool b)  noexcept { denoiseOn = b; }
    void setDereverbEnabled (bool b) noexcept { dereverbOn = b; }
    /** 0 = natural (gentle floor, heavy smoothing, few artefacts)
        1 = aggressive (deep floor, fast gains, maximum cleanup). */
    void setCharacter (float c) noexcept { character = juce::jlimit (0.f, 1.f, c); }

    // Learn Room
    void startLearning (float seconds);
    bool isLearning() const noexcept { return learnFramesLeft.load() > 0; }
    bool hasLearnedProfile() const noexcept { return profileLearned; }
    void clearLearnedProfile();

    // For the UI: latest per-bin gain (0..1) and magnitude, channel 0.
    const std::vector<float>& getLastGains() const noexcept { return lastGains; }
    const std::vector<float>& getLastMags() const noexcept  { return lastMags; }

private:
    struct Channel
    {
        std::vector<float> inBuf, outBuf, frame, mags, magSmooth, gains, noiseProfile, gainSmooth;
        int pos = 0, hopCount = 0;
        bool noiseSeeded = false;
    };

    void processFrame (Channel& ch, int channelIndex);
    void computeDenoiseGains (Channel& ch);

    juce::dsp::FFT fft { fftOrder };
    std::vector<float> window;
    float olaNorm = 1.f;
    double sampleRate = 48000.0;

    std::vector<Channel> channels;
    DereverbEstimator dereverb;

    float denoiseAmount = 0.5f, dereverbAmount = 0.3f, character = 0.35f;
    bool  denoiseOn = true, dereverbOn = true;

    std::atomic<int> learnFramesLeft { 0 };
    bool profileLearned = false;
    int  learnFramesTotal = 0;

    std::vector<float> lastGains, lastMags;
};
