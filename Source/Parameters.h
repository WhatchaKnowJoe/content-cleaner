#pragma once
#include <JuceHeader.h>
#include <array>

/*  Every parameter ID in one place. Modules read these; the editor binds to them.
    Naming: <module>_<param>. Each module has an "_on" toggle so the UI can
    bypass any stage independently.                                              */
namespace ParamID
{
    // Global
    inline constexpr auto clean       = "clean";        // one-knob macro 0..100
    inline constexpr auto preset      = "preset";       // mic / situation preset
    inline constexpr auto target      = "target";       // loudness target choice
    inline constexpr auto deltaListen = "delta";        // hear what is being removed
    inline constexpr auto outputGain  = "output_gain";
    inline constexpr auto character   = "character";    // 0 = natural .. 100 = aggressive
    inline constexpr auto analyze     = "analyze";      // momentary: run Auto-Analyze

    // Problem layer (what the creator sees). Each: Off / Light / Medium / Heavy.
    inline constexpr auto pRoom      = "p_room";        // "Echoey room"
    inline constexpr auto pNoise     = "p_noise";       // "Background noise"
    inline constexpr auto pHum       = "p_hum";         // "Hum / buzz"
    inline constexpr auto pPops      = "p_pops";        // "Pops & clicks"
    inline constexpr auto pHarsh     = "p_harsh";       // "Harsh S sounds"
    inline constexpr auto pUneven    = "p_uneven";      // "Uneven / too quiet"
    inline constexpr auto pDistorted = "p_distorted";   // "Distorted / clipped"

    // 1. High-pass / rumble
    inline constexpr auto hpfOn   = "hpf_on";
    inline constexpr auto hpfFreq = "hpf_freq";

    // 2. Hum
    inline constexpr auto humOn    = "hum_on";
    inline constexpr auto humFreq  = "hum_freq";        // 50 / 60
    inline constexpr auto humDepth = "hum_depth";

    // 3. Denoise
    inline constexpr auto dnOn     = "dn_on";
    inline constexpr auto dnAmount = "dn_amount";
    inline constexpr auto dnLearn  = "dn_learn";        // momentary: capture room tone

    // 4. De-reverb
    inline constexpr auto drOn     = "dr_on";
    inline constexpr auto drAmount = "dr_amount";

    // 5. Gate / expander
    inline constexpr auto gateOn      = "gate_on";
    inline constexpr auto gateThresh  = "gate_thresh";
    inline constexpr auto gateRange   = "gate_range";
    inline constexpr auto gateRelease = "gate_release";

    // 6. De-click / plosive / mouth noise
    inline constexpr auto dcOn      = "dc_on";
    inline constexpr auto dcSens    = "dc_sens";
    inline constexpr auto plosiveOn = "plosive_on";

    // 7. Clip repair
    inline constexpr auto clipOn     = "clip_on";
    inline constexpr auto clipThresh = "clip_thresh";

    // 8. Voice EQ
    inline constexpr auto eqOn       = "eq_on";
    inline constexpr auto eqLowCut   = "eq_lowcut";     // mud/boxiness cut dB
    inline constexpr auto eqPresence = "eq_presence";   // 3-5k lift dB
    inline constexpr auto eqAir      = "eq_air";        // 10k+ shelf dB

    // 9. De-esser
    inline constexpr auto dsOn     = "ds_on";
    inline constexpr auto dsFreq   = "ds_freq";
    inline constexpr auto dsAmount = "ds_amount";

    // 10. Multiband compressor
    inline constexpr auto mbOn     = "mb_on";
    inline constexpr auto mbAmount = "mb_amount";

    // 11. Leveler
    inline constexpr auto lvOn     = "lv_on";
    inline constexpr auto lvAmount = "lv_amount";
    inline constexpr auto lvTarget = "lv_target";       // dBFS RMS to ride toward

    // 12. Loudness / limiter
    inline constexpr auto limOn      = "lim_on";
    inline constexpr auto limCeiling = "lim_ceiling";
}

namespace Presets
{
    inline const juce::StringArray micPresets {
        "Lavalier", "Shotgun / Boom", "On-Camera Mic", "USB / Podcast",
        "Phone Recording", "Zoom / Webcam", "Custom" };

    inline const juce::StringArray loudnessTargets {
        "YouTube  (-14 LUFS)", "Podcast  (-16 LUFS)", "Social / Reels (-14 LUFS)",
        "Broadcast  (-23 LUFS)", "Off" };

    inline const juce::StringArray intensities { "Off", "Light", "Medium", "Heavy" };

    struct Problem { const char* id; const char* title; const char* blurb; };
    inline const std::array<Problem, 7> problems {{
        { "p_room",      "Echoey room",        "Reflections, big-room sound" },
        { "p_noise",     "Background noise",   "AC, traffic, hiss, fans" },
        { "p_hum",       "Hum / buzz",         "Electrical 50 / 60 Hz buzz" },
        { "p_pops",      "Pops & clicks",      "P-pops, clicks, mic bumps" },
        { "p_harsh",     "Harsh S sounds",     "Sibilance, piercing highs" },
        { "p_uneven",    "Uneven / quiet",     "Volume drifts, off-mic" },
        { "p_distorted", "Distorted",          "Clipped or overloaded" } }};

    inline float targetLUFS (int index)
    {
        switch (index) { case 0: return -14.f; case 1: return -16.f;
                         case 2: return -14.f; case 3: return -23.f; default: return 0.f; }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
