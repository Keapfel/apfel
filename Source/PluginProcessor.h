#pragma once
#include <JuceHeader.h>
#include "PitchShifter.h"

class ChaosChopperAudioProcessor : public juce::AudioProcessor
{
public:
    ChaosChopperAudioProcessor();
    ~ChaosChopperAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    double currentSampleRate = 44100.0;

    // ── Stutter / chop engine ────────────────────────────────────────────
    // Up to ~4 s at 48 kHz for the slowest chop (1/4 @ 30 BPM)
    static constexpr int MAX_BUF = 192000;

    juce::AudioBuffer<float> rollingBuf;   // circular input history
    juce::AudioBuffer<float> chopBuf;      // frozen snapshot for stutter loop

    int    rollingWritePos = 0;
    double chopReadPos     = 0.0;   // fractional read head through chopBuf
    int    samplesIntoChop = 0;     // where we are within the current chop unit
    int    chopUnitSamples = 0;     // current chop-unit length in samples
    bool   chopBufReady    = false;

    // ── Phase-vocoder pitch shifter (stereo) ─────────────────────────────
    PitchShifter pitchL, pitchR;

    // ── Formant filters – two resonant bandpass peaks (stereo) ───────────
    juce::dsp::StateVariableTPTFilter<float> formF1L, formF1R, formF2L, formF2R;
    float lastFormantValue = -999.0f;

    // ── Chaos state ───────────────────────────────────────────────────────
    juce::Random rng { 0xDEADBEEF };
    float chaosExtraPitch     = 0.0f;  // semitones added by chaos
    bool  chaosReverse        = false; // read chopBuf backwards
    int   chaosOverrideDivIdx = -1;    // -1 = use param; ≥0 = faster division
    int   chaosHoldUnits      = 0;     // chop units remaining for current chaos state

    // ── Helpers ───────────────────────────────────────────────────────────
    int    getDivisionSamples (int divIdx, double bpm) const;
    double getHostBpm() const;
    void   snapshotToChopBuffer (int numCh, int chopSamples);
    void   triggerChaos (float chaosAmt, int baseDivIdx);
    void   updateFormantFilters (float formantSemitones);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChaosChopperAudioProcessor)
};
