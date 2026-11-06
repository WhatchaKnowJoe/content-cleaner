#include "ModuleStrip.h"
#include "Theme.h"

ModuleStrip::ModuleStrip (juce::AudioProcessorValueTreeState& apvts, int num, const juce::String& t,
                          const juce::String& onParamID, std::vector<Knob> knobs)
    : number (num), title (t)
{
    onButton.setButtonText ({});
    onButton.onStateChange = [this] { repaint(); for (auto& k : knobUIs) k.slider->setEnabled (onButton.getToggleState()); };
    addAndMakeVisible (onButton);
    if (onParamID.isNotEmpty())
        onAttachment = std::make_unique<BA> (apvts, onParamID, onButton);

    for (auto& k : knobs)
    {
        KnobUI ui;
        ui.slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        ui.slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        ui.slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        ui.label = std::make_unique<juce::Label> (juce::String(), k.label.toUpperCase());
        ui.label->setJustificationType (juce::Justification::centred);
        ui.label->setFont (Theme::label (9.5f));
        ui.label->setColour (juce::Label::textColourId, Theme::textDim);
        addAndMakeVisible (*ui.slider);
        addAndMakeVisible (*ui.label);
        ui.attach = std::make_unique<SA> (apvts, k.paramID, *ui.slider);
        knobUIs.push_back (std::move (ui));
    }
    for (auto& k : knobUIs) k.slider->setEnabled (onButton.getToggleState());
}

void ModuleStrip::paint (juce::Graphics& g)
{
    const bool on = onButton.getToggleState();
    auto r = getLocalBounds().toFloat().reduced (3.f);

    g.setColour (Theme::surface);
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (on ? Theme::border : Theme::border.withAlpha (0.6f));
    g.drawRoundedRectangle (r, 8.f, 1.f);

    // Accent tick on the left edge when active
    if (on)
    {
        g.setGradientFill (Theme::accentGradient (r));
        g.fillRoundedRectangle (r.getX() + 1.f, r.getY() + 10.f, 2.5f, 16.f, 1.25f);
    }

    auto head = getLocalBounds().reduced (10, 6).removeFromTop (22);
    g.setFont (Theme::mono (10.f));
    g.setColour (on ? Theme::accent : Theme::textMute);
    g.drawText (juce::String (number).paddedLeft ('0', 2), head.removeFromLeft (20), juce::Justification::centredLeft);

    g.setFont (Theme::label (11.5f));
    g.setColour (on ? Theme::text : Theme::textDim);
    g.drawText (title.toUpperCase(), head.withTrimmedRight (40), juce::Justification::centredLeft);
}

void ModuleStrip::resized()
{
    auto r = getLocalBounds().reduced (8, 6);
    auto top = r.removeFromTop (22);
    onButton.setBounds (top.removeFromRight (34));
    r.removeFromTop (2);

    if (knobUIs.empty()) return;
    const int w = r.getWidth() / (int) knobUIs.size();
    for (auto& k : knobUIs)
    {
        auto cell = r.removeFromLeft (w);
        k.label->setBounds (cell.removeFromBottom (14));
        k.slider->setBounds (cell);
    }
}
