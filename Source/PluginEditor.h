#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ModuleStrip.h"
#include "UI/ProblemCard.h"
#include "UI/CleanLookAndFeel.h"

/*  Simple view = creator language: Analyze, seven problems, Clean, Natural↔Aggressive.
    Advanced view = the 12 engineering stages underneath.                               */
class ContentCleanerEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ContentCleanerEditor (ContentCleanerProcessor&);
    ~ContentCleanerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawHeader (juce::Graphics& g, juce::Rectangle<int> r);
    void drawMeter (juce::Graphics& g, juce::Rectangle<int> r, float db, float& hold, const juce::String& label);
    void drawSpectralDisplay (juce::Graphics& g, juce::Rectangle<int> r);
    void drawStatusPill (juce::Graphics& g, juce::Rectangle<int> r);
    void drawReportCard (juce::Graphics& g, juce::Rectangle<int> r);
    void drawCharacterLabels (juce::Graphics& g);

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ContentCleanerProcessor& processor;
    CleanLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    juce::Slider cleanDial, characterSlider, outputSlider;
    juce::TextButton analyzeButton { "ANALYZE" }, learnButton { "LEARN ROOM" }, advancedButton { "ADVANCED" };
    juce::ToggleButton deltaButton { "Hear what's removed" };
    juce::ComboBox presetBox, targetBox;

    std::unique_ptr<SA> cleanAttach, characterAttach, outputAttach;
    std::unique_ptr<BA> analyzeAttach, learnAttach, deltaAttach;
    std::unique_ptr<CA> presetAttach, targetAttach;

    std::vector<std::unique_ptr<ProblemCard>> problemCards;
    std::vector<std::unique_ptr<ModuleStrip>> strips;
    bool advancedVisible = false;

    juce::Rectangle<int> headerArea, inMeterArea, outMeterArea, spectralArea, statusArea, dialArea,
                         reportArea, characterArea, outputLabelArea;
    float inHold = -100.f, outHold = -100.f;
    std::vector<float> smoothedGains;

    static constexpr int width = 980, headerH = 48, simpleH = 404, stripH = 168, advancedH = simpleH + 2 * stripH + 36;
};
