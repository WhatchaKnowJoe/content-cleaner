cmake_minimum_required(VERSION 3.22)
project(ContentCleaner VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE: point JUCE_DIR at a local checkout, or let FetchContent grab it.
# ---------------------------------------------------------------------------
if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(ContentCleaner
    COMPANY_NAME                "YourCompany"
    BUNDLE_ID                   "com.yourcompany.contentcleaner"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Ycmp
    PLUGIN_CODE                 Ccln
    FORMATS                     VST3 AU Standalone      # AU is ignored on non-Apple
    VST3_CATEGORIES             Fx Restoration Dynamics EQ
    AU_MAIN_TYPE                kAudioUnitType_Effect
    PRODUCT_NAME                "Content Cleaner")

juce_generate_juce_header(ContentCleaner)

target_sources(ContentCleaner PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/UI/ModuleStrip.cpp
    Source/UI/CleanLookAndFeel.cpp
    Source/UI/ProblemCard.cpp
    Source/DSP/ModuleChain.cpp
    Source/DSP/Analyzer.cpp
    Source/DSP/SpectralDenoiser.cpp
    Source/DSP/Dereverb.cpp
    Source/DSP/HumRemover.cpp
    Source/DSP/Gate.cpp
    Source/DSP/DeClicker.cpp
    Source/DSP/ClipRepair.cpp
    Source/DSP/VoiceEQ.cpp
    Source/DSP/DeEsser.cpp
    Source/DSP/MultibandComp.cpp
    Source/DSP/Leveler.cpp
    Source/DSP/LoudnessStage.cpp)

target_include_directories(ContentCleaner PRIVATE Source)

target_compile_definitions(ContentCleaner PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ContentCleaner
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Offline analyzer test: synthesises a "bad recording" and checks detections.
# ---------------------------------------------------------------------------
juce_add_console_app(AnalyzerTest PRODUCT_NAME "AnalyzerTest")
juce_generate_juce_header(AnalyzerTest)
target_sources(AnalyzerTest PRIVATE Tests/AnalyzerTest.cpp Source/DSP/Analyzer.cpp Source/DSP/SpectralDenoiser.cpp Source/DSP/Dereverb.cpp)
target_include_directories(AnalyzerTest PRIVATE Source)
target_compile_definitions(AnalyzerTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(AnalyzerTest PRIVATE juce::juce_dsp juce::juce_audio_processors PUBLIC juce::juce_recommended_config_flags)
