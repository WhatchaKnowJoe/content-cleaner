#pragma once
#include "ModuleBase.h"

/*  11. Speech leveler — slow automatic gain rider.
    Tracks a ~400 ms RMS of the (linked) input and nudges gain toward the target
    level, within ±maxGainDb, but only while the signal is above a speech floor
    so silence and room tone are never pumped up. Amount scales how much of the
    correction is applied (0 = off, 1 = full ride).                                */
class Leveler : public ModuleBase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer) override;

    void setTargetDb (float dbfs) noexcept { targetDb = dbfs; }
    void setAmount (float a01) noexcept    { amount = a01; }
    float getCurrentGainDb() const noexcept { return currentGainDb; }

private:
    float targetDb = -18.f, amount = 0.5f;
    float maxGainDb = 12.f, speechFloorDb = -45.f;
    float rmsCoeff = 0.f, gainUpCoeff = 0.f, gainDownCoeff = 0.f;
    float rmsSq = 0.f, currentGainDb = 0.f;
};
