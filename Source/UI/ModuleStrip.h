#pragma once
#include <JuceHeader.h>

/*  One stage in the Advanced panel: title, on/off toggle, up to three knobs.   */
class ModuleStrip : public juce::Component
{
public:
    struct Knob { juce::String paramID; juce::String label; };

    ModuleStrip (juce::AudioProcessorValueTreeState& apvts, int number, const juce::String& title,
                 const juce::String& onParamID, std::vector<Knob> knobs);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    int number;
    juce::String title;
    juce::ToggleButton onButton;
    std::unique_ptr<BA> onAttachment;

    struct KnobUI { std::unique_ptr<juce::Slider> slider; std::unique_ptr<juce::Label> label; std::unique_ptr<SA> attach; };
    std::vector<KnobUI> knobUIs;
};
