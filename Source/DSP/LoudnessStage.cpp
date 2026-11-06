#include "LoudnessStage.h"

void LoudnessStage::prepare (const juce::dsp::ProcessSpec& spec)
{
    using C = juce::dsp::IIR::Coefficients<float>;
    kShelf.prepare (spec);    kHighpass.prepare (spec);
    // BS.1770 pre-filter approximated by its design targets so it works at any rate:
    // +4 dB high shelf around 1.68 kHz, then a 2nd-order high-pass at ~38 Hz.
    *kShelf.state    = *C::makeHighShelf (spec.sampleRate, 1681.97f, 0.7071f, DSPUtil::dbToGain (3.99984f));
    *kHighpass.state = *C::makeHighPass  (spec.sampleRate, 38.13f, 0.5f);

    weighted.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);

    limiter.prepare (spec);
    limiter.setRelease (60.f);
    setCeilingDb (ceilingDb);

    msCoeff = DSPUtil::tauToCoeff (3000.f, spec.sampleRate);          // 3 s short-term window
    trimStepPerSample = 1.f / (float) spec.sampleRate;                // ≈ 1 dB per second
    reset();
}

void LoudnessStage::reset()
{
    kShelf.reset(); kHighpass.reset(); limiter.reset();
    meanSq = 0.f; shortTermLUFS = -70.f; trimDb = 0.f;
}

void LoudnessStage::setCeilingDb (float db) noexcept
{
    ceilingDb = db;
    limiter.setThreshold (db);
}

void LoudnessStage::process (juce::AudioBuffer<float>& buffer)
{
    if (bypassed) return;
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();

    // ---- Loudness measurement on a K-weighted copy --------------------------------
    if (targetLUFS != 0.f)
    {
        weighted.setSize (numCh, n, false, false, true);
        for (int c = 0; c < numCh; ++c) weighted.copyFrom (c, 0, buffer, c, 0, n);
        juce::dsp::AudioBlock<float> wb (weighted);
        juce::dsp::ProcessContextReplacing<float> ctx (wb);
        kShelf.process (ctx); kHighpass.process (ctx);

        for (int i = 0; i < n; ++i)
        {
            float sq = 0.f;
            for (int c = 0; c < numCh; ++c) { const float x = weighted.getSample (c, i); sq += x * x; }
            meanSq = msCoeff * meanSq + (1.f - msCoeff) * sq;
        }
        shortTermLUFS = -0.691f + 10.f * std::log10 (meanSq + 1.0e-12f);

        // Only steer while there is programme (not during silence), and slowly.
        if (shortTermLUFS > -50.f)
        {
            const float wanted = juce::jlimit (-12.f, 12.f, targetLUFS - (shortTermLUFS - trimDb));
            const float maxStep = trimStepPerSample * (float) n;
            trimDb += juce::jlimit (-maxStep, maxStep, wanted - trimDb);
        }
        buffer.applyGain (DSPUtil::dbToGain (trimDb));
    }

    // ---- Limiter --------------------------------------------------------------------
    if (limiterOn)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        limiter.process (ctx);
    }
}
