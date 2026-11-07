#pragma once
#include <JuceHeader.h>

/*  Pill-style Off / Low / Mid / High control bound to an AudioParameterChoice.
    Selected segment warms from teal (Low) to amber (High).                      */
class SegmentedControl : public juce::Component
{
public:
    SegmentedControl (juce::RangedAudioParameter& param, const juce::StringArray& labels);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    int  getCurrent() const noexcept { return current; }

    static juce::Colour colourForLevel (int level);

private:
    juce::StringArray labels;
    int current = 0;
    juce::ParameterAttachment attachment;
};

/*  One creator-facing problem: icon, title, blurb, status chip, segmented intensity. */
class ProblemCard : public juce::Component
{
public:
    ProblemCard (juce::AudioProcessorValueTreeState& apvts, int problemIndex, const juce::String& paramID,
                 const juce::String& title, const juce::String& blurb);
    void paint (juce::Graphics&) override;
    void resized() override;
    int  level() const;

    /** Draws the glyph for problem `index` inside `r` using the current colour. */
    static void drawIcon (juce::Graphics& g, int index, juce::Rectangle<float> r);

private:
    int index;
    juce::String title, blurb;
    juce::RangedAudioParameter& param;
    SegmentedControl segments;
};
