#pragma once
#include <JuceHeader.h>

/*  Custom look: thin glowing arcs on dark knobs, pill toggles, flat buttons
    with hairline borders, NLE-style combo boxes.                                */
class CleanLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CleanLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float min, float max,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool hover, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool hover, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int height) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

    // Set a per-slider flag to draw the "hero" style (bigger, value inside).
    static constexpr int heroStyleFlag = 0x4e3a;    // slider.getProperties()["hero"]
};
