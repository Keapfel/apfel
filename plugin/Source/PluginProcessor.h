#pragma once

#include <JuceHeader.h>
#include "dsp/FormantShifter.h"
#include "dsp/TempoSyncedLFO.h"
#include "dsp/Saturator.h"

namespace voxshred
{
namespace ParamIDs
{
    static const juce::String shiftAmount { "shiftAmount" };
    static const juce::String depth       { "depth" };
    static const juce::String rate        { "rate" };
    static const juce::String mix         { "mix" };
    static const juce::String character   { "character" };
}
}

/**
    VOXSHRED — Vocal-Formant-Modulator
    Developer: Malus Audio

    Turns a vocal recording into a pulsing, tempo-locked synthetic instrument
    by decoupling and rhythmically modulating its formant structure.
*/
class VoxShredAudioProcessor : public juce::AudioProcessor
{
public:
    VoxShredAudioProcessor();
    ~VoxShredAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    /** Snapshot of the current formant envelope shape, safe to read from the
        message thread for UI display purposes. */
    const std::vector<float>& getFormantEnvelopeForUI() const noexcept
    {
        return formantShifters[0].getEnvelopeSnapshot();
    }

    float getModRatePhaseForUI() const noexcept { return lfo.getCurrentPhase(); }
    float getModRateOutputForUI() const noexcept { return lfo.getCurrentOutput(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::array<voxshred::FormantShifter, 2> formantShifters;
    voxshred::TempoSyncedLFO lfo;
    std::array<voxshred::Saturator, 2> saturators;

    std::atomic<float>* shiftAmountParam = nullptr;
    std::atomic<float>* depthParam       = nullptr;
    std::atomic<float>* rateParam        = nullptr;
    std::atomic<float>* mixParam         = nullptr;
    std::atomic<float>* characterParam   = nullptr;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxShredAudioProcessor)
};
