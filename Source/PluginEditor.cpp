#include "PluginEditor.h"
#include "UI/Theme.h"

ContentCleanerEditor::ContentCleanerEditor (ContentCleanerProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    auto& apvts = processor.apvts;

    // --- Hero dial ----------------------------------------------------------------------
    cleanDial.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    cleanDial.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    cleanDial.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    cleanDial.getProperties().set ("hero", true);
    cleanDial.setTooltip ("How much to clean overall. Start at 50 and nudge.");
    addAndMakeVisible (cleanDial);
    cleanAttach = std::make_unique<SA> (apvts, ParamID::clean, cleanDial);

    // --- Natural ↔ Aggressive --------------------------------------------------------------
    characterSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    characterSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    characterSlider.setTooltip ("Natural keeps the voice untouched-sounding. Aggressive removes more but risks artefacts.");
    addAndMakeVisible (characterSlider);
    characterAttach = std::make_unique<SA> (apvts, ParamID::character, characterSlider);

    outputSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    outputSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 18);
    outputSlider.setTextValueSuffix (" dB");
    addAndMakeVisible (outputSlider);
    outputAttach = std::make_unique<SA> (apvts, ParamID::outputGain, outputSlider);

    // --- Buttons ------------------------------------------------------------------------------
    analyzeButton.setClickingTogglesState (true);
    analyzeButton.getProperties().set ("primary", true);
    analyzeButton.setTooltip ("Play the clip and press this. Listens for 5 seconds and sets the problems for you.");
    addAndMakeVisible (analyzeButton);
    analyzeAttach = std::make_unique<BA> (apvts, ParamID::analyze, analyzeButton);

    learnButton.setClickingTogglesState (true);
    learnButton.getProperties().set ("warnWhenOn", true);
    learnButton.setTooltip ("Optional: play 2 seconds of silence (just the room) and press this for a better noise fingerprint.");
    addAndMakeVisible (learnButton);
    learnAttach = std::make_unique<BA> (apvts, ParamID::dnLearn, learnButton);

    deltaButton.setTooltip ("Solo what the plugin is taking out. If you hear your voice in here, back off.");
    addAndMakeVisible (deltaButton);
    deltaAttach = std::make_unique<BA> (apvts, ParamID::deltaListen, deltaButton);

    for (auto* box : { &presetBox, &targetBox }) addAndMakeVisible (*box);
    presetBox.addItemList (Presets::micPresets, 1);
    targetBox.addItemList (Presets::loudnessTargets, 1);
    presetAttach = std::make_unique<CA> (apvts, ParamID::preset, presetBox);
    targetAttach = std::make_unique<CA> (apvts, ParamID::target, targetBox);

    advancedButton.setClickingTogglesState (true);
    advancedButton.onClick = [this]
    {
        advancedVisible = advancedButton.getToggleState();
        for (auto& s : strips) s->setVisible (advancedVisible);
        setSize (width, advancedVisible ? advancedH : simpleH);
    };
    addAndMakeVisible (advancedButton);

    // --- Problem cards ------------------------------------------------------------------------
    for (size_t i = 0; i < Presets::problems.size(); ++i)
    {
        auto& pr = Presets::problems[i];
        auto c = std::make_unique<ProblemCard> (apvts, (int) i, pr.id, pr.title, pr.blurb);
        addAndMakeVisible (*c);
        problemCards.push_back (std::move (c));
    }

    // --- Advanced strips ------------------------------------------------------------------------
    using K = ModuleStrip::Knob;
    auto add = [&] (int n, const juce::String& title, const char* onID, std::vector<K> knobs)
    {
        auto s = std::make_unique<ModuleStrip> (apvts, n, title, juce::String (onID), std::move (knobs));
        s->setVisible (false);
        addChildComponent (*s);
        strips.push_back (std::move (s));
    };
    add (1,  "High-Pass",     ParamID::hpfOn,  { { ParamID::hpfFreq, "Freq" } });
    add (2,  "Hum",           ParamID::humOn,  { { ParamID::humDepth, "Depth" } });
    add (3,  "Denoise",       ParamID::dnOn,   { { ParamID::dnAmount, "Amount" } });
    add (4,  "De-reverb",     ParamID::drOn,   { { ParamID::drAmount, "Amount" } });
    add (5,  "Gate",          ParamID::gateOn, { { ParamID::gateThresh, "Thresh" }, { ParamID::gateRange, "Range" }, { ParamID::gateRelease, "Release" } });
    add (6,  "Click / Plosive", ParamID::dcOn, { { ParamID::dcSens, "Sens" } });
    add (7,  "Clip Repair",   ParamID::clipOn, { { ParamID::clipThresh, "Thresh" } });
    add (8,  "Voice EQ",      ParamID::eqOn,   { { ParamID::eqLowCut, "Mud" }, { ParamID::eqPresence, "Presence" }, { ParamID::eqAir, "Air" } });
    add (9,  "De-esser",      ParamID::dsOn,   { { ParamID::dsFreq, "Freq" }, { ParamID::dsAmount, "Amount" } });
    add (10, "Multiband",     ParamID::mbOn,   { { ParamID::mbAmount, "Amount" } });
    add (11, "Leveler",       ParamID::lvOn,   { { ParamID::lvAmount, "Amount" }, { ParamID::lvTarget, "Target" } });
    add (12, "Loudness",      ParamID::limOn,  { { ParamID::limCeiling, "Ceiling" } });

    setSize (width, simpleH);
    startTimerHz (30);
}

ContentCleanerEditor::~ContentCleanerEditor() { setLookAndFeel (nullptr); }

// --------------------------------------------------------------------------------------
void ContentCleanerEditor::resized()
{
    auto r = getLocalBounds();
    headerArea = r.removeFromTop (headerH);

    auto h = headerArea.reduced (16, 10);
    h.removeFromLeft (230);
    advancedButton.setBounds (h.removeFromRight (100));
    h.removeFromRight (10);
    targetBox.setBounds (h.removeFromRight (200));
    h.removeFromRight (10);
    presetBox.setBounds (h.removeFromRight (170));

    auto main = r.removeFromTop (simpleH - headerH).reduced (16, 14);

    // Left column: dial, character, output
    auto left = main.removeFromLeft (230);
    dialArea = left.removeFromTop (214);
    cleanDial.setBounds (dialArea.withTrimmedTop (16));
    left.removeFromTop (6);
    characterArea = left.removeFromTop (44);
    characterSlider.setBounds (characterArea.withTrimmedTop (16).reduced (6, 0));
    left.removeFromTop (10);
    auto out = left.removeFromTop (26);
    outputLabelArea = out.removeFromLeft (52);
    outputSlider.setBounds (out);
    main.removeFromLeft (16);

    // Right column: meters
    auto meters = main.removeFromRight (74);
    inMeterArea  = meters.removeFromLeft (32);
    meters.removeFromLeft (10);
    outMeterArea = meters;
    main.removeFromRight (16);

    // Middle: action row, problem grid, spectral strip
    auto actions = main.removeFromTop (34);
    analyzeButton.setBounds (actions.removeFromLeft (120));
    actions.removeFromLeft (10);
    learnButton.setBounds (actions.removeFromLeft (112));
    actions.removeFromLeft (14);
    deltaButton.setBounds (actions.removeFromLeft (170));
    statusArea = actions.removeFromRight (150);

    main.removeFromTop (12);
    auto grid = main.removeFromTop (2 * 120);
    const int cols = 4, cellW = grid.getWidth() / cols, cellH = 120;
    for (int i = 0; i < (int) problemCards.size(); ++i)
        problemCards[(size_t) i]->setBounds (grid.getX() + (i % cols) * cellW, grid.getY() + (i / cols) * cellH, cellW, cellH);
    reportArea = juce::Rectangle<int> (grid.getX() + 3 * cellW, grid.getY() + cellH, cellW, cellH);

    main.removeFromTop (10);
    spectralArea = main;

    if (advancedVisible)
    {
        auto g2 = r.reduced (13, 0).withTrimmedTop (20);
        const int scols = 6, sw = g2.getWidth() / scols;
        for (int i = 0; i < (int) strips.size(); ++i)
            strips[(size_t) i]->setBounds (g2.getX() + (i % scols) * sw, g2.getY() + (i / scols) * stripH, sw, stripH);
    }
}

// --------------------------------------------------------------------------------------
void ContentCleanerEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::background);
    drawHeader (g, headerArea);

    g.setColour (Theme::textDim);
    g.setFont (Theme::label (10.5f));
    g.drawText ("C L E A N", dialArea.withHeight (16), juce::Justification::centred);
    g.drawText ("OUTPUT", outputLabelArea, juce::Justification::centredLeft);

    drawCharacterLabels (g);
    drawStatusPill (g, statusArea);
    drawReportCard (g, reportArea);
    drawSpectralDisplay (g, spectralArea);
    drawMeter (g, inMeterArea,  processor.inputPeakDb.load(),  inHold,  "IN");
    drawMeter (g, outMeterArea, processor.outputPeakDb.load(), outHold, "OUT");

    if (advancedVisible)
    {
        g.setColour (Theme::border);
        g.drawHorizontalLine (simpleH, 16.f, (float) getWidth() - 16.f);
        g.setColour (Theme::textMute);
        g.setFont (Theme::label (9.5f));
        g.drawText ("UNDER THE HOOD", 16, simpleH - 2, 200, 16, juce::Justification::centredLeft);
    }
}

void ContentCleanerEditor::drawCharacterLabels (juce::Graphics& g)
{
    auto r = characterArea.withHeight (14);
    g.setFont (Theme::label (9.5f));
    const float c = (float) characterSlider.getValue() / 100.f;
    g.setColour (Theme::text.interpolatedWith (Theme::textMute, c));
    g.drawText ("NATURAL", r.reduced (6, 0), juce::Justification::centredLeft);
    g.setColour (Theme::text.interpolatedWith (Theme::textMute, 1.f - c));
    g.drawText ("AGGRESSIVE", r.reduced (6, 0), juce::Justification::centredRight);
}

void ContentCleanerEditor::drawHeader (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour (Theme::surface);
    g.fillRect (r);
    g.setColour (Theme::border);
    g.drawHorizontalLine (r.getBottom() - 1, 0.f, (float) getWidth());

    auto left = r.reduced (16, 0);
    auto mark = left.removeFromLeft (22).withSizeKeepingCentre (18, 18).toFloat();
    juce::Path mp; mp.addRoundedRectangle (mark, 5.f);
    g.setGradientFill (Theme::accentGradient (mark));
    g.fillPath (mp);
    g.setColour (Theme::background);
    g.fillEllipse (mark.reduced (5.f));

    left.removeFromLeft (10);
    g.setColour (Theme::text);
    g.setFont (Theme::title (15.f));
    g.drawText ("Content Cleaner", left.removeFromLeft (130), juce::Justification::centredLeft);

    auto tag = left.removeFromLeft (46).withSizeKeepingCentre (42, 18).toFloat();
    g.setColour (Theme::surfaceHi);
    g.fillRoundedRectangle (tag, 4.f);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (tag, 4.f, 1.f);
    g.setColour (Theme::textDim);
    g.setFont (Theme::mono (9.5f));
    g.drawText ("v0.2", tag, juce::Justification::centred);
}

void ContentCleanerEditor::drawStatusPill (juce::Graphics& g, juce::Rectangle<int> r)
{
    juce::String text; juce::Colour col;
    if (processor.isAnalyzing())         { text = "ANALYZING";    col = Theme::accent; }
    else if (processor.isLearningRoom()) { text = "LISTENING";    col = Theme::warn; }
    else if (processor.hasRoomProfile()) { text = "ROOM LEARNED"; col = Theme::ok; }
    else                                 { text = "AUTO";         col = Theme::textDim; }

    auto pill = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 130), 22).toFloat();
    g.setColour (col.withAlpha (0.12f));
    g.fillRoundedRectangle (pill, 11.f);
    g.setColour (col.withAlpha (0.5f));
    g.drawRoundedRectangle (pill, 11.f, 1.f);
    g.setColour (col);
    g.fillEllipse (pill.getX() + 9.f, pill.getCentreY() - 3.f, 6.f, 6.f);
    g.setFont (Theme::label (9.5f));
    g.drawText (text, pill.withTrimmedLeft (20.f), juce::Justification::centredLeft);
}

void ContentCleanerEditor::drawReportCard (juce::Graphics& g, juce::Rectangle<int> r)
{
    auto box = r.toFloat().reduced (4.f);
    const bool busy = processor.isAnalyzing();
    auto rep = processor.getLastReport();

    juce::ColourGradient bg (Theme::background.brighter (0.03f), 0.f, box.getY(), Theme::background, 0.f, box.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (box, 10.f);
    g.setColour (busy ? Theme::accent.withAlpha (0.5f) : Theme::border);
    g.drawRoundedRectangle (box, 10.f, 1.f);

    auto inner = r.reduced (14, 11);
    auto head = inner.removeFromTop (16);
    g.setColour (busy ? Theme::accent : Theme::textDim);
    g.setFont (Theme::label (9.5f));
    g.drawText (busy ? "LISTENING" : "WHAT I FOUND", head, juce::Justification::centredLeft);
    if (rep.valid && ! busy)
    {
        g.setColour (Theme::textMute);
        g.setFont (Theme::mono (9.f));
        g.drawText (juce::String ((int) std::round (rep.snrDb)) + " dB clean", head, juce::Justification::centredRight);
    }
    inner.removeFromTop (6);

    g.setFont (Theme::body (10.5f));
    if (busy)
    {
        // animated dots
        const int dots = (int) (juce::Time::getMillisecondCounter() / 350) % 4;
        g.setColour (Theme::text);
        g.drawText ("Analyzing the clip" + juce::String::repeatedString (".", dots), inner.removeFromTop (14), juce::Justification::centredLeft);
        auto bar = inner.removeFromTop (10).withHeight (4).toFloat();
        g.setColour (Theme::track); g.fillRoundedRectangle (bar, 2.f);
        const float ph = (float) (juce::Time::getMillisecondCounter() % 1500) / 1500.f;
        g.setGradientFill (Theme::accentGradient (bar));
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * 0.3f).translated (bar.getWidth() * 0.7f * ph, 0.f), 2.f);
    }
    else if (! rep.valid)
    {
        g.setColour (Theme::textDim);
        g.drawFittedText ("Play the clip and press Analyze.", inner.removeFromTop (14), juce::Justification::centredLeft, 1);
        g.setColour (Theme::textMute);
        g.drawFittedText ("I'll set the problems above for you.", inner.removeFromTop (14), juce::Justification::centredLeft, 1);
    }
    else
    {
        int shown = 0;
        for (size_t i = 0; i < Presets::problems.size() && shown < 3; ++i)
        {
            if (rep.levels[i] == 0) continue;
            auto row = inner.removeFromTop (15);
            const juce::Colour c = SegmentedControl::colourForLevel (rep.levels[i]);
            g.setColour (c);
            g.fillEllipse (row.removeFromLeft (10).toFloat().withSizeKeepingCentre (6.f, 6.f));
            row.removeFromLeft (4);
            g.setColour (Theme::text);
            g.drawText (Presets::problems[i].title, row, juce::Justification::centredLeft, true);
            ++shown;
        }
        if (shown == 0)
        {
            g.setColour (Theme::ok);
            g.fillEllipse (inner.removeFromLeft (10).toFloat().withSizeKeepingCentre (6.f, 6.f).withY ((float) inner.getY() + 4.f));
            g.setColour (Theme::text);
            g.drawText ("Sounds clean already", inner.withTrimmedLeft (4).removeFromTop (15), juce::Justification::centredLeft);
        }
    }
}

void ContentCleanerEditor::drawMeter (juce::Graphics& g, juce::Rectangle<int> r, float db, float& hold, const juce::String& label)
{
    auto lab = r.removeFromBottom (14);
    g.setColour (Theme::textDim);
    g.setFont (Theme::label (9.5f));
    g.drawText (label, lab, juce::Justification::centred);

    auto body = r.toFloat().reduced (0.f, 2.f);
    g.setColour (Theme::surface);
    g.fillRoundedRectangle (body, 4.f);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (body, 4.f, 1.f);

    hold = db > hold ? db : hold - 0.35f;
    const float norm = juce::jlimit (0.f, 1.f, (db + 60.f) / 60.f);
    const float holdNorm = juce::jlimit (0.f, 1.f, (hold + 60.f) / 60.f);

    const int segs = 30;
    const float segH = (body.getHeight() - 6.f) / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        const float level = (float) (i + 1) / (float) segs;
        auto seg = juce::Rectangle<float> (body.getX() + 4.f, body.getBottom() - 3.f - (float) (i + 1) * segH + 1.f, body.getWidth() - 8.f, segH - 2.f);
        juce::Colour c = level > 0.92f ? Theme::removed : level > 0.75f ? Theme::warn : Theme::accent;
        g.setColour (level <= norm ? c : c.withAlpha (0.08f));
        g.fillRoundedRectangle (seg, 1.f);
    }
    const float hy = body.getBottom() - 3.f - holdNorm * (body.getHeight() - 6.f);
    g.setColour (Theme::text);
    g.fillRect (body.getX() + 4.f, hy - 1.f, body.getWidth() - 8.f, 2.f);
}

void ContentCleanerEditor::drawSpectralDisplay (juce::Graphics& g, juce::Rectangle<int> r)
{
    auto box = r.toFloat();
    juce::ColourGradient bg (Theme::surface, 0.f, box.getY(), Theme::background.brighter (0.02f), 0.f, box.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (box, 10.f);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (box.reduced (0.5f), 10.f, 1.f);

    auto inner = box.reduced (12.f, 8.f).withTrimmedBottom (12.f).withTrimmedTop (16.f).withTrimmedLeft (28.f);
    const float minF = 40.f, maxF = 20000.f, nyquist = 24000.f;
    auto xForF = [&] (float f) { return inner.getX() + inner.getWidth() * std::log (f / minF) / std::log (maxF / minF); };

    // grid + frequency labels
    g.setFont (Theme::mono (8.5f));
    for (float f : { 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = xForF (f);
        const bool major = f == 100.f || f == 1000.f || f == 10000.f;
        g.setColour (Theme::border.withAlpha (major ? 0.9f : 0.4f));
        g.drawVerticalLine ((int) x, inner.getY(), inner.getBottom());
        if (major)
        {
            g.setColour (Theme::textMute);
            g.drawText (f >= 1000.f ? juce::String ((int) (f / 1000.f)) + "k" : juce::String ((int) f),
                        (int) x - 14, (int) inner.getBottom() + 1, 28, 11, juce::Justification::centred);
        }
    }
    // dB scale for the removed curve (0 at bottom, -12 dB at top)
    for (int i = 0; i <= 3; ++i)
    {
        const float yy = inner.getBottom() - inner.getHeight() * (float) i / 3.f;
        g.setColour (Theme::border.withAlpha (0.35f));
        g.drawHorizontalLine ((int) yy, inner.getX(), inner.getRight());
        g.setColour (Theme::textMute);
        g.drawText (i == 0 ? "0" : juce::String (-4 * i), (int) box.getX() + 8, (int) yy - 6, 18, 11, juce::Justification::centredRight);
    }

    g.setColour (Theme::textDim);
    g.setFont (Theme::label (9.5f));
    g.drawText ("WHAT'S BEING REMOVED", box.reduced (12.f, 5.f), juce::Justification::topLeft);
    g.setColour (Theme::textMute);
    g.setFont (Theme::mono (9.f));
    const float lufs = processor.getChain().getLoudness().getShortTermLUFS();
    g.drawText (lufs > -69.f ? juce::String (lufs, 1) + " LUFS" : "-- LUFS", box.reduced (12.f, 5.f), juce::Justification::topRight);

    const auto& gains = processor.getChain().getSpectral().getLastGains();
    const auto& mags  = processor.getChain().getSpectral().getLastMags();
    const bool haveAudio = processor.inputPeakDb.load() > -80.f && ! gains.empty();

    if (! haveAudio)
    {
        g.setColour (Theme::textMute);
        g.setFont (Theme::body (11.f));
        g.drawText ("Waiting for audio", inner, juce::Justification::centred);
        return;
    }

    if (smoothedGains.size() != gains.size()) smoothedGains = gains;
    for (size_t i = 0; i < gains.size(); ++i) smoothedGains[i] += 0.35f * (gains[i] - smoothedGains[i]);
    const int bins = (int) smoothedGains.size();

    // Input spectrum (dim) behind everything: -90..0 dBFS-ish mapped to the box height.
    if (mags.size() == gains.size())
    {
        juce::Path sp;
        sp.startNewSubPath (inner.getX(), inner.getBottom());
        for (int k = 1; k < bins; ++k)
        {
            const float f = (float) k / (float) (bins - 1) * nyquist;
            if (f < minF || f > maxF) continue;
            const float db = juce::Decibels::gainToDecibels (mags[(size_t) k] / 256.f, -90.f);    // FFT-size normalised
            const float y = inner.getBottom() - inner.getHeight() * juce::jlimit (0.f, 1.f, (db + 90.f) / 90.f);
            sp.lineTo (xForF (f), y);
        }
        sp.lineTo (inner.getRight(), inner.getBottom());
        sp.closeSubPath();
        g.setColour (Theme::accent.withAlpha (0.10f));
        g.fillPath (sp);
        g.setColour (Theme::accent.withAlpha (0.35f));
        g.strokePath (sp, juce::PathStrokeType (1.f));
    }

    // Removed (attenuation in dB, 0..12 dB mapped to height)
    juce::Path p;
    p.startNewSubPath (inner.getX(), inner.getBottom());
    for (int k = 1; k < bins; ++k)
    {
        const float f = (float) k / (float) (bins - 1) * nyquist;
        if (f < minF || f > maxF) continue;
        const float attDb = -juce::Decibels::gainToDecibels (juce::jmax (smoothedGains[(size_t) k], 0.001f));
        const float y = inner.getBottom() - inner.getHeight() * juce::jlimit (0.f, 1.f, attDb / 12.f);
        p.lineTo (xForF (f), y);
    }
    p.lineTo (inner.getRight(), inner.getBottom());
    p.closeSubPath();
    juce::ColourGradient fill (Theme::removed.withAlpha (0.6f), 0.f, inner.getY(), Theme::removed.withAlpha (0.04f), 0.f, inner.getBottom(), false);
    g.setGradientFill (fill);
    g.fillPath (p);
    g.setColour (Theme::removed);
    g.strokePath (p, juce::PathStrokeType (1.4f));
}

void ContentCleanerEditor::timerCallback() { repaint(); }
