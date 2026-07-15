#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "gui/VoxShredLookAndFeel.h"
#include "gui/GlowKnob.h"
#include "gui/FormantCurveDisplay.h"
#include "gui/ModRateDisplay.h"

/**
    Compact, high-impact editor for VOXSHRED. Dark/aggressive aesthetic,
    modules colour-coded (formant = pink/red, rate = cyan, character =
    amber, mix = green), all controls are custom-drawn glowing rotary knobs.
*/
class VoxShredAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VoxShredAudioProcessorEditor (VoxShredAudioProcessor&);
    ~VoxShredAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VoxShredAudioProcessor& audioProcessor;

    voxshred::VoxShredLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    voxshred::GlowKnob shiftKnob, depthKnob, rateKnob, mixKnob, characterKnob;

    voxshred::FormantCurveDisplay formantDisplay;
    voxshred::ModRateDisplay modRateDisplay;

    juce::Label formantSectionLabel, rateSectionLabel, characterSectionLabel, mixSectionLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxShredAudioProcessorEditor)
};
