#include "PluginProcessor.h"
#include "PluginEditor.h"

ContentCleanerProcessor::ContentCleanerProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    apvts.addParameterListener (ParamID::dnLearn, this);
    apvts.addParameterListener (ParamID::analyze, this);
    apvts.addParameterListener (ParamID::preset, this);
    for (auto& pr : Presets::problems) apvts.addParameterListener (pr.id, this);

    // Problem defaults are authoritative: push them into the module layer once.
    for (auto& pr : Presets::problems)
        applyProblem (pr.id, (int) apvts.getRawParameterValue (pr.id)->load());
    startTimerHz (10);
}

ContentCleanerProcessor::~ContentCleanerProcessor()
{
    apvts.removeParameterListener (ParamID::dnLearn, this);
    apvts.removeParameterListener (ParamID::analyze, this);
    apvts.removeParameterListener (ParamID::preset, this);
    for (auto& pr : Presets::problems) apvts.removeParameterListener (pr.id, this);
}

bool ContentCleanerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == layouts.getMainInputChannelSet();
}

void ContentCleanerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };
    chain.prepare (spec);
    analyzer.prepare (sampleRate, samplesPerBlock, (int) spec.numChannels);
    analysisCopy.setSize ((int) spec.numChannels, samplesPerBlock);
    chain.applySettings (readSettings());
    setLatencySamples (chain.getLatencySamples());
}

ModuleChain::Settings ContentCleanerProcessor::readSettings() const
{
    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    ModuleChain::Settings s;
    s.clean        = p (ParamID::clean) / 100.f;
    s.character    = p (ParamID::character) / 100.f;
    s.deltaListen  = p (ParamID::deltaListen) > 0.5f;
    s.outputGainDb = p (ParamID::outputGain);

    s.hpfOn  = p (ParamID::hpfOn) > 0.5f;  s.hpfFreq  = p (ParamID::hpfFreq);
    s.humOn  = p (ParamID::humOn) > 0.5f;  s.humFreq  = p (ParamID::humFreq) < 0.5f ? 50.f : 60.f;
    s.humDepth = p (ParamID::humDepth);
    s.dnOn   = p (ParamID::dnOn) > 0.5f;   s.dnAmount = p (ParamID::dnAmount) / 100.f;
    s.drOn   = p (ParamID::drOn) > 0.5f;   s.drAmount = p (ParamID::drAmount) / 100.f;
    s.gateOn = p (ParamID::gateOn) > 0.5f; s.gateThresh = p (ParamID::gateThresh);
    s.gateRange = p (ParamID::gateRange);  s.gateRelease = p (ParamID::gateRelease);
    s.dcOn   = p (ParamID::dcOn) > 0.5f;   s.dcSens = p (ParamID::dcSens) / 100.f;
    s.plosiveOn = p (ParamID::plosiveOn) > 0.5f;
    s.clipOn = p (ParamID::clipOn) > 0.5f; s.clipThresh = p (ParamID::clipThresh);
    s.eqOn   = p (ParamID::eqOn) > 0.5f;   s.eqMud = p (ParamID::eqLowCut);
    s.eqPresence = p (ParamID::eqPresence); s.eqAir = p (ParamID::eqAir);
    s.dsOn   = p (ParamID::dsOn) > 0.5f;   s.dsFreq = p (ParamID::dsFreq); s.dsAmount = p (ParamID::dsAmount) / 100.f;
    s.mbOn   = p (ParamID::mbOn) > 0.5f;   s.mbAmount = p (ParamID::mbAmount) / 100.f;
    s.lvOn   = p (ParamID::lvOn) > 0.5f;   s.lvAmount = p (ParamID::lvAmount) / 100.f; s.lvTarget = p (ParamID::lvTarget);
    s.limOn  = p (ParamID::limOn) > 0.5f;  s.limCeiling = p (ParamID::limCeiling);
    s.targetLUFS = Presets::targetLUFS ((int) p (ParamID::target));
    return s;
}

void ContentCleanerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (learnRequested.exchange (false))
        chain.getSpectral().startLearning (2.0f);
    if (analyzeRequested.exchange (false))
        analyzer.start (5.0f);

    analyzer.process (buffer);          // looks at the raw input
    chain.applySettings (readSettings());

    inputPeakDb.store (juce::Decibels::gainToDecibels (buffer.getMagnitude (0, buffer.getNumSamples()), -100.f));
    chain.process (buffer);
    outputPeakDb.store (juce::Decibels::gainToDecibels (buffer.getMagnitude (0, buffer.getNumSamples()), -100.f));
}

// --- Learn Room + presets ----------------------------------------------------------
void ContentCleanerProcessor::parameterChanged (const juce::String& id, float value)
{
    if (id == ParamID::dnLearn && value > 0.5f)
        learnRequested.store (true);
    else if (id == ParamID::analyze && value > 0.5f)
        analyzeRequested.store (true);
    else if (id == ParamID::preset && ! applyingPreset)
        applyMicPreset ((int) value);
    else if (id.startsWith ("p_") && ! applyingPreset)
        applyProblem (id, (int) std::round (value));
}

void ContentCleanerProcessor::timerCallback()
{
    // Auto-release the momentary Learn button once capture has finished.
    auto release = [this] (const char* id, bool pending, bool busy)
    {
        auto* prm = apvts.getParameter (id);
        if (prm->getValue() > 0.5f && ! pending && ! busy)
        {
            prm->beginChangeGesture(); prm->setValueNotifyingHost (0.f); prm->endChangeGesture();
        }
    };
    release (ParamID::dnLearn, learnRequested.load(), isLearningRoom());
    release (ParamID::analyze, analyzeRequested.load(), isAnalyzing());

    if (analyzer.hasNewReport())
    {
        auto r = analyzer.takeReport();
        { const juce::ScopedLock sl (reportLock); lastReport = r; }
        // Push recommendations into the problem layer (which cascades to modules).
        for (size_t i = 0; i < Presets::problems.size(); ++i)
            if (auto* prm = apvts.getParameter (Presets::problems[i].id))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) r.levels[i]));
                prm->endChangeGesture();
            }
        if (r.levels[2] > 0)
            if (auto* hf = apvts.getParameter (ParamID::humFreq))
            { hf->beginChangeGesture(); hf->setValueNotifyingHost (r.humFreq >= 55.f ? 1.f : 0.f); hf->endChangeGesture(); }
    }
}

void ContentCleanerProcessor::applyMicPreset (int idx)
{
    struct P { const char* id; float value; };
    std::vector<P> values;

    switch (idx)
    {
        case 0: // Lavalier: chest resonance, clothing rustle, low self-noise
            values = { { ParamID::hpfFreq, 90.f }, { ParamID::eqLowCut, -3.f }, { ParamID::eqPresence, 2.f },
                       { ParamID::dnAmount, 40.f }, { ParamID::drAmount, 25.f }, { ParamID::dsAmount, 35.f } };
            break;
        case 1: // Shotgun / boom: room pickup, handling noise
            values = { { ParamID::hpfFreq, 80.f }, { ParamID::eqLowCut, -1.5f }, { ParamID::eqPresence, 1.f },
                       { ParamID::dnAmount, 45.f }, { ParamID::drAmount, 45.f }, { ParamID::gateThresh, -48.f } };
            break;
        case 2: // On-camera mic: distant, roomy, thin, often windy
            values = { { ParamID::hpfFreq, 100.f }, { ParamID::eqLowCut, 0.f }, { ParamID::eqPresence, 2.5f },
                       { ParamID::eqAir, 1.5f }, { ParamID::dnAmount, 60.f }, { ParamID::drAmount, 60.f },
                       { ParamID::lvAmount, 70.f } };
            break;
        case 3: // USB / podcast: proximity boom, plosives, sibilance
            values = { { ParamID::hpfFreq, 70.f }, { ParamID::eqLowCut, -3.f }, { ParamID::dsAmount, 50.f },
                       { ParamID::dnAmount, 30.f }, { ParamID::drAmount, 15.f }, { ParamID::mbAmount, 50.f } };
            break;
        case 4: // Phone recording: band-limited, compressed, noisy
            values = { { ParamID::hpfFreq, 120.f }, { ParamID::eqPresence, 3.f }, { ParamID::eqAir, 2.f },
                       { ParamID::dnAmount, 55.f }, { ParamID::drAmount, 35.f }, { ParamID::clipOn, 1.f } };
            break;
        case 5: // Zoom / webcam: codec artefacts, AGC pumping
            values = { { ParamID::hpfFreq, 100.f }, { ParamID::dnAmount, 50.f }, { ParamID::drAmount, 30.f },
                       { ParamID::lvAmount, 60.f }, { ParamID::mbAmount, 30.f }, { ParamID::eqPresence, 2.f } };
            break;
        default: return; // Custom: leave everything alone
    }

    applyingPreset = true;
    for (auto& v : values)
        if (auto* param = apvts.getParameter (v.id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (v.value));
            param->endChangeGesture();
        }
    applyingPreset = false;
}

void ContentCleanerProcessor::applyProblem (const juce::String& id, int L)
{
    struct P { const char* id; float value; };
    std::vector<P> v;
    const bool on = L > 0;
    auto pick = [L] (float a, float b, float c, float d) { return L <= 0 ? a : L == 1 ? b : L == 2 ? c : d; };

    if (id == ParamID::pRoom)
        v = { { ParamID::drOn, on }, { ParamID::drAmount, pick (0.f, 25.f, 50.f, 80.f) },
              { ParamID::gateOn, on }, { ParamID::gateRange, pick (0.f, 6.f, 10.f, 16.f) } };
    else if (id == ParamID::pNoise)
        v = { { ParamID::dnOn, on }, { ParamID::dnAmount, pick (0.f, 30.f, 55.f, 80.f) } };
    else if (id == ParamID::pHum)
        v = { { ParamID::humOn, on }, { ParamID::humDepth, pick (0.f, 20.f, 35.f, 50.f) } };
    else if (id == ParamID::pPops)
        v = { { ParamID::plosiveOn, on }, { ParamID::dcOn, L >= 2 }, { ParamID::dcSens, pick (0.f, 40.f, 60.f, 80.f) } };
    else if (id == ParamID::pHarsh)
        v = { { ParamID::dsOn, on }, { ParamID::dsAmount, pick (0.f, 25.f, 45.f, 70.f) } };
    else if (id == ParamID::pUneven)
        v = { { ParamID::lvOn, on }, { ParamID::lvAmount, pick (0.f, 35.f, 60.f, 85.f) },
              { ParamID::mbOn, on }, { ParamID::mbAmount, pick (0.f, 25.f, 40.f, 60.f) } };
    else if (id == ParamID::pDistorted)
        v = { { ParamID::clipOn, on }, { ParamID::clipThresh, pick (-0.3f, -0.3f, -1.0f, -2.0f) } };
    else return;

    applyingPreset = true;
    for (auto& p : v)
        if (auto* prm = apvts.getParameter (p.id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (p.value));
            prm->endChangeGesture();
        }
    applyingPreset = false;
}

// --- State ---------------------------------------------------------------------------
void ContentCleanerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void ContentCleanerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* ContentCleanerProcessor::createEditor()
{
    return new ContentCleanerEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ContentCleanerProcessor();
}
