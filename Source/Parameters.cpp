#include "Parameters.h"

using namespace juce;
using APF  = AudioParameterFloat;
using APB  = AudioParameterBool;
using APC  = AudioParameterChoice;
using Rng  = NormalisableRange<float>;

static std::unique_ptr<APF> f (const char* id, const String& name, Rng r, float def, const String& unit = {})
{
    return std::make_unique<APF> (ParameterID { id, 1 }, name, r, def,
        AudioParameterFloatAttributes().withLabel (unit));
}
static std::unique_ptr<APB> b (const char* id, const String& name, bool def)
{
    return std::make_unique<APB> (ParameterID { id, 1 }, name, def);
}

AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout L;

    // Global
    L.add (f (ParamID::clean, "Clean", Rng (0.f, 100.f, 1.f), 50.f, "%"));
    L.add (std::make_unique<APC> (ParameterID { ParamID::preset, 1 }, "Mic Preset", Presets::micPresets, 0));
    L.add (std::make_unique<APC> (ParameterID { ParamID::target, 1 }, "Loudness Target", Presets::loudnessTargets, 0));
    L.add (b (ParamID::deltaListen, "Delta Listen", false));
    L.add (f (ParamID::outputGain, "Output", Rng (-24.f, 24.f, 0.1f), 0.f, "dB"));
    L.add (f (ParamID::character, "Natural / Aggressive", Rng (0.f, 100.f, 1.f), 35.f, "%"));
    L.add (b (ParamID::analyze, "Analyze", false));

    for (auto& pr : Presets::problems)
        L.add (std::make_unique<APC> (ParameterID { pr.id, 1 }, pr.title, Presets::intensities,
                                      juce::String (pr.id) == ParamID::pNoise || juce::String (pr.id) == ParamID::pRoom ? 2 : 0));

    // 1 HPF
    L.add (b (ParamID::hpfOn, "HPF On", true));
    L.add (f (ParamID::hpfFreq, "HPF Freq", Rng (20.f, 300.f, 1.f, 0.5f), 80.f, "Hz"));

    // 2 Hum
    L.add (b (ParamID::humOn, "Hum On", false));
    L.add (std::make_unique<APC> (ParameterID { ParamID::humFreq, 1 }, "Hum Freq", StringArray { "50 Hz", "60 Hz" }, 1));
    L.add (f (ParamID::humDepth, "Hum Depth", Rng (0.f, 60.f, 0.5f), 30.f, "dB"));

    // 3 Denoise
    L.add (b (ParamID::dnOn, "Denoise On", true));
    L.add (f (ParamID::dnAmount, "Denoise", Rng (0.f, 100.f, 1.f), 50.f, "%"));
    L.add (b (ParamID::dnLearn, "Learn Room", false));

    // 4 De-reverb
    L.add (b (ParamID::drOn, "De-reverb On", true));
    L.add (f (ParamID::drAmount, "De-reverb", Rng (0.f, 100.f, 1.f), 30.f, "%"));

    // 5 Gate
    L.add (b (ParamID::gateOn, "Gate On", true));
    L.add (f (ParamID::gateThresh,  "Gate Threshold", Rng (-80.f, 0.f, 0.5f), -50.f, "dB"));
    L.add (f (ParamID::gateRange,   "Gate Range",     Rng (0.f, 40.f, 0.5f),  12.f,  "dB"));
    L.add (f (ParamID::gateRelease, "Gate Release",   Rng (20.f, 1000.f, 1.f, 0.4f), 150.f, "ms"));

    // 6 De-click
    L.add (b (ParamID::dcOn, "De-click On", false));
    L.add (f (ParamID::dcSens, "De-click Sensitivity", Rng (0.f, 100.f, 1.f), 50.f, "%"));
    L.add (b (ParamID::plosiveOn, "Plosive Tamer", true));

    // 7 Clip repair
    L.add (b (ParamID::clipOn, "Clip Repair On", false));
    L.add (f (ParamID::clipThresh, "Clip Threshold", Rng (-6.f, 0.f, 0.1f), -0.3f, "dB"));

    // 8 EQ
    L.add (b (ParamID::eqOn, "EQ On", true));
    L.add (f (ParamID::eqLowCut,   "Mud Cut",  Rng (-12.f, 0.f, 0.1f), -2.f, "dB"));
    L.add (f (ParamID::eqPresence, "Presence", Rng (-6.f, 6.f, 0.1f),   1.5f, "dB"));
    L.add (f (ParamID::eqAir,      "Air",      Rng (-6.f, 6.f, 0.1f),   1.f,  "dB"));

    // 9 De-esser
    L.add (b (ParamID::dsOn, "De-esser On", true));
    L.add (f (ParamID::dsFreq,   "De-ess Freq", Rng (3000.f, 12000.f, 10.f, 0.5f), 6500.f, "Hz"));
    L.add (f (ParamID::dsAmount, "De-ess",      Rng (0.f, 100.f, 1.f), 40.f, "%"));

    // 10 Multiband comp
    L.add (b (ParamID::mbOn, "Multiband On", true));
    L.add (f (ParamID::mbAmount, "Compression", Rng (0.f, 100.f, 1.f), 40.f, "%"));

    // 11 Leveler
    L.add (b (ParamID::lvOn, "Leveler On", true));
    L.add (f (ParamID::lvAmount, "Leveler", Rng (0.f, 100.f, 1.f), 50.f, "%"));
    L.add (f (ParamID::lvTarget, "Level Target", Rng (-30.f, -10.f, 0.5f), -18.f, "dBFS"));

    // 12 Limiter
    L.add (b (ParamID::limOn, "Limiter On", true));
    L.add (f (ParamID::limCeiling, "Ceiling", Rng (-6.f, 0.f, 0.1f), -1.f, "dBTP"));

    return L;
}
