#pragma once

#include <juce_dsp/juce_dsp.h>

namespace voxshred
{

/**
    "Character" saturation stage.

    A tanh-based waveshaper with an asymmetry term and a pre/post gain-staged
    tone control, used to add grit and help the processed vocal cut through
    a dense Montagem mix.
*/
class Saturator
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        smoothedDrive.reset (spec.sampleRate, 0.02);
        smoothedDrive.setCurrentAndTargetValue (1.0f);
    }

    void reset() noexcept { smoothedDrive.reset (smoothedDrive.getTargetValue()); }

    /** amount is 0-1, mapped internally to a musically useful drive range. */
    void setAmount (float amount) noexcept
    {
        amount = juce::jlimit (0.0f, 1.0f, amount);
        smoothedDrive.setTargetValue (1.0f + amount * 9.0f);
    }

    float processSample (float inputSample) noexcept
    {
        const auto drive = smoothedDrive.getNextValue();
        auto driven = inputSample * drive;

        // Slight asymmetry adds odd+even harmonics for a grittier, less
        // "clean digital" saturation character.
        auto shaped = std::tanh (driven + 0.15f * driven * driven);

        // Compensate makeup gain so the wet signal doesn't get quieter as
        // drive is reduced back towards unity.
        return shaped / std::tanh (juce::jmax (1.0f, drive));
    }

private:
    juce::SmoothedValue<float> smoothedDrive;
};

} // namespace voxshred
