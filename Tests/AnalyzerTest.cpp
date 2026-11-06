/*  Offline sanity test for the Analyzer and the spectral cleaner.

    Builds a deliberately bad 6-second "recording":
      speech-like bursts (harmonic buzz, varying level)  + room reverb (comb tail)
      + white noise floor  + 60 Hz hum with harmonic  + sibilant bursts  + clipped moments
    then checks the Analyzer flags each problem, and measures how much the spectral
    stage lowers the noise floor between words without eating the voice.              */
#include <JuceHeader.h>
#include "DSP/Analyzer.h"
#include "DSP/SpectralDenoiser.h"

static constexpr double sr = 48000.0;

struct Synth
{
    juce::Random rng { 42 };
    std::vector<float> comb1, comb2; size_t c1 = 0, c2 = 0;
    Synth() : comb1 (2400, 0.f), comb2 (3101, 0.f) {}

    float reverb (float x, float amount)
    {
        float y1 = comb1[c1], y2 = comb2[c2];
        comb1[c1] = x + y1 * 0.70f; comb2[c2] = x + y2 * 0.68f;
        c1 = (c1 + 1) % comb1.size(); c2 = (c2 + 1) % comb2.size();
        return x + amount * 0.5f * (y1 + y2);
    }
};

static juce::AudioBuffer<float> makeBadRecording (bool withReverb, bool withNoise, bool withHum, bool withClip, bool withSib)
{
    const int n = (int) (sr * 6.0);
    juce::AudioBuffer<float> buf (1, n);
    Synth s;
    juce::dsp::IIR::Filter<float> sibBP; sibBP.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, 7500.f, 1.2f);
    float* d = buf.getWritePointer (0);
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr;
        const int word = (int) (t / 0.5);                      // 500 ms per "word": 300 on, 200 off
        const bool on = std::fmod (t, 0.5) < 0.3;
        const float levelDb = -18.f + ((word * 7) % 11 - 5) * 1.2f;   // ±6 dB drift between words
        float v = 0.f;
        if (on)
        {
            const float f0 = 120.f + 20.f * std::sin (t * 3.0);
            for (int h = 1; h <= 12; ++h) v += std::sin ((float) (juce::MathConstants<double>::twoPi * f0 * h * t)) / (float) (h * h);
            v *= juce::Decibels::decibelsToGain (levelDb);
            if (withSib && (word % 3 == 0) && std::fmod (t, 0.5) > 0.2)   // S at the end of some words
                v += sibBP.processSample (s.rng.nextFloat() * 2.f - 1.f) * 0.25f;
            if (withClip && word % 5 == 0) v *= 8.f;                    // overloaded words
        }
        if (withReverb) v = s.reverb (v, 1.0f);
        if (withNoise)  v += (s.rng.nextFloat() * 2.f - 1.f) * juce::Decibels::decibelsToGain (-42.f);
        if (withHum)    v += 0.012f * std::sin ((float) (juce::MathConstants<double>::twoPi * 60.0 * t))
                           + 0.004f * std::sin ((float) (juce::MathConstants<double>::twoPi * 120.0 * t));
        d[i] = juce::jlimit (-1.f, 1.f, v);
    }
    return buf;
}

static Analyzer::Report analyse (const juce::AudioBuffer<float>& buf)
{
    Analyzer a; a.prepare (sr, 512, 1); a.start (5.0f);
    for (int pos = 0; pos + 512 <= buf.getNumSamples() && ! a.hasNewReport(); pos += 512)
    {
        juce::AudioBuffer<float> blk (1, 512);
        blk.copyFrom (0, 0, buf, 0, pos, 512);
        a.process (blk);
    }
    return a.takeReport();
}

// RMS (dB) of the gaps between words vs. the words themselves.
static void levels (const juce::AudioBuffer<float>& b, float& gapDb, float& wordDb)
{
    double g = 0, w = 0; int gn = 0, wn = 0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / sr, ph = std::fmod (t, 0.5);
        const float x = b.getSample (0, i);
        if (ph > 0.42 && ph < 0.5) { g += x * x; ++gn; }           // late gap (reverb has mostly died)
        else if (ph > 0.05 && ph < 0.28) { w += x * x; ++wn; }
    }
    gapDb  = 10.f * std::log10 ((float) (g / juce::jmax (1, gn)) + 1e-12f);
    wordDb = 10.f * std::log10 ((float) (w / juce::jmax (1, wn)) + 1e-12f);
}

int main()
{
    int failures = 0;
    auto check = [&] (bool ok, const juce::String& what) { std::printf ("  [%s] %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8()); if (! ok) ++failures; };

    std::printf ("== Analyzer: everything wrong ==\n");
    auto bad = makeBadRecording (true, true, true, true, true);
    auto r = analyse (bad);
    std::printf ("  noise %.1f dB  speech %.1f dB  snr %.1f dB  decay %.0f ms  hum %.1f dB@%.0f  sib %.1f dB  plosives %d  clips %d  uneven %.1f dB\n",
                 r.noiseFloorDb, r.speechLevelDb, r.snrDb, r.decayMs, r.humRatioDb, r.humFreq, r.sibilanceDb, r.plosiveFrames, r.clippedRuns, r.unevennessDb);
    for (auto& f : r.findings) std::printf ("    - %s\n", f.toRawUTF8());
    check (r.valid, "report produced");
    check (r.levels[0] >= 1, "echoey room detected");
    check (r.levels[1] >= 1, "background noise detected");
    check (r.levels[2] >= 1 && r.humFreq == 60.f, "60 Hz hum detected");
    check (r.levels[4] >= 1, "sibilance detected");
    check (r.levels[5] >= 1, "uneven level detected");
    check (r.levels[6] >= 1, "clipping detected");

    std::printf ("== Analyzer: clean recording ==\n");
    auto clean = makeBadRecording (false, false, false, false, false);
    auto rc = analyse (clean);
    std::printf ("  snr %.1f dB  decay %.0f ms  hum %.1f dB  sib %.1f dB  clips %d\n", rc.snrDb, rc.decayMs, rc.humRatioDb, rc.sibilanceDb, rc.clippedRuns);
    check (rc.levels[0] == 0, "no false room");
    check (rc.levels[2] == 0, "no false hum");
    check (rc.levels[6] == 0, "no false clipping");

    std::printf ("== Spectral cleaner: noise floor between words ==\n");
    auto noisy = makeBadRecording (false, true, false, false, false);
    float gapBefore, wordBefore; levels (noisy, gapBefore, wordBefore);

    SpectralDenoiser dn;
    dn.prepare ({ sr, 512, 1 });
    dn.setDenoiseEnabled (true); dn.setDereverbEnabled (false);
    dn.setDenoiseAmount (0.70f); dn.setCharacter (0.35f);   // 'Mid-High' problem level, Natural-ish
    juce::AudioBuffer<float> out (1, noisy.getNumSamples()); out.clear();
    for (int pos = 0; pos + 512 <= noisy.getNumSamples(); pos += 512)
    {
        juce::AudioBuffer<float> blk (1, 512);
        blk.copyFrom (0, 0, noisy, 0, pos, 512);
        dn.process (blk);
        out.copyFrom (0, pos, blk, 0, 0, 512);
    }
    // compensate the STFT latency when measuring
    juce::AudioBuffer<float> aligned (1, out.getNumSamples() - dn.getLatencySamples());
    aligned.copyFrom (0, 0, out, 0, dn.getLatencySamples(), aligned.getNumSamples());
    float gapAfter, wordAfter; levels (aligned, gapAfter, wordAfter);
    std::printf ("  gap  %.1f -> %.1f dB  (%.1f dB quieter)\n", gapBefore, gapAfter, gapBefore - gapAfter);
    std::printf ("  word %.1f -> %.1f dB  (%.1f dB change)\n", wordBefore, wordAfter, wordAfter - wordBefore);
    check (gapBefore - gapAfter >= 8.f, "noise floor dropped by at least 8 dB");
    check (std::abs (wordAfter - wordBefore) <= 2.f, "voice level kept within 2 dB");

    std::printf ("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
