#include "Analyzer.h"
#include "ModuleBase.h"
#include "../Parameters.h"

void Analyzer::prepare (double sampleRate, int, int)
{
    sr = sampleRate;
    frameLen = (int) (sr * 0.02);                          // 20 ms frames
    using C = juce::dsp::IIR::Coefficients<float>;
    lowLP.coefficients = C::makeLowPass  (sr, 150.f);
    midBP.coefficients = C::makeBandPass (sr, 2000.f, 0.6f);   // ~1–4 kHz
    hiBP.coefficients  = C::makeBandPass (sr, 7000.f, 0.9f);   // ~5–10 kHz
    for (size_t i = 0; i < gz.size(); ++i)
        gz[i].coeff = 2.f * std::cos (juce::MathConstants<float>::twoPi * gzFreqs[i] / (float) sr);
    frameDb.reserve (1000); lowDb.reserve (1000); midDb.reserve (1000); hiDb.reserve (1000);
    reset();
}

void Analyzer::reset()
{
    lowLP.reset(); midBP.reset(); hiBP.reset();
    framePos = 0; frameSumSq = frameLowSq = frameMidSq = frameHiSq = 0.f;
    clipRun = 0; clippedRuns = 0; totalPower = 0.0;
    for (auto& g : gz) { g.s1 = g.s2 = 0.f; g.power = 0.0; }
    frameDb.clear(); lowDb.clear(); midDb.clear(); hiDb.clear();
}

void Analyzer::start (float seconds)
{
    framesWanted = juce::jmax (10, (int) (seconds / 0.02f));
    reportReady.store (false);
    reset();
    running.store (true);
}

Analyzer::Report Analyzer::takeReport()
{
    reportReady.store (false);
    return report;
}

void Analyzer::process (const juce::AudioBuffer<float>& in)
{
    if (! running.load()) return;
    const int numCh = in.getNumChannels(), n = in.getNumSamples();
    const float invCh = 1.f / (float) juce::jmax (1, numCh);

    for (int i = 0; i < n; ++i)
    {
        float x = 0.f; float peak = 0.f;
        for (int c = 0; c < numCh; ++c) { const float v = in.getSample (c, i); x += v; peak = juce::jmax (peak, std::abs (v)); }
        x *= invCh;

        // clipping runs
        if (peak >= 0.985f) { if (++clipRun == 3) ++clippedRuns; } else clipRun = 0;

        frameSumSq += x * x;
        const float lo = lowLP.processSample (x), mid = midBP.processSample (x), hi = hiBP.processSample (x);
        frameLowSq += lo * lo; frameMidSq += mid * mid; frameHiSq += hi * hi;

        for (auto& g : gz) { const float s0 = x + g.coeff * g.s1 - g.s2; g.s2 = g.s1; g.s1 = s0; }
        totalPower += x * x;

        if (++framePos >= frameLen)
        {
            const float inv = 1.f / (float) frameLen;
            frameDb.push_back (10.f * std::log10 (frameSumSq * inv + 1.0e-12f));
            lowDb.push_back   (10.f * std::log10 (frameLowSq * inv + 1.0e-12f));
            midDb.push_back   (10.f * std::log10 (frameMidSq * inv + 1.0e-12f));
            hiDb.push_back    (10.f * std::log10 (frameHiSq * inv + 1.0e-12f));
            frameSumSq = frameLowSq = frameMidSq = frameHiSq = 0.f; framePos = 0;

            // Fold Goertzel every ~0.5 s: long enough to resolve 60 vs 65 Hz (needs > 0.2 s).
            if (frameDb.size() % 25 == 0)
                for (auto& g : gz)
                {
                    g.power += (double) (g.s1 * g.s1 + g.s2 * g.s2 - g.coeff * g.s1 * g.s2);
                    g.s1 = g.s2 = 0.f;
                }

            if ((int) frameDb.size() >= framesWanted) { finish(); return; }
        }
    }
}

float Analyzer::percentile (std::vector<float> v, float p)
{
    if (v.empty()) return -90.f;
    std::sort (v.begin(), v.end());
    return v[(size_t) juce::jlimit (0, (int) v.size() - 1, (int) (p * (float) (v.size() - 1)))];
}

void Analyzer::finish()
{
    running.store (false);
    Report r;
    r.totalFrames = (int) frameDb.size();
    r.noiseFloorDb  = percentile (frameDb, 0.10f);
    r.speechLevelDb = percentile (frameDb, 0.85f);
    r.snrDb = r.speechLevelDb - r.noiseFloorDb;
    r.speechLUFSApprox = r.speechLevelDb - 3.f;             // RMS→LUFS rough offset for speech
    r.clippedRuns = clippedRuns;

    const float speechGate = r.noiseFloorDb + juce::jmax (6.f, r.snrDb * 0.4f);

    // --- unevenness: spread of per-word peak levels (ignores onsets/offsets)
    {
        std::vector<float> peaks; float cur = -200.f; bool inWord = false;
        for (float d : frameDb)
        {
            if (d > speechGate) { inWord = true; cur = juce::jmax (cur, d); }
            else if (inWord)    { peaks.push_back (cur); cur = -200.f; inWord = false; }
        }
        if (peaks.size() > 3)
        {
            float mean = 0.f; for (float d : peaks) mean += d; mean /= (float) peaks.size();
            float var = 0.f;  for (float d : peaks) var += (d - mean) * (d - mean); var /= (float) peaks.size();
            r.unevennessDb = std::sqrt (var);
        }
    }

    // --- reverb decay: median slope (dB per 20 ms frame) of descending runs after speech.
    //     Dry speech falls fast (> 5 dB/frame); a live room falls ~1 dB/frame.
    //     A run must be monotonic for >= 4 frames, fall >= 3 dB and end clearly below speech.
    {
        std::vector<float> slopes;
        size_t i = 1;
        while (i < frameDb.size())
        {
            if (frameDb[i - 1] > speechGate + 3.f && frameDb[i] < frameDb[i - 1] - 0.1f)
            {
                size_t j = i;
                while (j < frameDb.size() && frameDb[j] < frameDb[j - 1] - 0.1f) ++j;
                const size_t len = j - (i - 1);
                const float drop = frameDb[i - 1] - frameDb[j - 1];
                if (len >= 4 && drop >= 3.f && frameDb[j - 1] < r.speechLevelDb - 5.f)
                    slopes.push_back (drop / (float) (len - 1));
                i = j + 1;
            }
            else ++i;
        }
        if (! slopes.empty())
        {
            std::sort (slopes.begin(), slopes.end());
            const float slope = juce::jmax (0.05f, slopes[slopes.size() / 2]);
            r.decayMs = 20.f / slope * 20.f;              // ms to fall 20 dB
        }
    }

    // --- hum: compare 50 vs 55 and 60 vs 65 Hz, pick the stronger candidate
    {
        auto db = [] (double p) { return 10.f * std::log10 ((float) p + 1.0e-12f); };
        const float r50 = db (gz[0].power) - db (gz[1].power);
        const float r60 = db (gz[2].power) - db (gz[3].power);
        r.humFreq = r60 >= r50 ? 60.f : 50.f;
        const float fund = r60 >= r50 ? db (gz[2].power) : db (gz[0].power);
        const float harm = r60 >= r50 ? db (gz[5].power) : db (gz[4].power);
        const float absolute = fund - db (totalPower);     // hum power relative to everything
        r.humRatioDb = juce::jmax (r50, r60) + juce::jmax (0.f, harm - fund + 10.f) * 0.3f;
        if (absolute < -45.f) r.humRatioDb = 0.f;          // too quiet to matter
    }

    // --- sibilance & plosives over speech frames
    {
        int n = 0; float sib = 0.f; int plos = 0;
        for (size_t i = 0; i < frameDb.size(); ++i)
        {
            if (frameDb[i] <= speechGate) continue;
            ++n; sib += hiDb[i] - midDb[i];
            if (lowDb[i] > frameDb[i] - 2.f && lowDb[i] > speechGate + 6.f) ++plos;
        }
        r.sibilanceDb = n > 0 ? sib / (float) n : -20.f;
        r.plosiveFrames = plos;
    }

    // --- recommendations ---------------------------------------------------------------
    auto lvl = [] (float v, float t1, float t2, float t3) { return v >= t3 ? 3 : v >= t2 ? 2 : v >= t1 ? 1 : 0; };
    r.levels[0] = lvl (r.decayMs,          150.f, 280.f, 450.f);                  // room
    r.levels[1] = lvl (-r.snrDb,           -45.f, -32.f, -22.f);                  // noise (lower SNR → more)
    r.levels[2] = lvl (r.humRatioDb,       6.f,   12.f,  20.f);                   // hum
    r.levels[3] = lvl ((float) r.plosiveFrames / juce::jmax (1.f, (float) r.totalFrames) * 100.f, 1.f, 3.f, 6.f);
    r.levels[4] = lvl (r.sibilanceDb,      -9.f,  -5.f,  -1.f);                   // harsh
    r.levels[5] = lvl (r.unevennessDb,     3.f,   5.f,   8.f);                    // uneven
    r.levels[6] = lvl ((float) r.clippedRuns, 1.f, 10.f, 60.f);                   // distorted

    auto f1 = [] (float v) { return juce::String ((int) std::round (v)); };
    r.findings.add ("Voice around " + f1 (r.speechLevelDb) + " dB, noise floor " + f1 (r.noiseFloorDb) + " dB (" + f1 (r.snrDb) + " dB clean)");
    if (r.levels[0]) r.findings.add ("Room reflections ~" + f1 (r.decayMs) + " ms decay - " + Presets::intensities[r.levels[0]].toLowerCase() + " de-reverb");
    if (r.levels[1]) r.findings.add ("Steady background noise - " + Presets::intensities[r.levels[1]].toLowerCase() + " cleanup");
    if (r.levels[2]) r.findings.add ("Electrical hum at " + f1 (r.humFreq) + " Hz");
    if (r.levels[3]) r.findings.add (juce::String (r.plosiveFrames) + " plosive / bump moments");
    if (r.levels[4]) r.findings.add ("Sibilant S sounds (" + f1 (r.sibilanceDb) + " dB bright)");
    if (r.levels[5]) r.findings.add ("Level drifts by +/-" + f1 (r.unevennessDb) + " dB - leveling on");
    if (r.levels[6]) r.findings.add (juce::String (r.clippedRuns) + " clipped moments to repair");
    if (r.findings.size() == 1) r.findings.add ("Sounds clean - light polish only");

    r.valid = true;
    report = r;
    reportReady.store (true);
}
