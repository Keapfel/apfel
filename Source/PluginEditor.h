#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Custom look-and-feel for the entire plugin UI
// ─────────────────────────────────────────────────────────────────────────────
class ChaosLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit ChaosLookAndFeel (bool isChaosKnob = false);

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;

    juce::Font getLabelFont (juce::Label&) override;

    bool forChaos = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// Plugin editor
// ─────────────────────────────────────────────────────────────────────────────
class ChaosChopperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit ChaosChopperAudioProcessorEditor (ChaosChopperAudioProcessor&);
    ~ChaosChopperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;      // polls division param, updates button highlights
    void updateDivisionButtons();

    ChaosChopperAudioProcessor& proc;

    ChaosLookAndFeel lafNormal;
    ChaosLookAndFeel lafChaos { true };

    // ── Title & chop toggle ───────────────────────────────────────────────
    juce::Label    titleLabel;
    juce::TextButton chopToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chopAtt;

    // ── Division buttons ──────────────────────────────────────────────────
    static constexpr int NUM_DIVS = 5;
    const char* kDivLabels[NUM_DIVS] = { "1/4","1/8","1/16","1/32","1/64" };
    juce::TextButton divBtn[NUM_DIVS];

    // ── Large central Chaos knob ──────────────────────────────────────────
    juce::Slider chaosKnob;
    juce::Label  chaosLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAtt;

    // ── Side knobs (pitch, formant, clip, dry/wet) ───────────────────────
    juce::Slider pitchKnob, formantKnob, clipKnob, dryWetKnob;
    juce::Label  pitchLabel, formantLabel, clipLabel, dryWetLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        pitchAtt, formantAtt, clipAtt, dryWetAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChaosChopperAudioProcessorEditor)
};
