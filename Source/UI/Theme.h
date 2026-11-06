#pragma once
#include <JuceHeader.h>

/*  Pro-NLE palette: near-black surfaces with hairline borders, one vivid accent,
    amber for "attention", red only for what we removed / clipping.            */
namespace Theme
{
    inline const juce::Colour background { 0xff0f1012 };
    inline const juce::Colour surface    { 0xff17191c };   // cards
    inline const juce::Colour surfaceHi  { 0xff1f2226 };   // hovered / raised
    inline const juce::Colour border     { 0xff2a2e33 };
    inline const juce::Colour borderHi   { 0xff3a4047 };
    inline const juce::Colour track      { 0xff262a2f };   // knob/slider background arcs

    inline const juce::Colour text       { 0xffe8eaed };
    inline const juce::Colour textDim    { 0xff8b919a };
    inline const juce::Colour textMute   { 0xff5c626b };

    inline const juce::Colour accent     { 0xff22d3c5 };   // teal-cyan
    inline const juce::Colour accent2    { 0xff4f8cff };   // blue — gradient partner
    inline const juce::Colour warn       { 0xffffb020 };
    inline const juce::Colour removed    { 0xffff5a4f };
    inline const juce::Colour ok         { 0xff3ddc84 };

    inline juce::ColourGradient accentGradient (juce::Rectangle<float> r)
    {
        return juce::ColourGradient (accent, r.getX(), r.getBottom(), accent2, r.getRight(), r.getY(), false);
    }

    inline juce::Font label (float size = 10.5f)  { return juce::Font (juce::FontOptions (size, juce::Font::bold)); }
    inline juce::Font body  (float size = 12.5f)  { return juce::Font (juce::FontOptions (size)); }
    inline juce::Font title (float size = 15.f)   { return juce::Font (juce::FontOptions (size, juce::Font::bold)); }
    inline juce::Font mono  (float size = 12.f)   { return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain)); }
}
