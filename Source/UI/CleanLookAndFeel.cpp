#include "CleanLookAndFeel.h"
#include "Theme.h"

CleanLookAndFeel::CleanLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Theme::background);
    setColour (juce::PopupMenu::backgroundColourId, Theme::surfaceHi);
    setColour (juce::PopupMenu::textColourId, Theme::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, Theme::text);
    setColour (juce::TooltipWindow::backgroundColourId, Theme::surfaceHi);
    setColour (juce::TooltipWindow::textColourId, Theme::text);
    setColour (juce::Slider::textBoxTextColourId, Theme::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, Theme::text);
}

// ------------------------------------------------------------------ rotary -------------
void CleanLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& s)
{
    const bool hero = s.getProperties().contains ("hero");
    const bool enabled = s.isEnabled();
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (hero ? 10.f : 6.f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float lineW = hero ? 7.f : 3.5f;
    const float arcR  = radius - lineW * 0.5f;

    // Knob body — subtle radial shading so it reads as a physical cap.
    const float capR = arcR - lineW - (hero ? 8.f : 4.f);
    {
        juce::ColourGradient body (Theme::surfaceHi.brighter (0.08f), centre.x - capR * 0.4f, centre.y - capR * 0.5f,
                                   Theme::background, centre.x + capR * 0.3f, centre.y + capR * 0.8f, true);
        g.setGradientFill (body);
        g.fillEllipse (centre.x - capR, centre.y - capR, capR * 2.f, capR * 2.f);
        g.setColour (Theme::border);
        g.drawEllipse (centre.x - capR, centre.y - capR, capR * 2.f, capR * 2.f, 1.f);
    }

    // Track arc
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, startAngle, endAngle, true);
    g.setColour (Theme::track);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc — bipolar sliders fill from centre.
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroAngle = bipolar ? startAngle + (float) ((0.0 - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) * (endAngle - startAngle)
                                    : startAngle;
    if (std::abs (angle - zeroAngle) > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, juce::jmin (zeroAngle, angle), juce::jmax (zeroAngle, angle), true);
        auto arcBounds = value.getBounds();
        if (enabled)
        {
            // glow
            g.setColour (Theme::accent.withAlpha (0.22f));
            g.strokePath (value, juce::PathStrokeType (lineW + (hero ? 10.f : 5.f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setGradientFill (Theme::accentGradient (bounds));
        }
        else g.setColour (Theme::textMute);
        g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::ignoreUnused (arcBounds);
    }

    // Pointer line on the cap
    {
        juce::Path p;
        const float len = capR * (hero ? 0.32f : 0.38f);
        p.addRoundedRectangle (-1.2f, -capR + 5.f, 2.4f, len, 1.2f);
        g.setColour (enabled ? Theme::text : Theme::textMute);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (centre));
    }

    if (hero)
    {
        g.setColour (Theme::text);
        g.setFont (Theme::title (26.f));
        g.drawText (juce::String ((int) s.getValue()) + "%", bounds.withSizeKeepingCentre (capR * 2.f, 30.f), juce::Justification::centred);
    }
}

// ------------------------------------------------------------------ linear -------------
void CleanLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle, juce::Slider& s)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float cy = r.getCentreY();
    auto track = juce::Rectangle<float> (r.getX(), cy - 2.f, r.getWidth(), 4.f);
    g.setColour (Theme::track);
    g.fillRoundedRectangle (track, 2.f);

    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float zeroX = bipolar ? r.getX() + (float) ((0.0 - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) * r.getWidth() : r.getX();
    auto fill = juce::Rectangle<float> (juce::jmin (zeroX, pos), cy - 2.f, std::abs (pos - zeroX), 4.f);
    g.setGradientFill (Theme::accentGradient (r));
    g.fillRoundedRectangle (fill, 2.f);

    g.setColour (Theme::text);
    g.fillEllipse (pos - 6.f, cy - 6.f, 12.f, 12.f);
    g.setColour (Theme::background);
    g.fillEllipse (pos - 3.f, cy - 3.f, 6.f, 6.f);
}

juce::Label* CleanLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (Theme::mono (11.5f));
    l->setColour (juce::Label::textColourId, Theme::text);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

// ------------------------------------------------------------------ toggles -------------
void CleanLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool)
{
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat();
    const float h = juce::jmin (18.f, r.getHeight());
    auto pill = juce::Rectangle<float> (r.getX(), r.getCentreY() - h * 0.5f, h * 1.8f, h);

    g.setColour (on ? Theme::accent.withAlpha (hover ? 1.f : 0.9f) : (hover ? Theme::borderHi : Theme::track));
    g.fillRoundedRectangle (pill, h * 0.5f);
    const float knobD = h - 4.f;
    const float kx = on ? pill.getRight() - knobD - 2.f : pill.getX() + 2.f;
    g.setColour (on ? Theme::background : Theme::textDim);
    g.fillEllipse (kx, pill.getY() + 2.f, knobD, knobD);

    if (b.getButtonText().isNotEmpty())
    {
        g.setColour (Theme::text);
        g.setFont (Theme::body (12.f));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (pill.getWidth() + 8.f), juce::Justification::centredLeft);
    }
}

// ------------------------------------------------------------------ buttons -------------
void CleanLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    juce::Colour fill = on ? Theme::accent : (down ? Theme::surface : hover ? Theme::surfaceHi : Theme::surface);
    if (b.getProperties().contains ("warnWhenOn") && on) fill = Theme::warn;

    if (b.getProperties().contains ("primary"))
    {
        // Always-filled call-to-action; while active it goes amber to say "working".
        if (on) { g.setColour (Theme::warn); g.fillRoundedRectangle (r, 6.f); }
        else    { g.setGradientFill (Theme::accentGradient (r)); g.fillRoundedRectangle (r, 6.f); }
        if (hover) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRoundedRectangle (r, 6.f); }
        return;
    }

    g.setColour (fill);
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (on ? fill.brighter (0.2f) : (hover ? Theme::borderHi : Theme::border));
    g.drawRoundedRectangle (r, 6.f, 1.f);
}

juce::Font CleanLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return Theme::label (12.f); }

// ------------------------------------------------------------------ combo -------------
void CleanLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.isMouseOver() ? Theme::surfaceHi : Theme::surface);
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (box.isMouseOver() ? Theme::borderHi : Theme::border);
    g.drawRoundedRectangle (r, 6.f, 1.f);

    juce::Path chevron;
    const float cx = (float) w - 14.f, cy = (float) h * 0.5f;
    chevron.startNewSubPath (cx - 4.f, cy - 2.f);
    chevron.lineTo (cx, cy + 2.f);
    chevron.lineTo (cx + 4.f, cy - 2.f);
    g.setColour (Theme::textDim);
    g.strokePath (chevron, juce::PathStrokeType (1.6f));
}

void CleanLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 34, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setColour (juce::Label::textColourId, Theme::text);
}

juce::Font CleanLookAndFeel::getComboBoxFont (juce::ComboBox&) { return Theme::body (12.5f); }

void CleanLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.setColour (Theme::surfaceHi);
    g.fillRoundedRectangle (0.f, 0.f, (float) w, (float) h, 6.f);
    g.setColour (Theme::borderHi);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.f, (float) h - 1.f, 6.f, 1.f);
}

void CleanLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& l)
{
    // Text only — no box, no outline. Slider value boxes get a faint underline instead.
    g.setColour (l.findColour (juce::Label::textColourId).withMultipliedAlpha (l.isEnabled() ? 1.f : 0.45f));
    g.setFont (l.getFont());
    g.drawFittedText (l.getText(), l.getLocalBounds().reduced (2, 0), l.getJustificationType(), 1, 1.f);
    if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr)
    {
        g.setColour (Theme::border);
        auto b = l.getLocalBounds().toFloat();
        g.fillRect (b.getCentreX() - 14.f, b.getBottom() - 1.f, 28.f, 1.f);
    }
}
