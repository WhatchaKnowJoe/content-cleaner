#include "ModuleChain.h"

void ModuleChain::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    hpf.prepare (spec);
    lastHpfFreq = -1.f;

    for (ModuleBase* m : std::initializer_list<ModuleBase*> { &hum, &spectral, &gate, &declick, &clip, &eq, &deess, &multiband, &leveler, &loudness })
        m->prepare (spec);

    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) getLatencySamples());
    dryBuffer.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);

    outputGain.prepare (spec);
    outputGain.setRampDurationSeconds (0.02);
    reset();
}

void ModuleChain::reset()
{
    hpf.reset();
    for (ModuleBase* m : std::initializer_list<ModuleBase*> { &hum, &spectral, &gate, &declick, &clip, &eq, &deess, &multiband, &leveler, &loudness })
        m->reset();
    dryDelay.reset();
    outputGain.reset();
}

int ModuleChain::getLatencySamples() const
{
    return spectral.getLatencySamples() + clip.getLatencySamples();
}

void ModuleChain::applySettings (const Settings& s)
{
    // Clean macro: 0.5 → amounts exactly as set; 0 → nothing; 1 → up to 2x (clamped).
    const float k = juce::jlimit (0.f, 2.f, s.clean * 2.f);
    auto scaled = [k] (float a) { return juce::jlimit (0.f, 1.f, a * k); };

    deltaListen = s.deltaListen;
    outputGain.setGainDecibels (s.outputGainDb);

    hpfOn = s.hpfOn;
    if (s.hpfFreq != lastHpfFreq)
    {
        lastHpfFreq = s.hpfFreq;
        *hpf.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, s.hpfFreq, 0.707f);
    }

    hum.setBypassed (! s.humOn);
    hum.setFundamental (s.humFreq);
    hum.setDepthDb (s.humDepth);

    spectral.setBypassed (! (s.dnOn || s.drOn));
    spectral.setDenoiseEnabled (s.dnOn);
    spectral.setDereverbEnabled (s.drOn);
    spectral.setCharacter (s.character);
    spectral.setDenoiseAmount (scaled (s.dnAmount));
    spectral.setDereverbAmount (scaled (s.drAmount));

    gate.setBypassed (! s.gateOn);
    gate.setThresholdDb (s.gateThresh);
    gate.setRangeDb (s.gateRange * juce::jlimit (0.f, 1.f, k) * (0.6f + 0.4f * s.character));
    gate.setReleaseMs (s.gateRelease * (1.4f - 0.6f * s.character));

    declick.setBypassed (! (s.dcOn || s.plosiveOn));
    declick.setDeClickEnabled (s.dcOn);
    declick.setPlosiveEnabled (s.plosiveOn);
    declick.setSensitivity (s.dcSens);

    clip.setBypassed (! s.clipOn);
    clip.setThresholdDb (s.clipThresh);

    eq.setBypassed (! s.eqOn);
    eq.setMudDb (s.eqMud); eq.setPresenceDb (s.eqPresence); eq.setAirDb (s.eqAir);

    deess.setBypassed (! s.dsOn);
    deess.setFrequency (s.dsFreq);
    deess.setAmount (scaled (s.dsAmount));

    multiband.setBypassed (! s.mbOn);
    multiband.setAmount (scaled (s.mbAmount));

    leveler.setBypassed (! s.lvOn);
    leveler.setAmount (scaled (s.lvAmount));
    leveler.setTargetDb (s.lvTarget);

    loudness.setBypassed (false);                // always runs; target 0 disables steering
    loudness.setLimiterEnabled (s.limOn);
    loudness.setCeilingDb (s.limCeiling);
    loudness.setTargetLUFS (s.targetLUFS);
}

void ModuleChain::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();

    // Latency-matched dry copy for Delta Listen.
    dryBuffer.setSize (numCh, n, false, false, true);
    for (int c = 0; c < numCh; ++c)
    {
        const float* in = buffer.getReadPointer (c);
        float* dry = dryBuffer.getWritePointer (c);
        for (int i = 0; i < n; ++i) { dryDelay.pushSample (c, in[i]); dry[i] = dryDelay.popSample (c); }
    }

    // 1. HPF
    if (hpfOn)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        hpf.process (ctx);
    }

    // 2 → 7: repair stages
    hum.process (buffer);
    spectral.process (buffer);       // 3 + 4
    gate.process (buffer);
    declick.process (buffer);
    clip.process (buffer);

    // 8 → 12: polish stages
    eq.process (buffer);
    deess.process (buffer);
    multiband.process (buffer);
    leveler.process (buffer);
    loudness.process (buffer);

    if (deltaListen)
        for (int c = 0; c < numCh; ++c)
        {
            float* w = buffer.getWritePointer (c);
            const float* d = dryBuffer.getReadPointer (c);
            for (int i = 0; i < n; ++i) w[i] = d[i] - w[i];
        }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    outputGain.process (ctx);
}
