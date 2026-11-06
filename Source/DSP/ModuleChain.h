#pragma once
#include "ModuleBase.h"
#include "HumRemover.h"
#include "SpectralDenoiser.h"
#include "Gate.h"
#include "DeClicker.h"
#include "ClipRepair.h"
#include "VoiceEQ.h"
#include "DeEsser.h"
#include "MultibandComp.h"
#include "Leveler.h"
#include "LoudnessStage.h"

/*  The full 12-stage chain in repair-then-polish order:

      1 HPF → 2 Hum → 3 Denoise ─┐ (one STFT) → 5 Gate → 6 De-click/Plosive → 7 Clip Repair
                      4 Dereverb ─┘
      → 8 Voice EQ → 9 De-esser → 10 Multiband → 11 Leveler → 12 Loudness + Limiter

    Also keeps a latency-matched dry copy so "Delta Listen" can output
    (dry − wet) = exactly what the plugin removed.                               */
class ModuleChain
{
public:
    struct Settings
    {
        float clean = 0.5f;            // macro 0..1 (0.5 = params as set)
        float character = 0.35f;       // 0 natural .. 1 aggressive
        bool  deltaListen = false;
        float outputGainDb = 0.f;

        bool  hpfOn = true;   float hpfFreq = 80.f;
        bool  humOn = false;  float humFreq = 60.f, humDepth = 30.f;
        bool  dnOn = true;    float dnAmount = 0.5f;
        bool  drOn = true;    float drAmount = 0.3f;
        bool  gateOn = true;  float gateThresh = -50.f, gateRange = 12.f, gateRelease = 150.f;
        bool  dcOn = false;   float dcSens = 0.5f;   bool plosiveOn = true;
        bool  clipOn = false; float clipThresh = -0.3f;
        bool  eqOn = true;    float eqMud = -2.f, eqPresence = 1.5f, eqAir = 1.f;
        bool  dsOn = true;    float dsFreq = 6500.f, dsAmount = 0.4f;
        bool  mbOn = true;    float mbAmount = 0.4f;
        bool  lvOn = true;    float lvAmount = 0.5f, lvTarget = -18.f;
        bool  limOn = true;   float limCeiling = -1.f, targetLUFS = -14.f;
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void applySettings (const Settings& s);
    void process (juce::AudioBuffer<float>& buffer);
    int  getLatencySamples() const;

    SpectralDenoiser& getSpectral() noexcept { return spectral; }
    const SpectralDenoiser& getSpectral() const noexcept { return spectral; }
    Gate&             getGate() noexcept     { return gate; }
    Leveler&          getLeveler() noexcept  { return leveler; }
    LoudnessStage&    getLoudness() noexcept { return loudness; }

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    Filter hpf;
    HumRemover hum;
    SpectralDenoiser spectral;
    Gate gate;
    DeClicker declick;
    ClipRepair clip;
    VoiceEQ eq;
    DeEsser deess;
    MultibandComp multiband;
    Leveler leveler;
    LoudnessStage loudness;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 1 << 15 };
    juce::AudioBuffer<float> dryBuffer;
    juce::dsp::Gain<float> outputGain;

    double sampleRate = 48000.0;
    float lastHpfFreq = -1.f;
    bool hpfOn = true, deltaListen = false;
};
