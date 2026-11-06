#include "ProblemCard.h"
#include "Theme.h"

// ------------------------------------------------------------------ Segmented ---------
SegmentedControl::SegmentedControl (juce::RangedAudioParameter& p, const juce::StringArray& l)
    : labels (l),
      attachment (p, [this, &p] (float v) { current = (int) std::round (v); repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
}

void SegmentedControl::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Theme::background);
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.f);

    const int n = labels.size();
    const float w = r.getWidth() / (float) n;
    const int hoverIdx = isMouseOver() ? juce::jlimit (0, n - 1, (int) (getMouseXYRelative().x / w)) : -1;

    for (int i = 0; i < n; ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.f);
        if (i == current)
        {
            // Off is neutral grey; Light→Heavy warms up.
            juce::Colour c = i == 0 ? Theme::surfaceHi.brighter (0.15f)
                                    : Theme::accent.interpolatedWith (Theme::warn, (float) (i - 1) / 2.f);
            g.setColour (c);
            g.fillRoundedRectangle (seg, 4.f);
        }
        else if (i == hoverIdx)
        {
            g.setColour (Theme::surfaceHi);
            g.fillRoundedRectangle (seg, 4.f);
        }
        g.setColour (i == current ? (i == 0 ? Theme::text : Theme::background) : Theme::textDim);
        g.setFont (Theme::label (9.5f));
        g.drawText (labels[i].toUpperCase(), seg, juce::Justification::centred);
    }
}

void SegmentedControl::mouseDown (const juce::MouseEvent& e)
{
    const int n = labels.size();
    const int idx = juce::jlimit (0, n - 1, (int) (e.x / ((float) getWidth() / (float) n)));
    attachment.setValueAsCompleteGesture ((float) idx);
}

// ------------------------------------------------------------------ ProblemCard ---------
ProblemCard::ProblemCard (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                          const juce::String& t, const juce::String& b)
    : title (t), blurb (b), param (*apvts.getParameter (paramID)),
      segments (param, { "Off", "Low", "Mid", "High" })
{
    addAndMakeVisible (segments);
}

bool ProblemCard::isActive() const { return param.getValue() > 0.01f; }

void ProblemCard::paint (juce::Graphics& g)
{
    const bool on = isActive();
    auto r = getLocalBounds().toFloat().reduced (3.f);
    g.setColour (Theme::surface);
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (on ? Theme::borderHi : Theme::border);
    g.drawRoundedRectangle (r, 8.f, 1.f);
    if (on)
    {
        g.setGradientFill (Theme::accentGradient (r));
        g.fillRoundedRectangle (r.getX() + 1.f, r.getY() + 12.f, 2.5f, 18.f, 1.25f);
    }

    auto inner = getLocalBounds().reduced (12, 9);
    g.setColour (on ? Theme::text : Theme::textDim);
    g.setFont (Theme::label (12.f));
    g.drawText (title, inner.removeFromTop (16), juce::Justification::centredLeft);
    g.setColour (Theme::textMute);
    g.setFont (Theme::body (10.5f));
    g.drawText (blurb, inner.removeFromTop (14), juce::Justification::centredLeft);
}

void ProblemCard::resized()
{
    segments.setBounds (getLocalBounds().reduced (12, 9).removeFromBottom (24));
}
