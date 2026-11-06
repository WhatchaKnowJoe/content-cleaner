#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "DSP/ModuleChain.h"
#include "DSP/Analyzer.h"

class ContentCleanerProcessor : public juce::AudioProcessor,
                                private juce::AudioProcessorValueTreeState::Listener,
                                private juce::Timer
{
public:
    ContentCleanerProcessor();
    ~ContentCleanerProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- For the editor ---------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    ModuleChain& getChain() noexcept { return chain; }
    bool isLearningRoom() const noexcept { return chain.getSpectral().isLearning(); }
    bool hasRoomProfile() const noexcept { return chain.getSpectral().hasLearnedProfile(); }

    // Metering (atomic, written on audio thread)
    std::atomic<float> inputPeakDb { -100.f }, outputPeakDb { -100.f };

    void applyMicPreset (int presetIndex);
    void applyProblem (const juce::String& problemID, int level);   // writes module params

    bool isAnalyzing() const noexcept { return analyzer.isRunning(); }
    Analyzer::Report getLastReport() const { const juce::ScopedLock sl (reportLock); return lastReport; }

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;
    ModuleChain::Settings readSettings() const;

    ModuleChain chain;
    Analyzer analyzer;
    std::atomic<bool> learnRequested { false }, analyzeRequested { false };
    bool applyingPreset = false;
    juce::CriticalSection reportLock;
    Analyzer::Report lastReport;
    juce::AudioBuffer<float> analysisCopy;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentCleanerProcessor)
};
