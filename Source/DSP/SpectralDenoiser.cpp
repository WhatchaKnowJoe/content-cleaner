#include "SpectralDenoiser.h"

void SpectralDenoiser::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Periodic Hann window
    window.resize ((size_t) fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f * (1.f - std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) fftSize));

    // Overlap-add normalisation: window applied at analysis AND synthesis.
    float sum = 0.f;
    for (int i = 0; i < fftSize; i += hopSize) sum += window[(size_t) i] * window[(size_t) i];
    olaNorm = 1.f / sum;

    channels.resize ((size_t) spec.numChannels);
    for (auto& ch : channels)
    {
        ch.inBuf.assign ((size_t) fftSize, 0.f);
        ch.outBuf.assign ((size_t) fftSize, 0.f);
        ch.frame.assign ((size_t) fftSize * 2, 0.f);
        ch.mags.assign ((size_t) numBins, 0.f);
        ch.magSmooth.assign ((size_t) numBins, 0.f);
        ch.gains.assign ((size_t) numBins, 1.f);
        ch.noiseProfile.assign ((size_t) numBins, 1.0e-4f);
        ch.gainSmooth.assign ((size_t) numBins, 1.f);
        ch.pos = ch.hopCount = 0;
        ch.noiseSeeded = false;
    }

    dereverb.prepare (numBins, (int) spec.numChannels, spec.sampleRate, hopSize);
    lastGains.assign ((size_t) numBins, 1.f);
    reset();
}

void SpectralDenoiser::reset()
{
    for (auto& ch : channels)
    {
        std::fill (ch.inBuf.begin(),  ch.inBuf.end(),  0.f);
        std::fill (ch.outBuf.begin(), ch.outBuf.end(), 0.f);
        std::fill (ch.gainSmooth.begin(), ch.gainSmooth.end(), 1.f);
        ch.pos = ch.hopCount = 0;
    }
    dereverb.reset();
}

void SpectralDenoiser::startLearning (float seconds)
{
    const int frames = juce::jmax (1, (int) (seconds * sampleRate / hopSize));
    learnFramesTotal = frames;
    for (auto& ch : channels)
        std::fill (ch.noiseProfile.begin(), ch.noiseProfile.end(), 0.f);
    profileLearned = false;
    learnFramesLeft.store (frames);
}

void SpectralDenoiser::clearLearnedProfile()
{
    profileLearned = false;
    for (auto& ch : channels)
        std::fill (ch.noiseProfile.begin(), ch.noiseProfile.end(), 1.0e-4f);
}

void SpectralDenoiser::process (juce::AudioBuffer<float>& buffer)
{
    // Even when bypassed we must keep the delay line running so latency stays constant.
    const int numCh = juce::jmin (buffer.getNumChannels(), (int) channels.size());
    const int n = buffer.getNumSamples();

    for (int c = 0; c < numCh; ++c)
    {
        auto& ch = channels[(size_t) c];
        float* data = buffer.getWritePointer (c);

        for (int i = 0; i < n; ++i)
        {
            const float out = ch.outBuf[(size_t) ch.pos];
            ch.outBuf[(size_t) ch.pos] = 0.f;
            ch.inBuf[(size_t) ch.pos]  = data[i];
            data[i] = out;

            if (++ch.pos >= fftSize) ch.pos = 0;
            if (++ch.hopCount >= hopSize)
            {
                ch.hopCount = 0;
                processFrame (ch, c);
            }
        }
    }
}

void SpectralDenoiser::processFrame (Channel& ch, int channelIndex)
{
    // Unwrap the circular input into a windowed frame (oldest sample first).
    for (int i = 0; i < fftSize; ++i)
        ch.frame[(size_t) i] = ch.inBuf[(size_t) ((ch.pos + i) % fftSize)] * window[(size_t) i];
    std::fill (ch.frame.begin() + fftSize, ch.frame.end(), 0.f);

    fft.performRealOnlyForwardTransform (ch.frame.data(), true);

    // Magnitudes
    for (int k = 0; k < numBins; ++k)
    {
        const float re = ch.frame[(size_t) (2 * k)], im = ch.frame[(size_t) (2 * k + 1)];
        ch.mags[(size_t) k] = std::sqrt (re * re + im * im);
        ch.gains[(size_t) k] = 1.f;
    }

    const bool active = ! bypassed;

    // --- Learn Room: accumulate the noise profile, output untouched ---------------
    if (learnFramesLeft.load() > 0)
    {
        for (int k = 0; k < numBins; ++k)
            ch.noiseProfile[(size_t) k] += ch.mags[(size_t) k] / (float) learnFramesTotal;

        if (channelIndex == (int) channels.size() - 1)   // count once per frame, not per channel
            if (learnFramesLeft.fetch_sub (1) == 1)
                profileLearned = true;
    }
    else if (active)
    {
        if (denoiseOn)  computeDenoiseGains (ch);
        if (dereverbOn) dereverb.applyGains (channelIndex, ch.mags.data(), ch.gains.data(), dereverbAmount, character);

        for (int k = 0; k < numBins; ++k)
        {
            ch.frame[(size_t) (2 * k)]     *= ch.gains[(size_t) k];
            ch.frame[(size_t) (2 * k + 1)] *= ch.gains[(size_t) k];
        }
    }

    if (channelIndex == 0)
        lastGains = ch.gains;

    fft.performRealOnlyInverseTransform (ch.frame.data());

    // Windowed overlap-add into the output ring.
    for (int i = 0; i < fftSize; ++i)
        ch.outBuf[(size_t) ((ch.pos + i) % fftSize)] += ch.frame[(size_t) i] * window[(size_t) i] * olaNorm;
}

void SpectralDenoiser::computeDenoiseGains (Channel& ch)
{
    // Adaptive minimum-statistics tracker runs only when no learned profile exists.
    const bool adaptive = ! profileLearned;
    if (adaptive && ! ch.noiseSeeded)
    {
        // Seed from the first frame so the tracker starts near reality instead of
        // crawling up from zero over a minute.
        for (int k = 0; k < numBins; ++k)
        {
            ch.magSmooth[(size_t) k]    = ch.mags[(size_t) k];
            ch.noiseProfile[(size_t) k] = ch.mags[(size_t) k] * 0.7f + 1.0e-6f;
        }
        ch.noiseSeeded = true;
    }

    const float a = denoiseAmount, cr = character;
    // Natural: modest over-subtraction, higher floor, slow gain motion (no "underwater").
    // Aggressive: deeper subtraction, lower floor, faster gains (max cleanup, more risk).
    const float overSub  = DSPUtil::mapAmount (a, 0.8f, 1.4f + 1.2f * cr);
    const float floorG   = DSPUtil::mapAmount (a, 0.7f, 0.18f - 0.16f * cr);
    const float smoothUp = 0.55f - 0.35f * cr, smoothDown = 0.92f - 0.15f * cr;

    for (int k = 0; k < numBins; ++k)
    {
        float& noise = ch.noiseProfile[(size_t) k];
        const float m = ch.mags[(size_t) k];

        if (adaptive)
        {
            // Minimum statistics on a smoothed magnitude (less Rayleigh jitter), then
            // bias-compensate: the running minimum sits well below the mean noise level.
            float& ms = ch.magSmooth[(size_t) k];
            ms = 0.75f * ms + 0.25f * m;
            if (ms < noise) noise = 0.85f * noise + 0.15f * ms;   // track down quickly
            else            noise *= 1.006f;                     // rise ~0.05 dB/frame: 20 dB in ~2 s
            noise = juce::jmax (noise, 1.0e-6f);
        }
        const float noiseEst = adaptive ? noise * 1.8f : noise;

        float g = 1.f - overSub * (noiseEst / (m + 1.0e-9f));
        g = juce::jlimit (floorG, 1.f, g);

        float& gs = ch.gainSmooth[(size_t) k];
        gs = g > gs ? smoothUp * gs + (1.f - smoothUp) * g
                    : smoothDown * gs + (1.f - smoothDown) * g;

        ch.gains[(size_t) k] *= gs;
    }
}
