#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace voxshred
{

/** Selectable rhythmic sync divisions, relative to a whole note (1/1). */
enum class SyncDivision
{
    oneOverOne = 0,
    oneOverTwo,
    oneOverFour,
    oneOverEight,
    oneOverSixteen,
    oneOverThirtyTwo,

    numDivisions
};

inline juce::StringArray getSyncDivisionNames()
{
    return { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" };
}

inline double getSyncDivisionNoteFraction (SyncDivision division) noexcept
{
    switch (division)
    {
        case SyncDivision::oneOverOne:      return 1.0;
        case SyncDivision::oneOverTwo:      return 1.0 / 2.0;
        case SyncDivision::oneOverFour:     return 1.0 / 4.0;
        case SyncDivision::oneOverEight:    return 1.0 / 8.0;
        case SyncDivision::oneOverSixteen:  return 1.0 / 16.0;
        case SyncDivision::oneOverThirtyTwo:return 1.0 / 32.0;
        case SyncDivision::numDivisions:
        default:                            return 1.0 / 4.0;
    }
}

/**
    A rhythm-focused, host-tempo-locked sequencer/LFO.

    Produces a repeating pulse/ramp shape whose period is hard-locked to the
    DAW's tempo and transport position (rather than free-running time), so
    that rhythmic formant jumps and wobbles always land exactly on the beat
    grid, even after the host loops or the user scrubs the playhead.
*/
class TempoSyncedLFO
{
public:
    void prepare (double newSampleRate) noexcept { sampleRate = newSampleRate; }
    void reset() noexcept { phase = 0.0; smoothedOutput = 0.0f; }

    void setDivision (SyncDivision newDivision) noexcept { division = newDivision; }
    void setDepth (float newDepth) noexcept { depth = juce::jlimit (0.0f, 1.0f, newDepth); }

    /** Advances the LFO using the host's tempo (BPM) and its position in
        quarter notes (as reported by the DAW's playhead), and returns the
        current output in the range [-1, 1] scaled by depth. */
    float updateFromHostPosition (double bpm, double ppqPosition) noexcept
    {
        if (bpm <= 0.0)
            bpm = 120.0;

        const double quarterNotesPerCycle = getSyncDivisionNoteFraction (division) * 4.0;
        const double cyclePosition = std::fmod (ppqPosition / quarterNotesPerCycle, 1.0);

        phase = cyclePosition < 0.0 ? cyclePosition + 1.0 : cyclePosition;

        // Rhythmic "step + jump" shape: a fast rise then hold, reminiscent of
        // a sequenced formant jump rather than a smooth sine wobble.
        float shaped;
        if (phase < 0.15)
            shaped = (float) (phase / 0.15);
        else
            shaped = 1.0f - (float) ((phase - 0.15) / 0.85) * 0.3f;

        auto bipolar = shaped * 2.0f - 1.0f;
        smoothedOutput = bipolar * depth;
        return smoothedOutput;
    }

    /** Free-running fallback for standalone use without a host playhead. */
    float updateFreeRunning (double bpm, double blockSampleDelta) noexcept
    {
        if (bpm <= 0.0)
            bpm = 120.0;

        const double quarterNoteSeconds = 60.0 / bpm;
        const double cycleSeconds = quarterNoteSeconds * getSyncDivisionNoteFraction (division) * 4.0;
        const double cycleSamples = juce::jmax (1.0, cycleSeconds * sampleRate);

        phase += blockSampleDelta / cycleSamples;
        phase = std::fmod (phase, 1.0);

        float shaped;
        if (phase < 0.15)
            shaped = (float) (phase / 0.15);
        else
            shaped = 1.0f - (float) ((phase - 0.15) / 0.85) * 0.3f;

        auto bipolar = shaped * 2.0f - 1.0f;
        smoothedOutput = bipolar * depth;
        return smoothedOutput;
    }

    float getCurrentPhase() const noexcept { return (float) phase; }
    float getCurrentOutput() const noexcept { return smoothedOutput; }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    float smoothedOutput = 0.0f;

    SyncDivision division = SyncDivision::oneOverFour;
    float depth = 1.0f;
};

} // namespace voxshred
