#pragma once
#include <JuceHeader.h>

/*  Segmented Off / Light / Medium / Heavy control bound to an AudioParameterChoice. */
class SegmentedControl : public juce::Component
{
public:
    SegmentedControl (juce::RangedAudioParameter& param, const juce::StringArray& labels);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    juce::StringArray labels;
    int current = 0;
    juce::ParameterAttachment attachment;
};

/*  One creator-facing problem: title, one-line blurb, segmented intensity.        */
class ProblemCard : public juce::Component
{
public:
    ProblemCard (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                 const juce::String& title, const juce::String& blurb);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isActive() const;

private:
    juce::String title, blurb;
    juce::RangedAudioParameter& param;
    SegmentedControl segments;
};
