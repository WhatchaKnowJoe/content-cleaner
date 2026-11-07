#include "ProblemCard.h"
#include "Theme.h"

// ------------------------------------------------------------------ Segmented ---------
juce::Colour SegmentedControl::colourForLevel (int level)
{
    if (level <= 0) return Theme::textMute;
    return Theme::accent.interpolatedWith (Theme::warn, (float) (level - 1) / 2.f);
}

SegmentedControl::SegmentedControl (juce::RangedAudioParameter& p, const juce::StringArray& l)
    : labels (l),
      attachment (p, [this] (float v) { current = (int) std::round (v); repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
}

void SegmentedControl::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float rad = r.getHeight() * 0.5f;

    // trough with a faint inner shadow
    g.setColour (Theme::background);
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f).translated (0.f, 0.5f), rad, 1.f);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.f);

    const int n = labels.size();
    const float w = (r.getWidth() - 4.f) / (float) n;
    const int hoverIdx = isMouseOver() ? juce::jlimit (0, n - 1, (int) ((getMouseXYRelative().x - 2) / w)) : -1;

    for (int i = 0; i < n; ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + 2.f + w * (float) i, r.getY() + 2.f, w, r.getHeight() - 4.f);
        if (i == current)
        {
            const juce::Colour c = i == 0 ? Theme::surfaceHi.brighter (0.25f) : colourForLevel (i);
            if (i > 0) { g.setColour (c.withAlpha (0.35f)); g.fillRoundedRectangle (seg.expanded (1.5f), rad); }   // glow
            g.setColour (c);
            g.fillRoundedRectangle (seg, rad - 2.f);
        }
        else if (i == hoverIdx)
        {
            g.setColour (Theme::surfaceHi);
            g.fillRoundedRectangle (seg, rad - 2.f);
        }
        g.setColour (i == current ? (i == 0 ? Theme::text : Theme::background) : Theme::textDim);
        g.setFont (Theme::label (9.5f));
        g.drawText (labels[i].toUpperCase(), seg, juce::Justification::centred);
    }
}

void SegmentedControl::mouseDown (const juce::MouseEvent& e)
{
    const int n = labels.size();
    const int idx = juce::jlimit (0, n - 1, (int) ((e.x - 2) / ((float) (getWidth() - 4) / (float) n)));
    attachment.setValueAsCompleteGesture ((float) idx);
}

// ------------------------------------------------------------------ Icons -----------------
void ProblemCard::drawIcon (juce::Graphics& g, int index, juce::Rectangle<float> r)
{
    juce::Path p;
    const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight(), cy = r.getCentreY();
    juce::PathStrokeType stroke (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    switch (index)
    {
        case 0: // Echoey room: three expanding arcs
            for (int i = 0; i < 3; ++i)
            {
                const float rad = w * (0.18f + 0.16f * (float) i);
                p.addCentredArc (x + w * 0.2f, cy, rad, rad, 0.f, 0.6f, 2.5f, true);
            }
            g.strokePath (p, stroke);
            break;
        case 1: // Background noise: jagged noise line
            p.startNewSubPath (x, cy);
            for (int i = 1; i <= 10; ++i)
                p.lineTo (x + w * (float) i / 10.f, cy + h * 0.32f * ((i * 7) % 5 - 2) / 2.f);
            g.strokePath (p, stroke);
            break;
        case 2: // Hum: smooth sine
            p.startNewSubPath (x, cy);
            for (int i = 1; i <= 24; ++i)
                p.lineTo (x + w * (float) i / 24.f, cy - h * 0.34f * std::sin ((float) i / 24.f * juce::MathConstants<float>::twoPi * 2.f));
            g.strokePath (p, stroke);
            break;
        case 3: // Pops: flat line with one burst
            p.startNewSubPath (x, cy); p.lineTo (x + w * 0.38f, cy);
            p.lineTo (x + w * 0.46f, y + h * 0.08f); p.lineTo (x + w * 0.54f, y + h * 0.92f); p.lineTo (x + w * 0.62f, cy);
            p.lineTo (x + w, cy);
            g.strokePath (p, stroke);
            break;
        case 4: // Harsh S: dense high-frequency zigzag
            p.startNewSubPath (x, cy);
            for (int i = 1; i <= 7; ++i)
                p.lineTo (x + w * (float) i / 7.f, cy + ((i % 2) ? -1.f : 1.f) * h * 0.4f);
            g.strokePath (p, stroke);
            break;
        case 5: // Uneven: bars of varying height
            for (int i = 0; i < 6; ++i)
            {
                const float bh = h * (0.25f + 0.65f * (((i * 5) % 4) / 3.f));
                g.fillRoundedRectangle (x + w * (float) i / 6.f + 1.f, r.getBottom() - bh, w / 6.f - 2.5f, bh, 1.f);
            }
            break;
        default: // Distorted: clipped waveform
            p.startNewSubPath (x, cy);
            p.lineTo (x + w * 0.15f, y + h * 0.1f); p.lineTo (x + w * 0.4f, y + h * 0.1f);
            p.lineTo (x + w * 0.55f, y + h * 0.9f); p.lineTo (x + w * 0.8f, y + h * 0.9f);
            p.lineTo (x + w, cy);
            g.strokePath (p, stroke);
            break;
    }
}

// ------------------------------------------------------------------ ProblemCard ---------
ProblemCard::ProblemCard (juce::AudioProcessorValueTreeState& apvts, int idx, const juce::String& paramID,
                          const juce::String& t, const juce::String& b)
    : index (idx), title (t), blurb (b), param (*apvts.getParameter (paramID)),
      segments (param, { "Off", "Low", "Mid", "High" })
{
    addAndMakeVisible (segments);
}

int ProblemCard::level() const { return segments.getCurrent(); }

void ProblemCard::paint (juce::Graphics& g)
{
    const int L = level();
    const bool on = L > 0;
    const juce::Colour lc = SegmentedControl::colourForLevel (L);
    auto r = getLocalBounds().toFloat().reduced (4.f);

    // Card: subtle vertical gradient, tinted by the level colour when active.
    juce::ColourGradient bg (Theme::surfaceHi.interpolatedWith (lc, on ? 0.06f : 0.f), 0.f, r.getY(),
                             Theme::surface, 0.f, r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 10.f);
    g.setColour (on ? lc.withAlpha (0.45f) : Theme::border);
    g.drawRoundedRectangle (r, 10.f, 1.f);

    auto inner = getLocalBounds().reduced (14, 11);
    auto head = inner.removeFromTop (22);

    // icon tile (left) + status chip (right)
    auto tile = head.removeFromLeft (24).withHeight (24).toFloat();
    g.setColour (on ? lc.withAlpha (0.15f) : Theme::background);
    g.fillRoundedRectangle (tile, 6.f);
    g.setColour (on ? lc : Theme::textMute);
    drawIcon (g, index, tile.reduced (5.f, 6.f));

    auto chip = head.removeFromRight (40).withHeight (16).toFloat().translated (0.f, 4.f);
    g.setColour (on ? lc.withAlpha (0.18f) : Theme::background);
    g.fillRoundedRectangle (chip, 8.f);
    g.setColour (on ? lc : Theme::textMute);
    g.setFont (Theme::label (8.5f));
    g.drawText (juce::StringArray { "OFF", "LOW", "MID", "HIGH" }[L], chip, juce::Justification::centred);

    inner.removeFromTop (6);
    g.setColour (on ? Theme::text : Theme::textDim);
    g.setFont (Theme::label (12.5f));
    g.drawText (title, inner.removeFromTop (16), juce::Justification::centredLeft, true);

    g.setColour (Theme::textDim.withAlpha (on ? 1.f : 0.7f));
    g.setFont (Theme::body (10.5f));
    g.drawText (blurb, inner.removeFromTop (14), juce::Justification::centredLeft, true);
}

void ProblemCard::resized()
{
    segments.setBounds (getLocalBounds().reduced (14, 11).removeFromBottom (26));
}
