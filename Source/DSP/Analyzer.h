#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>

/*  Auto-Analyze: listens to the *unprocessed* input for a few seconds and
    measures what is wrong with it, then recommends a problem level (0–3) for
    each creator-facing problem. Runs on the audio thread; cheap (no FFT).

    Measurements
      noiseFloorDb   10th-percentile 20 ms RMS                      → Background noise
      speechLevelDb  85th-percentile 20 ms RMS
      snrDb          speech − noise
      decayMs        how long energy takes to fall 20 dB after speech → Echoey room
      humRatioDb     Goertzel power at 50/60 Hz (+2nd harmonic) vs. neighbours → Hum
      clippedRuns    runs of ≥3 consecutive near-full-scale samples    → Distorted
      sibilanceDb    5–10 kHz vs 1–4 kHz energy during speech          → Harsh S
      plosiveFrames  frames where <150 Hz energy dominates             → Pops
      unevennessDb   std-dev of speech-frame levels                    → Uneven     */
class Analyzer
{
public:
    struct Report
    {
        bool  valid = false;
        float noiseFloorDb = -90.f, speechLevelDb = -90.f, snrDb = 0.f;
        float decayMs = 0.f, humRatioDb = 0.f, humFreq = 60.f, sibilanceDb = -20.f, unevennessDb = 0.f;
        int   clippedRuns = 0, plosiveFrames = 0, totalFrames = 0;
        float speechLUFSApprox = -30.f;

        // Recommended levels (0 Off … 3 Heavy), in Presets::problems order.
        std::array<int, 7> levels { 0, 0, 0, 0, 0, 0, 0 };
        juce::StringArray findings;      // plain-English lines for the UI
    };

    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();

    /** Call from the audio thread with the raw input block (before processing). */
    void process (const juce::AudioBuffer<float>& input);

    void start (float seconds);                          // any thread
    bool isRunning() const noexcept { return running.load(); }
    bool hasNewReport() const noexcept { return reportReady.load(); }
    Report takeReport();                                 // message thread; clears the flag

private:
    void finish();
    static float percentile (std::vector<float> v, float p);

    double sr = 48000.0;
    int frameLen = 960, framePos = 0, framesWanted = 0;
    float frameSumSq = 0.f, frameLowSq = 0.f, frameMidSq = 0.f, frameHiSq = 0.f;

    // cheap filters on the mono mix
    juce::dsp::IIR::Filter<float> lowLP, midBP, hiBP;
    int clipRun = 0, clippedRuns = 0;

    // Goertzel accumulators: 50, 55, 60, 65, 100, 120 Hz
    struct Goertzel { float coeff = 0.f, s1 = 0.f, s2 = 0.f; double power = 0.0; };
    std::array<Goertzel, 6> gz;
    std::array<float, 6> gzFreqs { 50.f, 55.f, 60.f, 65.f, 100.f, 120.f };
    double totalPower = 0.0;

    std::vector<float> frameDb, lowDb, midDb, hiDb;

    std::atomic<bool> running { false }, reportReady { false };
    Report report;
};
